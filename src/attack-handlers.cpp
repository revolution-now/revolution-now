/****************************************************************
**attack-handlers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-01.
*
* Description: Command handlers for attacking.
*
*****************************************************************/
#include "attack-handlers.hpp"

// Revolution Now
#include "alarm.hpp"
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "colonies.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "command.hpp"
#include "commodity.hpp"
#include "conductor.hpp"
#include "damaged.hpp"
#include "harbor-units.hpp"
#include "icombat.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "missionary.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "road.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"

// config
#include "config/nation.hpp"
#include "config/natives.rds.hpp"
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
#include "base/conv.hpp"
#include "base/string.hpp"
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
          e_surface::water &&
      !attacker.desc().ship )
    co_return e_attack_verdict_base::land_unit_attack_ship;

  if( attacker.movement_points() < 1 ) {
    if( co_await ts.gui.optional_yes_no(
            { .msg = fmt::format(
                  "This unit only has [{}] movement points "
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

maybe<string> perform_euro_unit_combat_outcome(
    SS& ss, TS& ts, Player const& active_player,
    Player const& player, Unit& unit,
    EuroUnitCombatOutcome_t const& outcome ) {
  auto capture_unit = [&]( e_nation new_nation,
                           Coord    new_coord ) {
    unit.change_nation( ss.units, new_nation );
    unit_to_map_square_non_interactive( ss, ts, unit.id(),
                                        new_coord );
    // This is so that the captured unit won't ask for orders
    // in the same turn that it is captured.
    unit.forfeight_mv_points();
    unit.clear_orders();
  };

  maybe<string> msg;

  switch( outcome.to_enum() ) {
    using e = EuroUnitCombatOutcome::e;
    case e::no_change:
      break;
    case e::destroyed: {
      // This will be scouts, pioneers, missionaries, and ar-
      // tillery.
      msg = fmt::format( "{} [{}] has been lost in battle!",
                         nation_obj( unit.nation() ).adjective,
                         unit.desc().name );
      // Need to destroy the unit after accessing its info.
      ss.units.destroy_unit( unit.id() );
      break;
    }
    case e::captured: {
      auto& o = outcome.get<EuroUnitCombatOutcome::captured>();
      capture_unit( o.new_nation, o.new_coord );
      break;
    }
    case e::captured_and_demoted: {
      auto& o = outcome.get<
          EuroUnitCombatOutcome::captured_and_demoted>();
      // Need to do this first before demoting the unit.
      msg = "Unit demoted upon capture!";
      if( unit.type() == e_unit_type::veteran_colonist )
        // FIXME: this message needs to be displayed to the
        // player that is capturing the unit. Perhaps below when
        // we test the player's nation, instead of testing the
        // `player` that is passed in, we re-measure the unit's
        // nation and use that, that way in the case of a capture
        // it will use the new nation, which should probably do
        // the right thing (but only do that if the unit hasn't
        // been destroyed).
        msg = "Veteran status lost upon capture!";
      unit.change_type( player, o.to );
      capture_unit( o.new_nation, o.new_coord );
      break;
    }
    case e::promoted: {
      auto& o = outcome.get<EuroUnitCombatOutcome::promoted>();
      // TODO: make this message more specific like in the OG.
      msg = "Unit promoted for valor in combat!";
      unit.change_type( player, o.to );
      break;
    }
    case e::demoted: {
      auto& o = outcome.get<EuroUnitCombatOutcome::demoted>();
      unit.change_type( player, o.to );
      break;
    }
  }

  if( player.nation == active_player.nation &&
      active_player.human )
    // FIXME: not sure yet how we're going to do this GUI han-
    // dling with AI players. For now we just say that we will
    // only pop up a message box notifying the player of battle
    // outcomes for their unit if the active player owns the unit
    // and that active player is a human.
    return msg;
  return nothing;
}

maybe<string> perform_native_unit_combat_outcome(
    SS& ss, Player const& active_player, NativeUnit& unit,
    NativeUnitCombatOutcome_t const& outcome ) {
  maybe<string> msg;

  switch( outcome.to_enum() ) {
    using e = NativeUnitCombatOutcome::e;
    case e::no_change:
      break;
    case e::destroyed: {
      auto& o =
          outcome.get<NativeUnitCombatOutcome::destroyed>();
      Tribe& tribe =
          ss.natives.tribe_for( tribe_for_unit( ss, unit ) );
      if( o.tribe_retains_horses && o.tribe_retains_muskets )
        tribe.horses += 50;
      if( o.tribe_retains_muskets ) tribe.muskets += 50;
      ss.units.destroy_unit( unit.id );
      break;
    }
    case e::promoted: {
      auto& o = outcome.get<NativeUnitCombatOutcome::promoted>();
      static auto promotion_messages = [] {
        using E = e_native_unit_type;
        refl::enum_map<E, refl::enum_map<E, maybe<string>>> res;
        res[E::brave][E::armed_brave] =
            "[Muskets] acquired by brave!";
        res[E::brave][E::mounted_brave] =
            "[Horses] acquired by [{}] brave!";
        res[E::armed_brave][E::mounted_warrior] =
            "[Muskets] acquired by [{}] mounted "
            "brave!";
        res[E::mounted_brave][E::armed_brave] =
            "[Horses] acquired by [{}] armed brave!";
        return res;
      }();

      msg       = promotion_messages[unit.type][o.to];
      unit.type = o.to;
      break;
    }
  }

  // FIXME
  if( active_player.human ) return msg;
  return nothing;
}

maybe<string> perform_naval_unit_combat_outcome(
    SS& ss, TS& ts, Player const& active_player, Player& player,
    Unit& unit, EuroNavalUnitCombatOutcome_t const& outcome,
    UnitId opponent_id ) {
  maybe<string> msg;

  auto add_units_lost = [&]( int count ) {
    CHECK( msg.has_value() );
    if( count == 1 )
      *msg +=
          fmt::format( " [One] unit onboard has been lost." );
    else if( count > 1 )
      *msg += fmt::format(
          " [{}] units onboard have been lost.",
          base::capitalize_initials(
              base::int_to_string_literary( count ) ) );
  };

  switch( outcome.to_enum() ) {
    using e = EuroNavalUnitCombatOutcome::e;
    case e::no_change:
      break;
    case e::moved: {
      auto& o = outcome.get<EuroNavalUnitCombatOutcome::moved>();
      // FIXME: This reallyl should be run interactively because
      // e.g. the ship might discover the pacific ocean or an-
      // other nation upon moving. Once we figure out how to deal
      // with multiple european players then this needs to be im-
      // proved.
      unit_to_map_square_non_interactive( ss, ts, unit.id(),
                                          o.to );
      break;
    }
    case e::damaged: {
      auto& o =
          outcome.get<EuroNavalUnitCombatOutcome::damaged>();
      // This means that the unit is being marked as damaged and
      // has been damaged for zero turns as of now. Note that
      // this automatically removes the unit from any sentry/for-
      // tified status that it had, which is what we want.
      int const turns_until_repair =
          repair_turn_count_for_unit( o.port, unit.type() );
      if( turns_until_repair > 0 )
        unit.orders() = unit_orders::damaged{
            .turns_until_repair = turns_until_repair };
      else
        // In the OG a Caravel that is sent to the drydock will
        // have a repair turn requirement of 0, and so it is im-
        // mediately activated and can still move in the same
        // turn if it has movement points left. That said, it
        // still needs to be transported to the colony and any
        // units on it need to be destroyed as usual, so even in
        // this case we need to carry out the rest of the actions
        // in this function.
        unit.clear_orders();
      // All units in cargo are destroyed.
      vector<UnitId> const units_in_cargo = unit.cargo().units();
      int const num_units_lost = units_in_cargo.size();
      for( UnitId const held_id : units_in_cargo )
        ss.units.destroy_unit( held_id );
      // Now send the ship for repair.
      msg = fmt::format( "{} [{}] damaged in battle!",
                         nation_obj( unit.nation() ).adjective,
                         unit.desc().name );
      switch( o.port.to_enum() ) {
        case ShipRepairPort::e::colony: {
          ColonyId const colony_id =
              o.port.get<ShipRepairPort::colony>().id;
          Colony const& colony =
              ss.colonies.colony_for( colony_id );
          // This can be non-interactive because there is already
          // a colony on the square, and the ship is damaged, so
          // it shouldn't really trigger anything interactive
          // when we move it into the colony.
          unit_to_map_square_non_interactive( ss, ts, unit.id(),
                                              colony.location );
          *msg += fmt::format( " Ship sent to [{}] for repair.",
                               colony.name );
          break;
        }
        case ShipRepairPort::e::european_harbor: {
          unit_move_to_port( ss.units, player, unit.id() );
          *msg += fmt::format(
              " Ship sent to [{}] for repair.",
              nation_obj( unit.nation() ).harbor_city_name );
          break;
        }
      }
      add_units_lost( num_units_lost );
      break;
    }
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
      msg =
          fmt::format( "{} [{}] sunk by [{}] {}.",
                       nation_obj( unit.nation() ).adjective,
                       unit.desc().name,
                       nation_obj( opponent.nation() ).adjective,
                       opponent.desc().name );
      add_units_lost( num_units_lost );
      // Need to destroy unit first before displaying message
      // otherwise the unit will reappear on the map while the
      // message is open.
      ss.units.destroy_unit( unit.id() );
      break;
    }
  }

  // FIXME: this is a mess: some parts of the messages above
  // should be shown to the human player regardless of whether it
  // concerns their ship or not (e.g., information as to whether
  // a ship sunk or not), but other info (such as how many units
  // lost) should only be conveyed for the player's units. We
  // need to figure out a better way to handle UI interactions
  // throughout this module in situations with units from mul-
  // tiple players and messages/prompts concerning them.
  if( player.nation == active_player.nation &&
      active_player.human )
    return msg;
  return nothing;
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

e_direction direction_of_attack( SSConst const& ss,
                                 UnitId         attacker_id,
                                 DwellingId     dwelling_id ) {
  Coord const attacker_coord =
      coord_for_unit_multi_ownership_or_die( ss, attacker_id );
  Coord const defender_coord =
      ss.natives.coord_for( dwelling_id );
  UNWRAP_CHECK( d,
                attacker_coord.direction_to( defender_coord ) );
  return d;
}

wait<> show_outcome_messages( TS&                  ts,
                              maybe<string> const& attacker,
                              maybe<string> const& defender ) {
  if( attacker.has_value() )
    co_await ts.gui.message_box( *attacker );
  if( defender.has_value() )
    co_await ts.gui.message_box( *defender );
}

} // namespace

/****************************************************************
** AttackHandlerBase
*****************************************************************/
struct AttackHandlerBase : public CommandHandler {
  AttackHandlerBase( SS& ss, TS& ts, Player& player,
                     UnitId attacker_id, e_direction direction );

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> animate() const override;

  // Implement CommandHandler.
  wait<> perform() override;

 protected:
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

AttackHandlerBase::AttackHandlerBase( SS& ss, TS& ts,
                                      Player&     player,
                                      UnitId      attacker_id,
                                      e_direction direction )
  : ss_( ss ),
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
  CHECK( attacker_.orders().holds<unit_orders::none>() );
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

  EuroAttackHandlerBase( SS& ss, TS& ts, Player& player,
                         UnitId attacker_id,
                         UnitId defender_id );

 protected:
  UnitId  defender_id_;
  Unit&   defender_;
  Player& defending_player_;
};

EuroAttackHandlerBase::EuroAttackHandlerBase(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id )
  : AttackHandlerBase(
        ss, ts, player, attacker_id,
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

  NativeAttackHandlerBase( SS& ss, TS& ts, Player& player,
                           UnitId       attacker_id,
                           NativeUnitId defender_id );

 protected:
  NativeUnitId defender_id_;
  NativeUnit&  defender_;
  Tribe&       defender_tribe_;
};

NativeAttackHandlerBase::NativeAttackHandlerBase(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    NativeUnitId defender_id )
  : AttackHandlerBase(
        ss, ts, player, attacker_id,
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

  AttackColonyUndefendedHandler( SS& ss, TS& ts, Player& player,
                                 UnitId  attacker_id,
                                 UnitId  defender_id,
                                 Colony& colony );

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> perform() override;

 private:
  Colony&                          colony_;
  CombatEuroAttackUndefendedColony combat_;
};

unique_ptr<CommandHandler> attack_colony_undefended_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id, Colony& colony ) {
  return make_unique<AttackColonyUndefendedHandler>(
      ss, ts, player, attacker_id, defender_id, colony );
}

AttackColonyUndefendedHandler::AttackColonyUndefendedHandler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id, Colony& colony )
  : EuroAttackHandlerBase( ss, ts, player, attacker_id,
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
  co_await ts_.planes.land_view().animate(
      anim_seq_for_undefended_colony( ss_, combat_ ) );

  maybe<string> const attacker_msg =
      perform_euro_unit_combat_outcome(
          ss_, ts_, active_player_, attacking_player_, attacker_,
          combat_.attacker.outcome );
  co_await show_outcome_messages( ts_, attacker_msg, nothing );

  if( combat_.winner == e_combat_winner::defender )
    // return since in this case the attacker lost, so nothing
    // special happens; we just do what we normally do when an
    // attacker loses a battle.
    co_return;

  // The colony has been captured.

  // 1. The attacker moves into the colony square.
  co_await ts_.planes.land_view().animate(
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
      "The [{}] have captured the colony of [{}]!",
      nation_obj( attacker_.nation() ).display_name,
      colony_.name );

  // 4. Open colony view.
  e_colony_abandoned const abandoned =
      co_await ts_.colony_viewer.show( ts_, colony_.id );
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

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> animate() const override;

  // Implement CommandHandler.
  wait<> perform() override;

 private:
  CombatShipAttackShip combat_;
};

unique_ptr<CommandHandler> naval_battle_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id ) {
  return make_unique<NavalBattleHandler>(
      ss, ts, player, attacker_id, defender_id );
}

wait<bool> NavalBattleHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;
  combat_ = ts_.combat.ship_attack_ship( attacker_, defender_ );
  co_return true;
}

