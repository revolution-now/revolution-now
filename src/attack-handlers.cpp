/****************************************************************
**attack-handlers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-01.
*
* Description: Orders handlers for attacking.
*
*****************************************************************/
#include "attack-handlers.hpp"

// Revolution Now
#include "alarm.hpp"
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "conductor.hpp"
#include "icombat.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "on-map.hpp"
#include "orders.hpp"
#include "plane-stack.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"

// config
#include "config/nation.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// base
#include "base/to-str-ext-std.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

// These are possible results of an attack that are common to the
// two cases of attacking a euro unit and a native unit. Because
// they must be cases that are common to both, it follows that
// they can only be cases that result in the move being cancelled
// and/or not allowed.
enum class e_attack_verdict_base {
  cancelled,
  unit_cannot_attack,
  ship_attack_land_unit,
  attack_from_ship,
  land_unit_attack_ship
};

wait<> display_base_verdict_msg(
    IGui& gui, e_attack_verdict_base verdict ) {
  switch( verdict ) {
    case e_attack_verdict_base::cancelled: //
      break;
    // Non-allowed (would break game rules).
    case e_attack_verdict_base::ship_attack_land_unit:
      co_await gui.message_box(
          "Ships cannot attack land units." );
      break;
    case e_attack_verdict_base::unit_cannot_attack:
      co_await gui.message_box( "This unit cannot attack." );
      break;
    case e_attack_verdict_base::attack_from_ship:
      co_await gui.message_box(
          "We cannot attack a land unit from a ship." );
      break;
    case e_attack_verdict_base::land_unit_attack_ship:
      co_await gui.message_box(
          "Land units cannot attack ships that are at sea." );
      break;
  }
}

// This is for checking for no-go conditions that apply to both
// attacking a euro unit and attacking native units.
wait<maybe<e_attack_verdict_base>> check_attack_verdict_base(
    SSConst const& ss, TS& ts, Unit const& attacker,
    e_direction d ) {
  if( is_unit_onboard( ss.units, attacker.id() ) )
    co_return e_attack_verdict_base::attack_from_ship;

  Coord const source = ss.units.coord_for( attacker.id() );
  Coord const target = source.moved( d );

  if( !can_attack( attacker.type() ) )
    co_return e_attack_verdict_base::unit_cannot_attack;

  if( attacker.desc().ship ) {
    // Ship-specific checks.
    if( ss.terrain.is_land( target ) )
      co_return e_attack_verdict_base::ship_attack_land_unit;
  }

  if( surface_type( ss.terrain.square_at( target ) ) ==
      e_surface::water )
    co_return e_attack_verdict_base::land_unit_attack_ship;

  if( attacker.movement_points() < 1 ) {
    if( co_await ts.gui.optional_yes_no(
            { .msg = fmt::format(
                  "This unit only has @[H]{}@[] movement points "
                  "and so will not be fighting at full "
                  "strength.  Continue?",
                  attacker.movement_points() ),
              .yes_label =
                  "Yes, let us proceed with full force!",
              .no_label = "No, do not attack." } ) !=
        ui::e_confirm::yes )
      co_return e_attack_verdict_base::cancelled;
  }
  co_return nothing;
}

