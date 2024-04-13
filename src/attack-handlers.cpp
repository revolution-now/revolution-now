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
#include "combat-effects.hpp"
#include "command.hpp"
#include "commodity.hpp"
#include "conductor.hpp"
#include "harbor-units.hpp"
#include "icombat.hpp"
#include "ieuro-mind.hpp"
#include "igui.hpp"
#include "inative-mind.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "minds.hpp"
#include "missionary.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "revolution-status.hpp"
#include "roles.hpp"
#include "tribe-mgr.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "unit-ownership.hpp"
#include "unit-stack.hpp"

// config
#include "config/nation.hpp"
#include "config/natives.rds.hpp"
#include "config/text.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/terrain.hpp"
#include "ss/tribe.rds.hpp"
#include "ss/units.hpp"

// base
#include "base/conv.hpp"
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

// refl
#include "refl/to-str.hpp"
#include "visibility.hpp"

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
    IEuroMind& mind, e_attack_verdict_base verdict ) {
  switch( verdict ) {
    case e_attack_verdict_base::cancelled: //
      break;
    // Non-allowed (would break game rules).
    case e_attack_verdict_base::ship_attack_land_unit:
      co_await mind.message_box(
          "Ships cannot attack land units." );
      break;
    case e_attack_verdict_base::unit_cannot_attack:
      co_await mind.message_box( "This unit cannot attack." );
      break;
    case e_attack_verdict_base::attack_from_ship:
      co_await mind.message_box(
          "We cannot attack a land unit from a ship." );
      break;
    case e_attack_verdict_base::land_unit_attack_ship:
      co_await mind.message_box(
          "Land units cannot attack ships that are at sea." );
      break;
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

/****************************************************************
** AttackHandlerBase
*****************************************************************/
// Base class for all handlers that have a european unit as the
// attacker.
struct AttackHandlerBase : public CommandHandler {
  AttackHandlerBase( SS& ss, TS& ts, UnitId attacker_id,
                     e_direction direction );

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> perform() override;

 protected:
  wait<maybe<e_attack_verdict_base>> check_attack_verdict_base()
      const;

  SS& ss_;
  TS& ts_;

  unique_ptr<IVisibility const> viz_;

  // The unit doing the attacking.
  UnitId     attacker_id_;
  Unit&      attacker_;
  Player&    attacking_player_;
  IEuroMind& attacker_mind_;
  bool       attacker_human_ = {};

  e_direction direction_;

  // The square on which the unit resides.
  Coord attack_src_{};

  // The square toward which the attack is aimed; this is the
  // same as the square of the unit being attacked.
  Coord attack_dst_{};
};

AttackHandlerBase::AttackHandlerBase( SS& ss, TS& ts,
                                      UnitId      attacker_id,
                                      e_direction direction )
  : ss_( ss ),
    ts_( ts ),
    viz_( create_visibility_for(
        ss, player_for_role( ss, e_player_role::viewer ) ) ),
    attacker_id_( attacker_id ),
    attacker_( ss.units.unit_for( attacker_id ) ),
    attacking_player_( player_for_nation_or_die(
        ss.players, attacker_.nation() ) ),
    attacker_mind_( ts.euro_minds[attacker_.nation()] ),
    attacker_human_( ss.players.humans[attacker_.nation()] ),
    direction_( direction ) {
  CHECK( viz_ != nullptr );
  attack_src_ =
      coord_for_unit_indirect_or_die( ss_.units, attacker_id_ );
  attack_dst_ = attack_src_.moved( direction_ );

  CHECK( attack_src_ != attack_dst_ );
  CHECK( attack_src_.is_adjacent_to( attack_dst_ ) );
}

// This is for checking for no-go conditions that apply to both
// attacking a euro unit and attacking native units.
wait<maybe<e_attack_verdict_base>>
AttackHandlerBase::check_attack_verdict_base() const {
  if( is_unit_onboard( ss_.units, attacker_.id() ) )
    co_return e_attack_verdict_base::attack_from_ship;

  Coord const source = ss_.units.coord_for( attacker_.id() );
  Coord const target = source.moved( direction_ );

  if( !can_attack( attacker_.type() ) )
    co_return e_attack_verdict_base::unit_cannot_attack;

  if( attacker_.desc().ship ) {
    // Ship-specific checks.
    if( ss_.terrain.is_land( target ) )
      co_return e_attack_verdict_base::ship_attack_land_unit;
  }

  if( surface_type( ss_.terrain.square_at( target ) ) ==
          e_surface::water &&
      !attacker_.desc().ship )
    co_return e_attack_verdict_base::land_unit_attack_ship;

  if( attacker_.movement_points() < 1 && attacker_human_ ) {
    if( co_await ts_.gui.optional_yes_no(
            { .msg = fmt::format(
                  "This unit only has [{}] movement points "
                  "and so will not be fighting at full "
                  "strength.  Continue?",
                  attacker_.movement_points() ),
              .yes_label =
                  "Yes, let us proceed with full force!",
              .no_label = "No, do not attack." } ) !=
        ui::e_confirm::yes )
      co_return e_attack_verdict_base::cancelled;
  }
  co_return nothing;
}

wait<bool> AttackHandlerBase::confirm() {
  if( maybe<e_attack_verdict_base> const verdict =
          co_await check_attack_verdict_base();
      verdict.has_value() ) {
    co_await display_base_verdict_msg( attacker_mind_,
                                       *verdict );
    co_return false;
  }

  co_return true;
}

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

  EuroAttackHandlerBase( SS& ss, TS& ts, UnitId attacker_id,
                         UnitId defender_id );

 protected:
  UnitId     defender_id_;
  Unit&      defender_;
  Player&    defending_player_;
  IEuroMind& defender_mind_;
};

EuroAttackHandlerBase::EuroAttackHandlerBase(
    SS& ss, TS& ts, UnitId attacker_id, UnitId defender_id )
  : AttackHandlerBase(
        ss, ts, attacker_id,
        direction_of_attack( ss, attacker_id, defender_id ) ),
    defender_id_( defender_id ),
    defender_( ss.units.unit_for( defender_id ) ),
    defending_player_( player_for_nation_or_die(
        ss.players, defender_.nation() ) ),
    defender_mind_( ts.euro_minds[defender_.nation()] ) {
  CHECK( defender_id_ != attacker_id_ );
}

/****************************************************************
** NativeAttackHandlerBase
*****************************************************************/
// For when the defender is a native unit.
struct NativeAttackHandlerBase : public AttackHandlerBase {
  using Base = AttackHandlerBase;

  NativeAttackHandlerBase( SS& ss, TS& ts, UnitId attacker_id,
                           NativeUnitId defender_id );

 protected:
  NativeUnitId defender_id_;
  NativeUnit&  defender_;
  Tribe&       defender_tribe_;
  INativeMind& defender_mind_;
};

NativeAttackHandlerBase::NativeAttackHandlerBase(
    SS& ss, TS& ts, UnitId attacker_id,
    NativeUnitId defender_id )
  : AttackHandlerBase(
        ss, ts, attacker_id,
        direction_of_attack( ss, attacker_id, defender_id ) ),
    defender_id_( defender_id ),
    defender_( ss.units.unit_for( defender_id ) ),
    defender_tribe_( tribe_for_unit( ss, defender_ ) ),
    defender_mind_( ts.native_minds[defender_tribe_.type] ) {}

/****************************************************************
** AttackColonyUndefendedHandler
*****************************************************************/
struct AttackColonyUndefendedHandler
  : public EuroAttackHandlerBase {
  using Base = EuroAttackHandlerBase;

  AttackColonyUndefendedHandler( SS& ss, TS& ts,
                                 UnitId  attacker_id,
                                 UnitId  defender_id,
                                 Colony& colony );

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> perform() override;

 private:
  Colony& colony_;
};

AttackColonyUndefendedHandler::AttackColonyUndefendedHandler(
    SS& ss, TS& ts, UnitId attacker_id, UnitId defender_id,
    Colony& colony )
  : EuroAttackHandlerBase( ss, ts, attacker_id, defender_id ),
    colony_( colony ) {}

wait<bool> AttackColonyUndefendedHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;

  co_return true;
}