wait<> NavalBattleHandler::animate() const {
  co_await Base::animate();
  co_await ts_.planes.land_view().animate(
      anim_seq_for_naval_battle( ss_, combat_ ) );
}

wait<> NavalBattleHandler::perform() {
  co_await Base::perform();

  if( !combat_.winner.has_value() ) {
    // Defender evaded.
    co_await ts_.gui.message_box(
        "{} [{}] evades {} [{}].",
        nation_obj( defender_.nation() ).adjective,
        defender_.desc().name,
        nation_obj( attacker_.nation() ).adjective,
        attacker_.desc().name );
    // There shouldn't be anything else to do here; none of the
    // below should be relevant.
    co_return;
  }

  if( combat_.winner.has_value() ) {
    // One of the ships was either damaged or sunk.
    Unit const& loser =
        ( combat_.winner == e_combat_winner::attacker )
            ? defender_
            : attacker_;
    bool const has_commodity_cargo =
        ( loser.cargo().count_items_of_type<Cargo::commodity>() >
          0 );
    if( has_commodity_cargo ) {
      // At this point the losing ship has commodity cargo on it
      // that the attacker can potentially capture. Note that if
      // the ship has units in cargo then they will be destroyed,
      // but that is handled further below. This section is just
      // for commodity capture.

      // TODO: implementing this probably has to wait until we
      // have some kind of AI interface where we can ask the
      // player for which commodities they'd like to capture in a
      // way that will work for both human and AI players, since
      // the unit in question here (that is having its commodi-
      // ties stolen) could be an AI player. It could also be
      // that both players are human players, which would also
      // pose a challenge, but we probably will end up supporting
      // only one human player at a time, like the OG.
      //
      // So for now we will just clear the cargo out of the ship
      // if it has been damaged.
      for( auto [comm, slot] : loser.cargo().commodities() ) {
        Commodity const removed = rm_commodity_from_cargo(
            ss_.units, loser.id(), slot );
        CHECK_EQ( removed, comm );
      }
    }
  }

  maybe<string> const attacker_msg =
      perform_naval_unit_combat_outcome(
          ss_, ts_, active_player_, attacking_player_, attacker_,
          combat_.attacker.outcome, defender_id_ );
  maybe<string> const defender_msg =
      perform_naval_unit_combat_outcome(
          ss_, ts_, active_player_, defending_player_, defender_,
          combat_.defender.outcome, attacker_id_ );
  // Messages must be shown after effects are made for both par-
  // ties, otherwise they will temporarily appear to visually re-
  // vert while the message box is up.
  co_await show_outcome_messages( ts_, attacker_msg,
                                  defender_msg );

  // FIXME: hack. This is because the attacker may have moved to
  // the defender's square, but the above functions that perform
  // that movement don't do it interactively, so here we will
  // rerun it interactively just in case e.g. the ship discovers
  // the pacific ocean or another nation upon moving. Once we
  // figure out how to deal with multiple european players then
  // this needs to be improved.
  if( auto o = combat_.attacker.outcome
                   .get_if<EuroNavalUnitCombatOutcome::moved>();
      o.has_value() ) {
    maybe<UnitDeleted> const x = co_await unit_to_map_square(
        ss_, ts_, attacker_id_, o->to );
    CHECK( !x.has_value() );
  }
}