wait<> perform_euro_unit_combat_outcome(
    SS& ss, TS& ts, Player const& active_player,
    Player const& player, Unit& unit,
    EuroUnitCombatOutcome_t const& outcome ) {
  auto capture_unit = [&]( e_nation new_nation,
                           Coord    new_coord ) -> wait<> {
    unit.change_nation( ss.units, new_nation );
    maybe<UnitDeleted> const unit_deleted =
        co_await unit_to_map_square( ss, ts, unit.id(),
                                     new_coord );
    CHECK( !unit_deleted.has_value() );
    // This is so that the captured unit won't ask for orders
    // in the same turn that it is captured.
    unit.forfeight_mv_points();
    unit.clear_orders();
  };

  maybe<string> msg;

  switch( outcome.to_enum() ) {
    using namespace EuroUnitCombatOutcome;
    case e::no_change:
      break;
    case e::destroyed: {
      if( unit.desc().ship &&
          ss.terrain.is_land( ss.units.coord_for( unit.id() ) ) )
        msg =
            "Our ship, which was vulnerable in the abandoned "
            "colony port, has been lost due to an attack.";
      else
        // This will be scouts, pioneers, missionaries, and ar-
        // tillery.
        msg =
            fmt::format( "{} @[H]{}@[] has been lost in battle!",
                         nation_obj( unit.nation() ).adjective,
                         unit.desc().name );
      // Need to destroy the unit after accessing its info.
      ss.units.destroy_unit( unit.id() );
      break;
    }
    case e::captured: {
      auto& o = outcome.get<captured>();
      co_await capture_unit( o.new_nation, o.new_coord );
      break;
    }
    case e::captured_and_demoted: {
      auto& o = outcome.get<captured_and_demoted>();
      // Need to do this first before demoting the unit.
      msg = "Unit demoted upon capture!";
      if( unit.type() == e_unit_type::veteran_colonist )
        msg = "Veteran status lost upon capture!";
      unit.change_type( player,
                        UnitComposition::create( o.to ) );
      co_await capture_unit( o.new_nation, o.new_coord );
      break;
    }
    case e::promoted: {
      auto& o = outcome.get<promoted>();
      // TODO: make this message more specific like in the OG.
      msg = "Unit promoted for valor in combat!";
      unit.change_type( player,
                        UnitComposition::create( o.to ) );
      break;
    }
    case e::demoted: {
      auto& o = outcome.get<demoted>();
      unit.change_type( player,
                        UnitComposition::create( o.to ) );
      break;
    }
  }

  if( player.nation == active_player.nation &&
      active_player.human ) {
    // TODO: not sure yet how we're going to do this GUI handling
    // with AI players. For now we just say that we will only pop
    // up a message box notifying the player of battle outcomes
    // for their unit if the active player owns the unit and that
    // active player is a human.
    if( msg.has_value() ) co_await ts.gui.message_box( *msg );
  }
}

wait<> perform_native_unit_combat_outcome(
    SS& ss, TS& ts, NativeUnit& unit,
    NativeUnitCombatOutcome_t const& outcome,
    bool                             should_message ) {
  maybe<string> msg;

  switch( outcome.to_enum() ) {
    using namespace NativeUnitCombatOutcome;
    case e::no_change:
      break;
    case e::destroyed: {
      auto&  o = outcome.get<destroyed>();
      Tribe& tribe =
          ss.natives.tribe_for( tribe_for_unit( ss, unit ) );
      if( o.tribe_retains_horses && o.tribe_retains_muskets )
        tribe.horses += 50;
      if( o.tribe_retains_muskets ) tribe.muskets += 50;
      ss.units.destroy_unit( unit.id );
      break;
    }
    case e::promoted: {
      auto&       o                  = outcome.get<promoted>();
      static auto promotion_messages = [] {
        using E = e_native_unit_type;
        refl::enum_map<E, refl::enum_map<E, maybe<string>>> res;
        res[E::brave][E::armed_brave] =
            "@[H]Muskets@[] acquired by brave!";
        res[E::brave][E::mounted_brave] =
            "@[H]Horses@[] acquired by @[H]{}@[] brave!";
        res[E::armed_brave][E::mounted_warrior] =
            "@[H]Muskets@[] acquired by @[H]{}@[] mounted "
            "brave!";
        res[E::mounted_brave][E::armed_brave] =
            "@[H]Horses@[] acquired by @[H]{}@[] armed brave!";
        return res;
      }();

      msg       = promotion_messages[unit.type][o.to];
      unit.type = o.to;
      break;
    }
  }

  if( should_message && msg.has_value() )
    co_await ts.gui.message_box( *msg );
}