// For this sequence the animations need to be interleaved with
// actions, so the entire thing is done in the perform() method.
// In particular, the attacking unit, if it wins and gets pro-
// moted, needs to be promoted before it slides into the colony.
wait<> AttackColonyUndefendedHandler::perform() {
  // Should be done before consuming mv points in Base::perform.
  CombatEuroAttackUndefendedColony const combat =
      ts_.combat.euro_attack_undefended_colony(
          attacker_, defender_, colony_ );

  co_await Base::perform();

  // Animate the attack part of it. If the colony is captured
  // then the remainder will be done further below.
  co_await ts_.planes.land_view().animate(
      anim_seq_for_undefended_colony( ss_, combat ) );

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_mind_, defender_mind_ );

  if( combat.winner == e_combat_winner::defender )
    // return since in this case the attacker lost, so nothing
    // special happens; we just do what we normally do when an
    // attacker loses a battle.
    co_return;

  // The colony has been captured.

  // 1. The attacker moves into the colony square.
  co_await ts_.planes.land_view().animate(
      anim_seq_for_unit_move( ss_, attacker_.id(),
                              direction_ ) );
  maybe<UnitDeleted> const unit_deleted =
      co_await UnitOwnershipChanger( ss_, attacker_.id() )
          .change_to_map( ts_, attack_dst_ );
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
  //
  // Note that when we are searching for a port colony to repair
  // the ship we should make sure that it doesn't find the colony
  // being captured; that might require modifying the
  // find_repair_port_for_ship function.
  // TODO

  // 3. Any veteran_colonists in the colony must have their vet-
  // eran status stripped.
  // TODO

  // 4. Any non-military units at the gate (including wagon
  // trains) are captured (not destroyed) and a message pops up
  // informing of that.
  // TODO

  // 5. Compute gold plundered.  The OG using this formula:
  //
  //      plundered = G*(CP/TP)
  //
  //    where G is the total gold of the nation whose colony was
  //    captured, CP is the colony population, and TP is the
  //    total population of all colonies in that nation.
  // TODO

  // 6. The colony changes ownership, as well as all of the units
  // that are working in it and who are on the map at the colony
  // location.
  change_colony_nation( ss_, ts_, colony_, attacker_.nation() );

  // 7. Announce capture.
  // TODO: add an interface method to IGui for playing music.
  // conductor::play_request(
  //     ts_.rand, conductor::e_request::fife_drum_happy,
  //     conductor::e_request_probability::always );
  string const capture_msg = fmt::format(
      "The [{}] have captured the colony of [{}]!",
      nation_display_name( attacking_player_ ), colony_.name );
  co_await attacker_mind_.message_box( capture_msg );
  co_await defender_mind_.message_box( capture_msg );

  // 8. Open colony view.
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
  wait<> perform() override;
};