/****************************************************************
** EuroAttackHandler
*****************************************************************/
struct EuroAttackHandler : public EuroAttackHandlerBase {
  using Base = EuroAttackHandlerBase;

  using Base::Base;

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> animate() const override;

  // Implement CommandHandler.
  wait<> perform() override;

 private:
  CombatEuroAttackEuro combat_ = {};
};

unique_ptr<CommandHandler> attack_euro_land_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    UnitId defender_id ) {
  return make_unique<EuroAttackHandler>(
      ss, ts, player, attacker_id, defender_id );
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
  co_await ts_.planes.land_view().animate( seq );
}

wait<> EuroAttackHandler::perform() {
  co_await Base::perform();
  maybe<string> const attacker_msg =
      perform_euro_unit_combat_outcome(
          ss_, ts_, active_player_, attacking_player_, attacker_,
          combat_.attacker.outcome );
  maybe<string> const defender_msg =
      perform_euro_unit_combat_outcome(
          ss_, ts_, active_player_, defending_player_, defender_,
          combat_.defender.outcome );
  // Messages must be shown after effects are made for both par-
  // ties, otherwise they will temporarily appear to visually re-
  // vert while the message box is up.
  co_await show_outcome_messages( ts_, attacker_msg,
                                  defender_msg );
}