wait<> perform_naval_unit_combat_outcome(
    SS& ss, TS& ts, Player const& player, Unit& unit,
    EuroNavalUnitCombatOutcome_t const& outcome,
    UnitId                              opponent_id ) {
  switch( outcome.to_enum() ) {
    using namespace EuroNavalUnitCombatOutcome;
    case e::no_change:
      break;
    case e::moved: {
      auto& o = outcome.get<moved>();
      auto  unit_deleted =
          co_await unit_to_map_square( ss, ts, unit.id(), o.to );
      CHECK( !unit_deleted.has_value() );
      break;
    }
    case e::damaged:
      break;
    case e::sunk: {
      // This should always exist because, if we are here, then
      // this ship has been sunk, which means that the opponent
      // ship should not have been sunk and thus should exist.
      CHECK( ss.units.exists( opponent_id ) );
      Unit const& opponent = ss.units.unit_for( opponent_id );
      int const   num_units_lost =
          unit.cargo().items_of_type<Cargo::unit>().size();
      lg.info( "ship sunk: {} units onboard lost.",
               num_units_lost );
      string msg =
          fmt::format( "{} @[H]{}@[] sunk by @[H]{}@[] {}",
                       nation_obj( unit.nation() ).adjective,
                       unit.desc().name,
                       nation_obj( opponent.nation() ).adjective,
                       opponent.desc().name );
      if( num_units_lost == 1 )
        msg += fmt::format(
            ", @[H]1@[] unit onboard has been lost" );
      else if( num_units_lost > 1 )
        msg += fmt::format(
            ", @[H]{}@[] units onboard have been lost",
            num_units_lost );
      msg += '.';
      // Need to destroy unit first before displaying message
      // otherwise the unit will reappear on the map while the
      // message is open.
      ss.units.destroy_unit( unit.id() );
      co_await ts.gui.message_box( msg );
      break;
    }
  }
}

e_direction direction_of_attack( SSConst const& ss,
                                 auto           attacker_id,
                                 auto           defender_id ) {
  Coord const attacker_coord =
      coord_for_unit_multi_ownership_or_die( ss, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( d,
                attacker_coord.direction_to( defender_coord ) );
  return d;
}

} // namespace

/****************************************************************
** AttackHandlerBase
*****************************************************************/
struct AttackHandlerBase : public OrdersHandler {
  AttackHandlerBase( Planes& planes, SS& ss, TS& ts,
                     Player& player, UnitId attacker_id,
                     e_direction direction );

  // Implement OrdersHandler.
  wait<bool> confirm() override;

  // Implement OrdersHandler.
  wait<> animate() const override;

  // Implement OrdersHandler.
  wait<> perform() override;

 protected:
  Planes& planes_;
  SS&     ss_;
  TS&     ts_;
  Player& active_player_;

  // The unit doing the attacking.
  UnitId  attacker_id_;
  Unit&   attacker_;
  Player& attacking_player_;

  e_direction direction_;

  // The square on which the unit resides.
  Coord attack_src_{};

  // The square toward which the attack is aimed; this is the
  // same as the square of the unit being attacked.
  Coord attack_dst_{};
};

AttackHandlerBase::AttackHandlerBase( Planes& planes, SS& ss,
                                      TS& ts, Player& player,
                                      UnitId      attacker_id,
                                      e_direction direction )
  : planes_( planes ),
    ss_( ss ),
    ts_( ts ),
    active_player_( player ),
    attacker_id_( attacker_id ),
    attacker_( ss.units.unit_for( attacker_id ) ),
    attacking_player_( player_for_nation_or_die(
        ss.players, attacker_.nation() ) ),
    direction_( direction ) {
  attack_src_ =
      coord_for_unit_indirect_or_die( ss_.units, attacker_id_ );
  attack_dst_ = attack_src_.moved( direction_ );

  CHECK( attack_src_ != attack_dst_ );
  CHECK( attack_src_.is_adjacent_to( attack_dst_ ) );
}

wait<bool> AttackHandlerBase::confirm() {
  if( maybe<e_attack_verdict_base> const verdict =
          co_await check_attack_verdict_base(
              ss_, ts_, attacker_, direction_ );
      verdict.has_value() ) {
    co_await display_base_verdict_msg( ts_.gui, *verdict );
    co_return false;
  }

  co_return true;
}

wait<> AttackHandlerBase::animate() const { co_return; }

wait<> AttackHandlerBase::perform() {
  CHECK( !attacker_.mv_pts_exhausted() );
  CHECK( attacker_.orders() == e_unit_orders::none );
  // The original game seems to consume all movement points of a
  // unit when attacking.
  attacker_.forfeight_mv_points();
  co_return;
}