wait<bool> NavalBattleHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;
  co_return true;
}

wait<> NavalBattleHandler::perform() {
  // Should be done before consuming mv points in Base::perform.
  CombatShipAttackShip const combat =
      ts_.combat.ship_attack_ship( attacker_, defender_ );

  co_await Base::perform();

  co_await ts_.planes.land_view().animate(
      anim_seq_for_naval_battle( ss_, combat ) );

  if( combat.winner.has_value() ) {
    // One of the ships was either damaged or sunk.
    Unit const& loser =
        ( combat.winner == e_combat_winner::attacker )
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
      //
      // TODO: add a method to IEuroMind to implement this for
      // both human and AI players. Until then, we will just
      // clear the cargo out of the ship if it has been damaged.
      for( auto [comm, slot] : loser.cargo().commodities() ) {
        Commodity const removed = rm_commodity_from_cargo(
            ss_.units, loser.id(), slot );
        CHECK_EQ( removed, comm );
      }
    }
  }

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_naval_unit_combat_effects( ss_, ts_, attacker_,
                                     defender_id_,
                                     combat.attacker.outcome );
  perform_naval_unit_combat_effects( ss_, ts_, defender_,
                                     attacker_id_,
                                     combat.defender.outcome );
  // The order of iteration here may be non-deterministic but it
  // shouldn't cause any issues.
  for( auto const& [unit_id, affected] :
       combat.affected_defender_units )
    perform_naval_affected_unit_combat_effects(
        ss_, ts_, attacker_id_, affected );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_mind_, defender_mind_ );

  // This is slightly hacky, but because the attacker may have
  // moved to the defender's square, but the above functions that
  // perform that movement don't do it interactively (see com-
  // ments in that function for why), so here we will rerun it
  // interactively just in case e.g. the ship discovers the pa-
  // cific ocean or another nation upon moving.
  if( auto o = combat.attacker.outcome
                   .get_if<EuroNavalUnitCombatOutcome::moved>();
      o.has_value() )
    UnitOwnershipChanger( ss_, attacker_id_ )
        .reinstate_on_map_if_on_map( ts_ );
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
  wait<> perform() override;
};