/****************************************************************
** AttackNativeUnitHandler
*****************************************************************/
struct AttackNativeUnitHandler : public NativeAttackHandlerBase {
  using Base = NativeAttackHandlerBase;

  using Base::Base;

  AttackNativeUnitHandler( SS& ss, TS& ts, Player& player,
                           UnitId       attacker_id,
                           NativeUnitId defender_id,
                           e_direction  direction );

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> animate() const override;

  // Implement CommandHandler.
  wait<> perform() override;

 private:
  CombatEuroAttackBrave combat_;
};

unique_ptr<CommandHandler> attack_native_unit_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    NativeUnitId defender_id ) {
  return make_unique<AttackNativeUnitHandler>(
      ss, ts, player, attacker_id, defender_id );
}

// Returns true if the move is allowed.
wait<bool> AttackNativeUnitHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;

  TribeRelationship& relationship =
      defender_tribe_.relationship[attacker_.nation()];
  if( !relationship.nation_has_attacked_tribe ) {
    YesNoConfig const config{
        .msg = fmt::format(
            "Shall we attack the [{}]?",
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
  co_await ts_.planes.land_view().animate( seq );
}

wait<> AttackNativeUnitHandler::perform() {
  co_await Base::perform();

  // The tribal alarm goes up regardless of the battle outcome.
  TribeRelationship& relationship =
      defender_tribe_.relationship[attacker_.nation()];
  increase_tribal_alarm_from_attacking_brave(
      attacking_player_,
      ss_.natives.dwelling_for(
          ss_.units.dwelling_for( defender_id_ ) ),
      relationship );

  maybe<string> const attacker_msg =
      perform_euro_unit_combat_outcome(
          ss_, ts_, active_player_, attacking_player_, attacker_,
          combat_.attacker.outcome );
  maybe<string> const brave_msg =
      perform_native_unit_combat_outcome(
          ss_, active_player_, defender_,
          combat_.defender.outcome );
  // Messages must be shown after effects are made for both par-
  // ties, otherwise they will temporarily appear to visually re-
  // vert while the message box is up.
  co_await show_outcome_messages( ts_, attacker_msg, brave_msg );
}

/****************************************************************
** AttackDwellingHandler
*****************************************************************/
struct AttackDwellingHandler : public AttackHandlerBase {
  using Base = AttackHandlerBase;

  AttackDwellingHandler( SS& ss, TS& ts, Player& player,
                         UnitId     attacker_id,
                         DwellingId defender_id );

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> perform() override;

  // Implement CommandHandler.
  vector<UnitId> units_to_prioritize() const override;

 private:
  NativeUnitId create_phantom_brave();

  wait<> produce_convert();

  using PhantomCombatAnimatorFunc =
      wait<>( CombatEuroAttackBrave const& combat );

  wait<> with_phantom_brave_combat(
      base::function_ref<PhantomCombatAnimatorFunc> func );

  DwellingId         dwelling_id_;
  Dwelling&          dwelling_;
  Tribe&             tribe_;
  TribeRelationship& relationship_;

  CombatEuroAttackDwelling combat_;

  maybe<UnitId> treasure_;
  maybe<UnitId> native_convert_;
};

unique_ptr<CommandHandler> attack_dwelling_handler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    DwellingId dwelling_id ) {
  return make_unique<AttackDwellingHandler>(
      ss, ts, player, attacker_id, dwelling_id );
}

AttackDwellingHandler::AttackDwellingHandler(
    SS& ss, TS& ts, Player& player, UnitId attacker_id,
    DwellingId dwelling_id )
  : AttackHandlerBase(
        ss, ts, player, attacker_id,
        direction_of_attack( ss, attacker_id, dwelling_id ) ),
    dwelling_id_( dwelling_id ),
    dwelling_( ss.natives.dwelling_for( dwelling_id ) ),
    tribe_( ss.natives.tribe_for( dwelling_.id ) ),
    relationship_( tribe_.relationship[player.nation] ) {}

// Returns true if the move is allowed.
wait<bool> AttackDwellingHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;
  combat_ =
      ts_.combat.euro_attack_dwelling( attacker_, dwelling_ );
  co_return true;
}