/****************************************************************
** EuroAttackHandlerBase
*****************************************************************/
// For when the defender is also a european unit.
struct EuroAttackHandlerBase : public AttackHandlerBase {
  using Base = AttackHandlerBase;

  EuroAttackHandlerBase( Planes& planes, SS& ss, TS& ts,
                         Player& player, UnitId attacker_id,
                         UnitId defender_id );

 protected:
  UnitId  defender_id_;
  Unit&   defender_;
  Player& defending_player_;
};

EuroAttackHandlerBase::EuroAttackHandlerBase(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, UnitId defender_id )
  : AttackHandlerBase(
        planes, ss, ts, player, attacker_id,
        direction_of_attack( ss, attacker_id, defender_id ) ),
    defender_id_( defender_id ),
    defender_( ss.units.unit_for( defender_id ) ),
    defending_player_( player_for_nation_or_die(
        ss.players, defender_.nation() ) ) {
  CHECK( defender_id_ != attacker_id_ );
}

/****************************************************************
** NativeAttackHandlerBase
*****************************************************************/
// For when the defender is a native unit.
struct NativeAttackHandlerBase : public AttackHandlerBase {
  using Base = AttackHandlerBase;

  NativeAttackHandlerBase( Planes& planes, SS& ss, TS& ts,
                           Player& player, UnitId attacker_id,
                           NativeUnitId defender_id );

 protected:
  NativeUnitId defender_id_;
  NativeUnit&  defender_;
  Tribe&       defender_tribe_;
};

NativeAttackHandlerBase::NativeAttackHandlerBase(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, NativeUnitId defender_id )
  : AttackHandlerBase(
        planes, ss, ts, player, attacker_id,
        direction_of_attack( ss, attacker_id, defender_id ) ),
    defender_id_( defender_id ),
    defender_( ss.units.unit_for( defender_id ) ),
    defender_tribe_( ss.natives.tribe_for(
        tribe_for_unit( ss, defender_ ) ) ) {}

/****************************************************************
** AttackColonyUndefendedHandler
*****************************************************************/
struct AttackColonyUndefendedHandler
  : public EuroAttackHandlerBase {
  using Base = EuroAttackHandlerBase;

  AttackColonyUndefendedHandler( Planes& planes, SS& ss, TS& ts,
                                 Player& player,
                                 UnitId  attacker_id,
                                 UnitId  defender_id,
                                 Colony& colony );

  // Implement OrdersHandler.
  wait<bool> confirm() override;

  // Implement OrdersHandler.
  wait<> perform() override;

 private:
  Colony&                          colony_;
  CombatEuroAttackUndefendedColony combat_;
};

unique_ptr<OrdersHandler> attack_colony_undefended_handler(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, UnitId defender_id, Colony& colony ) {
  return make_unique<AttackColonyUndefendedHandler>(
      planes, ss, ts, player, attacker_id, defender_id, colony );
}

AttackColonyUndefendedHandler::AttackColonyUndefendedHandler(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, UnitId defender_id, Colony& colony )
  : EuroAttackHandlerBase( planes, ss, ts, player, attacker_id,
                           defender_id ),
    colony_( colony ) {}

wait<bool> AttackColonyUndefendedHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;

  combat_ = ts_.combat.euro_attack_undefended_colony(
      attacker_, defender_, colony_ );
  co_return true;
}