wait<bool> EuroAttackHandler::confirm() {
  CHECK( !attacker_.desc().ship );
  if( defender_.desc().ship ) {
    co_await attacker_mind_.message_box(
        "Our land units can neither attack nor board foreign "
        "ships." );
    co_return false;
  }
  // Do all of the more generic tests that don't know about the
  // defender.
  if( !co_await Base::confirm() ) co_return false;
  co_return true;
}

wait<> EuroAttackHandler::perform() {
  // Should be done before consuming mv points in Base::perform.
  CombatEuroAttackEuro const combat =
      ts_.combat.euro_attack_euro( attacker_, defender_ );

  co_await Base::perform();

  AnimationSequence const seq =
      anim_seq_for_euro_attack_euro( ss_, combat );
  co_await ts_.planes.land_view().animate( seq );

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );
  perform_euro_unit_combat_effects( ss_, ts_, defender_,
                                    combat.defender.outcome );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_mind_, defender_mind_ );
}

/****************************************************************
** AttackNativeUnitHandler
*****************************************************************/
struct AttackNativeUnitHandler : public NativeAttackHandlerBase {
  using Base = NativeAttackHandlerBase;

  using Base::Base;

  // Implement CommandHandler.
  wait<bool> confirm() override;

  // Implement CommandHandler.
  wait<> perform() override;
};

// Returns true if the move is allowed.
wait<bool> AttackNativeUnitHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;

  TribeRelationship& relationship =
      defender_tribe_.relationship[attacker_.nation()];
  if( !relationship.nation_has_attacked_tribe ) {
    if( attacker_human_ ) {
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
    }
    relationship.nation_has_attacked_tribe = true;
  }

  co_return true;
}

wait<> AttackNativeUnitHandler::perform() {
  // Should be done before consuming mv points in Base::perform.
  CombatEuroAttackBrave const combat =
      ts_.combat.euro_attack_brave( attacker_, defender_ );

  co_await Base::perform();

  AnimationSequence const seq =
      anim_seq_for_euro_attack_brave( ss_, combat );
  co_await ts_.planes.land_view().animate( seq );

  // The tribal alarm goes up regardless of the battle outcome.
  TribeRelationship& relationship =
      defender_tribe_.relationship[attacker_.nation()];
  increase_tribal_alarm_from_attacking_brave(
      attacking_player_,
      ss_.natives.dwelling_for(
          ss_.units.dwelling_for( defender_id_ ) ),
      relationship );

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );
  perform_native_unit_combat_effects( ss_, defender_,
                                      combat.defender.outcome );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_mind_, defender_mind_ );
}

/****************************************************************
** AttackDwellingHandler
*****************************************************************/
struct AttackDwellingHandler : public AttackHandlerBase {
  using Base = AttackHandlerBase;

  AttackDwellingHandler( SS& ss, TS& ts, UnitId attacker_id,
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
      CombatEuroAttackDwelling const&               combat,
      base::function_ref<PhantomCombatAnimatorFunc> func );

  DwellingId         dwelling_id_;
  Dwelling&          dwelling_;
  Tribe&             tribe_;
  INativeMind&       defender_mind_;
  TribeRelationship& relationship_;

  maybe<UnitId> treasure_;
  maybe<UnitId> native_convert_;
};