// Note that the returned unit should be destroyed when it is no
// longer needed.
NativeUnitId AttackDwellingHandler::create_phantom_brave() {
  // The strategy here is that we're going to create a "phantom"
  // (temporary) brave unit over top of the dwelling to act as
  // the target of the attack visually. We know that there won't
  // already be a brave there because otherwise the handling of
  // this battle would have been delegated to another handler.
  Coord const dwelling_location =
      ss_.natives.coord_for( dwelling_id_ );
  NativeUnitId const phantom_brave =
      create_unit_on_map_non_interactive(
          ss_, e_native_unit_type::brave, dwelling_location,
          dwelling_id_ );
  return phantom_brave;
}

// Note that the dwelling may no longer exist when this function
// is called.
wait<> AttackDwellingHandler::produce_convert() {
  Coord const attacker_coord =
      ss_.units.coord_for( attacker_id_ );
  Coord const dwelling_coord = attack_dst_;
  // Produce the convert on the dwelling tile, then we will ani-
  // mate it enpixelating and then sliding over to the attacker's
  // square. We can use the non-interactive version because the
  // convert is not actually going to be created on the dwelling
  // square; it will be created on the attacker's square, where
  // we know there is already a friendly unit, so no interactive
  // stuff need be done.
  UnitId const convert_id = create_unit_on_map_non_interactive(
      ss_, ts_, attacking_player_, e_unit_type::native_convert,
      dwelling_coord );
  native_convert_ = convert_id;
  string_view const tribe_name_adjective =
      config_natives.tribes[tribe_.type].name_adjective;
  string_view const nation_name_adjective =
      nation_obj( attacking_player_.nation ).adjective;
  // Pop up a message box and simultaneously make the convert ap-
  // pear and slide over to the attacker's square.
  //
  // The ordering of following is a bit subtle: Overall, we want
  // to wait for both the animations and the message box to fin-
  // ish, but as soon as the slide animation finishes we need to
  // move the convert to the attacker's square so that it doesn't
  // revert back while the user finishes reading the contents of
  // the message box.
  //
  // Then, as soon as the unit is moved, we initiate another ani-
  // mation to keep the unit on the front of the stack while the
  // message box is open. That will 1) allow the player to see
  // the unit that they're reading about in the message box, and
  // 2) will provide continuity because we are going to arrange
  // for the convert to ask for orders immediately after this.
  // Note that we use a non-background "front" animation which is
  // non-terminating, so we don't await on it, we just let it get
  // cancelled when the box closes.
  AnimationSequence const appear_and_slide_seq =
      anim_seq_for_convert_produced(
          convert_id, reverse_direction( direction_ ) );
  wait<> appear_and_slide_animation =
      ts_.planes.land_view().animate( appear_and_slide_seq );
  wait<> box = ts_.gui.message_box(
      "[{}] citizens frightened in combat rush to the "
      "[{} mission] as [converts]!",
      tribe_name_adjective, nation_name_adjective );
  co_await std::move( appear_and_slide_animation );
  // Non-interactive is OK here because the attacker is already
  // on this square.
  unit_to_map_square_non_interactive( ss_, ts_, convert_id,
                                      attacker_coord );
  AnimationSequence const front_seq =
      anim_seq_unit_to_front_non_background( convert_id );
  wait<> front_animation =
      ts_.planes.land_view().animate( front_seq );
  co_await std::move( box );
}