// For this sequence the animations need to be interleaved with
// actions, so the entire thing is done in the perform() method.
// In particular, the attacking unit, if it wins and gets pro-
// moted, needs to be promoted before it slides into the colony.
wait<> AttackColonyUndefendedHandler::perform() {
  co_await Base::perform();

  // Animate the attack part of it. If the colony is captured
  // then the remainder will be done further below.
  co_await planes_.land_view().animate(
      anim_seq_for_undefended_colony( ss_, combat_ ) );

  co_await perform_euro_unit_combat_outcome(
      ss_, ts_, active_player_, attacking_player_, attacker_,
      combat_.attacker.outcome );

  if( combat_.winner == e_combat_winner::defender )
    // return since in this case the attacker lost, so nothing
    // special happens; we just do what we normally do when an
    // attacker loses a battle.
    co_return;

  // The colony has been captured.

  // 1. The attacker moves into the colony square.
  co_await planes_.land_view().animate(
      anim_seq_for_unit_move( attacker_.id(), direction_ ) );
  maybe<UnitDeleted> unit_deleted = co_await unit_to_map_square(
      ss_, ts_, attacker_.id(), attack_dst_ );
  CHECK( !unit_deleted.has_value() );

  // 2. All ships in the colony's port are considered damaged and
  // are sent for repair. In the OG this seems to happen with
  // 100% probability to all ships in the colony's port. Contrary
  // to how it might first seem, this is actually a huge boon to
  // the player whose colony was captured, since it means that
  // they don't lose any ships (which are very valuable) when
  // their colonies are captured; they just have to wait a few
  // turns for them to be repaired. If ships were allowed to be
  // taken upon colony capture then that could enable an
  // early-game strategy where you put some military next to a
  // rival's initial colony and wait for them to move their ship
  // into the colony, then conquer it. That losing player would
  // then probably not be able to recover from that.
  // TODO

  // 3. Any veteran_colonists in the colony must have their vet-
  // eran status stripped.
  // TODO

  // 4. Compute gold plundered.  The OG using this formula:
  //
  //      plundered = G*(CP/TP)
  //
  //    where G is the total gold of the nation whose colony was
  //    captured, CP is the colony population, and TP is the
  //    total population of all colonies in that nation.
  // TODO

  // 5. The colony changes ownership, as well as all of the units
  // that are working in it and who are on the map at the colony
  // location.
  change_colony_nation( colony_, ss_.units, attacker_.nation() );

  // 6. Announce capture.
  // TODO: add an interface method to IGui for playing music.
  // conductor::play_request(
  //     ts_.rand, conductor::e_request::fife_drum_happy,
  //     conductor::e_request_probability::always );
  co_await ts_.gui.message_box(
      "The @[H]{}@[] have captured the colony of @[H]{}@[]!",
      nation_obj( attacker_.nation() ).display_name,
      colony_.name );

  // 4. Open colony view.
  e_colony_abandoned const abandoned =
      co_await show_colony_view( planes_, ss_, ts_, colony_ );
  if( abandoned == e_colony_abandoned::yes )
    // Nothing really special to do here.
    co_return;

  // TODO: what if there are trade routes that involve this
  // colony?
  co_return;
}

/****************************************************************
** NavalBattleHandler
*****************************************************************/
struct NavalBattleHandler : public EuroAttackHandlerBase {
  using Base = EuroAttackHandlerBase;

  using Base::Base;

  // Implement OrdersHandler.
  wait<bool> confirm() override;

  // Implement OrdersHandler.
  wait<> animate() const override;

  // Implement OrdersHandler.
  wait<> perform() override;

 private:
  CombatShipAttackShip combat_;
};

unique_ptr<OrdersHandler> naval_battle_handler(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, UnitId defender_id ) {
  return make_unique<NavalBattleHandler>(
      planes, ss, ts, player, attacker_id, defender_id );
}

wait<bool> NavalBattleHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;
  TODO(
      "implement naval battle combat mechanics them implement "
      "unit test case." );
  // combat_ = ts_.combat.ship_attack_ship( attacker_, defender_
  // );

  // co_return true;
}

wait<> NavalBattleHandler::animate() const {
  co_await Base::animate();
  co_await planes_.land_view().animate(
      anim_seq_for_naval_battle( ss_, combat_ ) );
}

wait<> NavalBattleHandler::perform() {
  co_await Base::perform();
  co_await perform_naval_unit_combat_outcome(
      ss_, ts_, attacking_player_, attacker_,
      combat_.attacker.outcome, defender_id_ );
  co_await perform_naval_unit_combat_outcome(
      ss_, ts_, defending_player_, defender_,
      combat_.defender.outcome, attacker_id_ );
}

/****************************************************************
** EuroAttackHandler
*****************************************************************/
struct EuroAttackHandler : public EuroAttackHandlerBase {
  using Base = EuroAttackHandlerBase;

  using Base::Base;

  // Implement OrdersHandler.
  wait<bool> confirm() override;