AttackDwellingHandler::AttackDwellingHandler(
    SS& ss, TS& ts, UnitId attacker_id, DwellingId dwelling_id )
  : AttackHandlerBase(
        ss, ts, attacker_id,
        direction_of_attack( ss, attacker_id, dwelling_id ) ),
    dwelling_id_( dwelling_id ),
    dwelling_( ss.natives.dwelling_for( dwelling_id ) ),
    tribe_( ss.natives.tribe_for( dwelling_.id ) ),
    defender_mind_( ts.native_minds[tribe_.type] ),
    relationship_(
        tribe_.relationship[attacking_player_.nation] ) {}

// Returns true if the move is allowed.
wait<bool> AttackDwellingHandler::confirm() {
  if( !co_await Base::confirm() ) co_return false;
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
  string_view const tribe_name_possessive =
      config_natives.tribes[tribe_.type].name_possessive;
  string_view const nation_name_possessive =
      nation_possessive( attacking_player_ );
  co_await attacker_mind_.message_box(
      "[{}] citizens frightened in combat rush to the [{} "
      "mission] as [converts]!",
      tribe_name_possessive, nation_name_possessive );

  // Produce the convert on the dwelling tile, then we will ani-
  // mate it enpixelating and then sliding over to the attacker's
  // square. We can use the non-interactive version because the
  // convert, although created on the dwelling square, will be
  // moved over to the attacker's square immediately, where we
  // know there is already a friendly unit, so no interactive
  // stuff need be done.
  Coord const  dwelling_coord = attack_dst_;
  UnitId const convert_id = create_unit_on_map_non_interactive(
      ss_, ts_, attacking_player_, e_unit_type::native_convert,
      dwelling_coord );
  native_convert_ = convert_id;
  co_await ts_.planes.land_view().animate(
      anim_seq_for_convert_produced(
          ss_, convert_id, reverse_direction( direction_ ) ) );
  // Non-interactive is OK here because the attacker is already
  // on this square.
  UnitOwnershipChanger( ss_, convert_id )
      .change_to_map_non_interactive( ts_, attacker_coord );
}

wait<> AttackDwellingHandler::with_phantom_brave_combat(
    CombatEuroAttackDwelling const&               combat,
    base::function_ref<PhantomCombatAnimatorFunc> func ) {
  NativeUnitId const phantom_brave = create_phantom_brave();
  SCOPE_EXIT { ss_.units.destroy_unit( phantom_brave ); };
  CombatEuroAttackBrave phantom_combat{
      .winner   = combat.winner,
      .attacker = combat.attacker,
      .defender = { .id = phantom_brave } };
  if( combat.winner == e_combat_winner::attacker )
    phantom_combat.defender.outcome =
        NativeUnitCombatOutcome::destroyed{};
  else
    phantom_combat.defender.outcome =
        NativeUnitCombatOutcome::no_change{};
  co_await func( phantom_combat );
}