wait<> AttackDwellingHandler::with_phantom_brave_combat(
    base::function_ref<PhantomCombatAnimatorFunc> func ) {
  NativeUnitId const phantom_brave = create_phantom_brave();
  SCOPE_EXIT( ss_.units.destroy_unit( phantom_brave ) );
  CombatEuroAttackBrave phantom_combat{
      .winner   = combat_.winner,
      .attacker = combat_.attacker,
      .defender = { .id = phantom_brave } };
  if( combat_.winner == e_combat_winner::attacker )
    phantom_combat.defender.outcome =
        NativeUnitCombatOutcome::destroyed{};
  else
    phantom_combat.defender.outcome =
        NativeUnitCombatOutcome::no_change{};
  co_await func( phantom_combat );
}

wait<> AttackDwellingHandler::perform() {
  co_await Base::perform();

  auto const& tribe_conf = config_natives.tribes[tribe_.type];
  string_view const dwelling_label =
      dwelling_.is_capital
          ? "capital"sv
          : config_natives.dwelling_types[tribe_conf.level]
                .name_singular;
  string_view const tribe_name = tribe_conf.name_singular;
  string_view const nation_name =
      nation_obj( attacking_player_.nation ).display_name;
  string_view const nation_name_adjective =
      nation_obj( attacking_player_.nation ).adjective;
  string_view const nation_harbor_name =
      nation_obj( attacking_player_.nation ).harbor_city_name;

  // Set new tribal alarm.
  relationship_.tribal_alarm = combat_.new_tribal_alarm;

  // Consume attacker movement points.
  // Done in base handler.

  // Check if the tribe has burned our missions.
  if( combat_.missions_burned ) {
    // TODO: depixelation animation/sound?
    vector<UnitId> const missionaries =
        player_missionaries_in_tribe( ss_, attacking_player_,
                                      tribe_.type );
    for( UnitId const missionary : missionaries ) {
      CHECK( ss_.units.unit_for( missionary ).nation() ==
             attacking_player_.nation );
      ss_.units.destroy_unit( missionary );
    }
    co_await ts_.gui.message_box(
        "The [{}] revolt against [{}] missions! "
        "All {} missionaries eliminated!",
        tribe_name, nation_name_adjective,
        nation_name_adjective );
  }

  // Attacker lost:
  if( combat_.winner == e_combat_winner::defender ) {
    CHECK( combat_.defender.outcome
               .holds<DwellingCombatOutcome::no_change>() );
    co_await with_phantom_brave_combat(
        [&]( CombatEuroAttackBrave const& combat ) -> wait<> {
          AnimationSequence const seq =
              anim_seq_for_attack_brave( ss_, combat );
          co_await ts_.planes.land_view().animate( seq );
        } );
    maybe<string> const attacker_msg =
        perform_euro_unit_combat_outcome(
            ss_, ts_, active_player_, attacking_player_,
            attacker_, combat_.attacker.outcome );
    co_await show_outcome_messages( ts_, attacker_msg, nothing );
    co_return;
  }

  // Attacker won.

  // Population decrease.
  if( auto population_decrease =
          combat_.defender.outcome.get_if<
              DwellingCombatOutcome::population_decrease>();
      population_decrease.has_value() ) {
    co_await with_phantom_brave_combat(
        [&]( CombatEuroAttackBrave const& combat ) -> wait<> {
          AnimationSequence const seq =
              anim_seq_for_attack_brave( ss_, combat );
          co_await ts_.planes.land_view().animate( seq );
        } );
    maybe<string> const attacker_msg =
        perform_euro_unit_combat_outcome(
            ss_, ts_, active_player_, attacking_player_,
            attacker_, combat_.attacker.outcome );
    co_await show_outcome_messages( ts_, attacker_msg, nothing );
    --dwelling_.population;
    CHECK_GT( dwelling_.population, 0 );
    if( population_decrease->convert_produced )
      co_await produce_convert();
    co_return;
  }

  // Dwelling burned.
  UNWRAP_CHECK(
      destruction,
      combat_.defender.outcome
          .get_if<DwellingCombatOutcome::destruction>() );
  Coord const dwelling_location =
      ss_.natives.coord_for( dwelling_id_ );

  // Inc villages burned.
  ++attacking_player_.score_stats.dwellings_burned;

  // Clear road. When we eventually delete the dwelling this will
  // be done, but here we want to do it earlier, before the ani-
  // mations, otherwise the dwelling will depixelate showing the
  // road, then the road will suddenly disappear, which doesn't
  // look good.
  clear_road( ts_.map_updater, dwelling_location );

  // If missionary needs releasing, release it under the
  // dwelling. It should continue to be hidden until the dwelling
  // starts depixelating, then it should gradually become visi-
  // ble.
  maybe<UnitId> const missionary_in_dwelling =
      ss_.units.missionary_from_dwelling( dwelling_id_ );
  if( destruction.missionary_to_release.has_value() ) {
    CHECK( missionary_in_dwelling.has_value() );
    CHECK_EQ(
        ss_.units.unit_for( *missionary_in_dwelling ).nation(),
        attacking_player_.nation );
    maybe<UnitDeleted> deleted = co_await unit_to_map_square(
        ss_, ts_, *destruction.missionary_to_release,
        dwelling_location );
    CHECK( !deleted.has_value() );
  }

  // Animate attacker winning w/ burning village and depixelating
  // all braves.
  co_await with_phantom_brave_combat(
      [&]( CombatEuroAttackBrave const& combat ) -> wait<> {
        AnimationSequence const seq = anim_seq_for_dwelling_burn(
            ss_, attacker_id_, combat_.attacker.outcome,
            combat.defender.id, dwelling_id_,
            combat_.defender.outcome );
        co_await ts_.planes.land_view().animate( seq );
      } );

  // Kill dwelling, free braves owned by the dwelling, and any
  // owned land of the dwelling. This will also remove the road
  // under the dwelling.
  bool const was_capital = dwelling_.is_capital;
  destroy_dwelling( ss_, ts_, dwelling_id_ );
  maybe<string> const attacker_msg =
      perform_euro_unit_combat_outcome(
          ss_, ts_, active_player_, attacking_player_, attacker_,
          combat_.attacker.outcome );
  co_await show_outcome_messages( ts_, attacker_msg, nothing );

  // Check if convert produced.
  if( destruction.convert_produced ) co_await produce_convert();

  // Show message saying that dwelling was destroyed, and that
  // missionary fled if applicable. Also, if there is a treasure,
  // include the amount.
  string msg =
      fmt::format( "[{}] {} burned by the [{}]!", tribe_name,
                   dwelling_label, nation_name );
  if( destruction.missionary_to_release.has_value() )
    msg += " [Missionary] flees in panic!";
  else if( missionary_in_dwelling.has_value() )
    // Must have be a foreign missionary.
    msg += fmt::format( " [Foreign missionary] hanged!" );
  if( destruction.treasure_amount.has_value() )
    msg += fmt::format(
        " Treasure worth [{}] has been recovered! It will take "
        "a [Galleon] to transport this treasure back to [{}].",
        *destruction.treasure_amount, nation_harbor_name );
  co_await ts_.gui.message_box( msg );

  if( destruction.treasure_amount.has_value() ) {
    UNWRAP_CHECK( treasure_comp,
                  UnitComposition::create(
                      e_unit_type::treasure,
                      UnitComposition::UnitInventoryMap{
                          { e_unit_inventory::gold,
                            *destruction.treasure_amount } } ) );
    maybe<UnitId> const treasure_id =
        co_await create_unit_on_map( ss_, ts_, attacking_player_,
                                     treasure_comp,
                                     dwelling_location );
    treasure_ = treasure_id;
    // We'll be defensive here and not check-fail if the treasure
    // doesn't exist, even though that should never really happen
    // in a normal game... it is possible somehow that there
    // could be an LCR under the dwelling tile that could have
    // swallowed up the treasure unit.
    if( treasure_id.has_value() ) {
      AnimationSequence const seq =
          anim_seq_for_treasure_enpixelation( *treasure_id );
      co_await ts_.planes.land_view().animate( seq );
    }
  }

  if( was_capital && !destruction.tribe_destroyed.has_value() )
    co_await ts_.gui.message_box(
        "The [{}] bow before the might of the [{}]!", tribe_name,
        nation_name );

  // Check if the tribe is now destroyed.
  if( destruction.tribe_destroyed.has_value() )
    // At this point there should be nothing left
    co_await destroy_tribe_interactive( ss_, ts_, tribe_.type );
}

vector<UnitId> AttackDwellingHandler::units_to_prioritize()
    const {
  vector<UnitId> res;
  if( native_convert_.has_value() )
    res.push_back( *native_convert_ );
  if( treasure_.has_value() ) res.push_back( *treasure_ );
  return res;
}

} // namespace rn