  // Implement OrdersHandler.
  wait<> animate() const override;

  // Implement OrdersHandler.
  wait<> perform() override;

 private:
  CombatEuroAttackEuro combat_ = {};
};

unique_ptr<OrdersHandler> attack_euro_land_handler(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, UnitId defender_id ) {
  return make_unique<EuroAttackHandler>(
      planes, ss, ts, player, attacker_id, defender_id );
}

wait<bool> EuroAttackHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;
  combat_ = ts_.combat.euro_attack_euro( attacker_, defender_ );
  co_return true;
}

wait<> EuroAttackHandler::animate() const {
  co_await Base::animate();
  AnimationSequence const seq =
      anim_seq_for_attack_euro( ss_, combat_ );
  co_await planes_.land_view().animate( seq );
}

wait<> EuroAttackHandler::perform() {
  co_await Base::perform();
  co_await perform_euro_unit_combat_outcome(
      ss_, ts_, active_player_, attacking_player_, attacker_,
      combat_.attacker.outcome );
  co_await perform_euro_unit_combat_outcome(
      ss_, ts_, active_player_, defending_player_, defender_,
      combat_.defender.outcome );
}

/****************************************************************
** AttackNativeUnitHandler
*****************************************************************/
struct AttackNativeUnitHandler : public NativeAttackHandlerBase {
  using Base = NativeAttackHandlerBase;

  using Base::Base;

  AttackNativeUnitHandler( Planes& planes, SS& ss, TS& ts,
                           Player& player, UnitId attacker_id,
                           NativeUnitId defender_id,
                           e_direction  direction );

  // Implement OrdersHandler.
  wait<bool> confirm() override;

  // Implement OrdersHandler.
  wait<> animate() const override;

  // Implement OrdersHandler.
  wait<> perform() override;

 private:
  CombatEuroAttackBrave combat_;
};

unique_ptr<OrdersHandler> attack_native_unit_handler(
    Planes& planes, SS& ss, TS& ts, Player& player,
    UnitId attacker_id, NativeUnitId defender_id ) {
  return make_unique<AttackNativeUnitHandler>(
      planes, ss, ts, player, attacker_id, defender_id );
}

// Returns true if the move is allowed.
wait<bool> AttackNativeUnitHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;

  UNWRAP_CHECK(
      relationship,
      defender_tribe_.relationship[attacker_.nation()] );
  if( !relationship.nation_has_attacked_tribe ) {
    YesNoConfig const config{
        .msg = fmt::format(
            "Shall we attack the @[H]{}@[]?",
            ts_.gui.identifier_to_display_name(
                fmt::to_string( defender_tribe_.type ) ) ),
        .yes_label      = "Attack",
        .no_label       = "Cancel",
        .no_comes_first = true };
    maybe<ui::e_confirm> const proceed =
        co_await ts_.gui.optional_yes_no( config );
    if( proceed != ui::e_confirm::yes ) co_return false;
    relationship.nation_has_attacked_tribe = true;
  }

  combat_ = ts_.combat.euro_attack_brave( attacker_, defender_ );
  co_return true;
}

wait<> AttackNativeUnitHandler::animate() const {
  co_await Base::animate();
  AnimationSequence const seq =
      anim_seq_for_attack_brave( ss_, combat_ );
  co_await planes_.land_view().animate( seq );
}

wait<> AttackNativeUnitHandler::perform() {
  co_await Base::perform();

  // The tribal alarm goes up regardless of the battle outcome.
  UNWRAP_CHECK(
      relationship,
      defender_tribe_.relationship[attacker_.nation()] );
  increase_tribal_alarm_from_attacking_brave(
      attacking_player_,
      ss_.natives.dwelling_for(
          ss_.units.dwelling_for( defender_id_ ) ),
      relationship );

  co_await perform_euro_unit_combat_outcome(
      ss_, ts_, active_player_, attacking_player_, attacker_,
      combat_.attacker.outcome );
  bool const should_message =
      ( attacking_player_.nation == active_player_.nation &&
        active_player_.human );
  co_await perform_native_unit_combat_outcome(
      ss_, ts_, defender_, combat_.defender.outcome,
      should_message );
}

} // namespace rn