wait<> AttackDwellingHandler::perform() {
  // Should be done before consuming mv points in Base::perform.
  CombatEuroAttackDwelling const combat =
      ts_.combat.euro_attack_dwelling( attacker_, dwelling_ );

  co_await Base::perform();

  auto const& tribe_conf = config_natives.tribes[tribe_.type];
  string_view const dwelling_label =
      dwelling_.is_capital
          ? "capital"sv
          : config_natives.dwelling_types[tribe_conf.level]
                .name_singular;
  string_view const tribe_name = tribe_conf.name_singular;
  string_view const nation_name =
      nation_display_name( attacking_player_ );
  string_view const nation_name_possessive =
      nation_possessive( attacking_player_ );
  string_view const nation_harbor_name =
      nation_obj( attacking_player_.nation ).harbor_city_name;

  // Set new tribal alarm.
  relationship_.tribal_alarm = combat.new_tribal_alarm;

  // Consume attacker movement points.
  // Done in base handler.

  // Check if the tribe has burned our missions.
  if( combat.missions_burned ) {
    // TODO: depixelation animation/sound?
    vector<UnitId> const missionaries =
        player_missionaries_in_tribe( ss_, attacking_player_,
                                      tribe_.type );
    for( UnitId const missionary : missionaries ) {
      CHECK( ss_.units.unit_for( missionary ).nation() ==
             attacking_player_.nation );
      UnitOwnershipChanger( ss_, missionary ).destroy();
    }
    // TODO: decide whether to update player fog squares to re-
    // move missionary status. We don't have to worry about the
    // ones that are currently visible, but the fogged ones would
    // continue to show a cross. It seems reasonable to remove
    // the crosses from the fogged ones because we are giving the
    // player that information.
    co_await attacker_mind_.message_box(
        "The [{}] revolt against [{}] missions! "
        "All {} missionaries eliminated!",
        tribe_name, nation_name_possessive,
        nation_name_possessive );
  }

  FilteredMixedCombatEffectsMessages const effects_msg =
      filter_combat_effects_msgs( mix_combat_effects_msgs(
          combat_effects_msg( ss_, combat ) ) );

  // Attacker lost:
  if( combat.winner == e_combat_winner::defender ) {
    CHECK( combat.defender.outcome
               .holds<DwellingCombatOutcome::no_change>() );
    co_await with_phantom_brave_combat(
        combat,
        [&]( CombatEuroAttackBrave const& combat ) -> wait<> {
          AnimationSequence const seq =
              anim_seq_for_euro_attack_brave( ss_, combat );
          co_await ts_.planes.land_view().animate( seq );
        } );
    perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                      combat.attacker.outcome );
    co_await show_combat_effects_msg(
        effects_msg, attacker_mind_, defender_mind_ );
    co_return;
  }

  // Attacker won.

  // Population decrease.
  if( auto population_decrease =
          combat.defender.outcome.get_if<
              DwellingCombatOutcome::population_decrease>();
      population_decrease.has_value() ) {
    co_await with_phantom_brave_combat(
        combat,
        [&]( CombatEuroAttackBrave const& combat ) -> wait<> {
          AnimationSequence const seq =
              anim_seq_for_euro_attack_brave( ss_, combat );
          co_await ts_.planes.land_view().animate( seq );
        } );
    perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                      combat.attacker.outcome );
    co_await show_combat_effects_msg(
        effects_msg, attacker_mind_, defender_mind_ );
    --dwelling_.population;
    CHECK_GT( dwelling_.population, 0 );
    if( population_decrease->convert_produced )
      co_await produce_convert();
    co_return;
  }

  // Dwelling burned.
  UNWRAP_CHECK(
      destruction,
      combat.defender.outcome
          .get_if<DwellingCombatOutcome::destruction>() );
  Coord const dwelling_location =
      ss_.natives.coord_for( dwelling_id_ );

  // Inc villages burned.
  ++attacking_player_.score_stats.dwellings_burned;

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
    // We need to use the non-interactive version here because,
    // at this point, the player is not aware that the missionary
    // has been released (no visual indication or messages), and
    // so if we were to release it onto the map interactively
    // then it could trigger some interactive events which would
    // probably seem confusing to the player. An example of such
    // an interactive event is that the missionary, upon being
    // released on the dwelling square, might end up adjacent to
    // a brave of another unencountered tribe which would then
    // trigger the meet-tribe UI sequence, which would feel
    // strange right in the middle of an attack sequence.
    UnitOwnershipChanger( ss_,
                          *destruction.missionary_to_release )
        .change_to_map_non_interactive( ts_, dwelling_location );
  }

  // Animate attacker winning w/ burning village and depixelating
  // all braves.
  co_await with_phantom_brave_combat(
      combat,
      [&]( CombatEuroAttackBrave const& phantom_combat )
          -> wait<> {
        AnimationSequence const seq = anim_seq_for_dwelling_burn(
            ss_, *viz_, attacker_id_, combat.attacker.outcome,
            phantom_combat.defender.id, dwelling_id_,
            combat.defender.outcome );
        co_await ts_.planes.land_view().animate( seq );
      } );

  bool const was_capital = dwelling_.is_capital;
  // Kill dwelling, free braves owned by the dwelling, and any
  // owned land of the dwelling. This will also remove the road
  // under the dwelling.
  destroy_dwelling( ss_, ts_.map_updater, dwelling_id_ );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );

  co_await show_combat_effects_msg( effects_msg, attacker_mind_,
                                    defender_mind_ );

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
        " Treasure worth [{}{}] has been recovered! It will "
        "take a [Galleon] to transport this treasure back to "
        "[{}].",
        *destruction.treasure_amount,
        config_text.special_chars.currency, nation_harbor_name );
  co_await attacker_mind_.message_box( msg );

  if( destruction.treasure_amount.has_value() ) {
    UNWRAP_CHECK( treasure_comp,
                  UnitComposition::create(
                      e_unit_type::treasure,
                      UnitComposition::UnitInventoryMap{
                          { e_unit_inventory::gold,
                            *destruction.treasure_amount } } ) );
    // This one needs to be non-interactive for the same reason
    // as is described in the comment further above at the point
    // when we release the missionary from the dwelling. Specfi-
    // cally, it would be weird to enter an interactive sequence
    // here since the treasure has not yet appeared yet. We will
    // take care of this by replacing it interactively after it
    // appears.
    UnitId const treasure_id =
        create_unit_on_map_non_interactive(
            ss_, ts_, attacking_player_, treasure_comp,
            dwelling_location );
    treasure_ = treasure_id;
    AnimationSequence const seq =
        anim_seq_for_treasure_enpixelation( ss_, treasure_id );
    co_await ts_.planes.land_view().animate( seq );
    // Just in case e.g. the treasure appeared next to a brave
    // from unencountered tribe, or the pacific ocean.
    [[maybe_unused]] auto const unit_deleted =
        co_await UnitOwnershipChanger( ss_, treasure_id )
            .change_to_map( ts_, dwelling_location );
  }

  if( was_capital && !destruction.tribe_destroyed.has_value() )
    co_await attacker_mind_.message_box(
        "The [{}] bow before the might of the [{}]!", tribe_name,
        nation_name );

  // Check if the tribe is now destroyed.
  if( destruction.tribe_destroyed.has_value() )
    // At this point there should be nothing left
    co_await destroy_tribe_interactive( ss_, ts_, tribe_.type );

  if( destruction.missionary_to_release.has_value() ) {
    // Now that the missionary is visible, we will replace it on
    // the world interactively, e.g. in case it ended up next to
    // a brave from an unencountered tribe. We do this last be-
    // cause any UI sequence that this leads to will be unrelated
    // to the burning of the dwelling.
    [[maybe_unused]] auto const unit_deleted =
        co_await UnitOwnershipChanger(
            ss_, *destruction.missionary_to_release )
            .change_to_map( ts_, dwelling_location );
  }
}

vector<UnitId> AttackDwellingHandler::units_to_prioritize()
    const {
  vector<UnitId> res;
  if( native_convert_.has_value() )
    res.push_back( *native_convert_ );
  if( treasure_.has_value() ) res.push_back( *treasure_ );
  return res;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
unique_ptr<CommandHandler> attack_euro_land_handler(
    SS& ss, TS& ts, UnitId attacker_id, UnitId defender_id ) {
  return make_unique<EuroAttackHandler>( ss, ts, attacker_id,
                                         defender_id );
}

unique_ptr<CommandHandler> naval_battle_handler(
    SS& ss, TS& ts, UnitId attacker_id, UnitId defender_id ) {
  return make_unique<NavalBattleHandler>( ss, ts, attacker_id,
                                          defender_id );
}

unique_ptr<CommandHandler> attack_colony_undefended_handler(
    SS& ss, TS& ts, UnitId attacker_id, UnitId defender_id,
    Colony& colony ) {
  return make_unique<AttackColonyUndefendedHandler>(
      ss, ts, attacker_id, defender_id, colony );
}

unique_ptr<CommandHandler> attack_native_unit_handler(
    SS& ss, TS& ts, UnitId attacker_id,
    NativeUnitId defender_id ) {
  return make_unique<AttackNativeUnitHandler>(
      ss, ts, attacker_id, defender_id );
}

unique_ptr<CommandHandler> attack_dwelling_handler(
    SS& ss, TS& ts, UnitId attacker_id,
    DwellingId dwelling_id ) {
  return make_unique<AttackDwellingHandler>( ss, ts, attacker_id,
                                             dwelling_id );
}

} // namespace rn
