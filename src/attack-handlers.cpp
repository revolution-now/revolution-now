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
#include "agents.hpp"
#include "alarm.hpp"
#include "anim-builders.hpp"
#include "capture-cargo.hpp"
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
#include "ieuro-agent.hpp"
#include "igui.hpp"
#include "imap-updater.hpp"
#include "inative-agent.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "missionary.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "revolution-status.hpp"
#include "roles.hpp"
#include "show-anim.hpp"
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

// refl
#include "refl/to-str.hpp"
#include "visibility.hpp"

// base
#include "base/conv.hpp"
#include "base/logger.hpp"
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

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
    IEuroAgent& agent, e_attack_verdict_base verdict ) {
  switch( verdict ) {
    case e_attack_verdict_base::cancelled: //
      break;
    // Non-allowed (would break game rules).
    case e_attack_verdict_base::ship_attack_land_unit:
      co_await agent.message_box(
          "Ships cannot attack land units." );
      break;
    case e_attack_verdict_base::unit_cannot_attack:
      co_await agent.message_box( "This unit cannot attack." );
      break;
    case e_attack_verdict_base::attack_from_ship:
      co_await agent.message_box(
          "We cannot attack a land unit from a ship." );
      break;
    case e_attack_verdict_base::land_unit_attack_ship:
      co_await agent.message_box(
          "Land units cannot attack ships that are at sea." );
      break;
  }
}

e_direction direction_of_attack( SSConst const& ss,
                                 auto attacker_id,
                                 auto defender_id ) {
  Coord const attacker_coord =
      coord_for_unit_multi_ownership_or_die( ss, attacker_id );
  Coord const defender_coord =
      coord_for_unit_multi_ownership_or_die( ss, defender_id );
  UNWRAP_CHECK( d,
                attacker_coord.direction_to( defender_coord ) );
  return d;
}

e_direction direction_of_attack( SSConst const& ss,
                                 UnitId attacker_id,
                                 DwellingId dwelling_id ) {
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
  UnitId attacker_id_;
  Unit& attacker_;
  Player& attacking_player_;
  IEuroAgent& attacker_agent_;

  e_direction direction_;

  // The square on which the unit resides.
  Coord attack_src_{};

  // The square toward which the attack is aimed; this is the
  // same as the square of the unit being attacked.
  Coord attack_dst_{};
};

AttackHandlerBase::AttackHandlerBase( SS& ss, TS& ts,
                                      UnitId attacker_id,
                                      e_direction direction )
  : ss_( ss ),
    ts_( ts ),
    viz_( create_visibility_for(
        ss, player_for_role( ss, e_player_role::viewer ) ) ),
    attacker_id_( attacker_id ),
    attacker_( ss.units.unit_for( attacker_id ) ),
    attacking_player_( player_for_player_or_die(
        ss.players, attacker_.player_type() ) ),
    attacker_agent_( ts.euro_agents()[attacker_.player_type()] ),
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

  if( attacker_.movement_points() < 1 ) {
    if( co_await attacker_agent_
            .attack_with_partial_movement_points(
                attacker_.id() ) != ui::e_confirm::yes )
      co_return e_attack_verdict_base::cancelled;
  }
  co_return nothing;
}

wait<bool> AttackHandlerBase::confirm() {
  if( maybe<e_attack_verdict_base> const verdict =
          co_await check_attack_verdict_base();
      verdict.has_value() ) {
    co_await display_base_verdict_msg( attacker_agent_,
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
  UnitId defender_id_;
  Unit& defender_;
  Player& defending_player_;
  IEuroAgent& defender_agent_;
};

EuroAttackHandlerBase::EuroAttackHandlerBase(
    SS& ss, TS& ts, UnitId attacker_id, UnitId defender_id )
  : AttackHandlerBase(
        ss, ts, attacker_id,
        direction_of_attack( ss, attacker_id, defender_id ) ),
    defender_id_( defender_id ),
    defender_( ss.units.unit_for( defender_id ) ),
    defending_player_( player_for_player_or_die(
        ss.players, defender_.player_type() ) ),
    defender_agent_(
        ts.euro_agents()[defender_.player_type()] ) {
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
  NativeUnit& defender_;
  Tribe& defender_tribe_;
  INativeAgent& defender_agent_;
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
    defender_agent_( ts.native_agents()[defender_tribe_.type] ) {
}

/****************************************************************
** AttackColonyUndefendedHandler
*****************************************************************/
struct AttackColonyUndefendedHandler
  : public EuroAttackHandlerBase {
  using Base = EuroAttackHandlerBase;

  AttackColonyUndefendedHandler( SS& ss, TS& ts,
                                 UnitId attacker_id,
                                 UnitId defender_id,
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
  if( auto const seq =
          anim_seq_for_undefended_colony( ss_, combat );
      should_animate_seq( ss_, seq ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_agent_, defender_agent_ );

  if( combat.winner == e_combat_winner::defender )
    // return since in this case the attacker lost, so nothing
    // special happens; we just do what we normally do when an
    // attacker loses a battle.
    co_return;

  // The colony has been captured.

  // 1. The attacker moves into the colony square.
  if( auto const seq = anim_seq_for_unit_move(
          ss_, attacker_.id(), direction_ );
      should_animate_seq( ss_, seq ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );
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
  //    where G is the total gold of the player whose colony was
  //    captured, CP is the colony population, and TP is the
  //    total population of all colonies in that player.
  // TODO

  // 6. The colony changes ownership, as well as all of the units
  // that are working in it and who are on the map at the colony
  // location.
  change_colony_player( ss_, ts_, colony_,
                        attacker_.player_type() );

  // 7. Make adjustments to SoL of colony.

  // 8. Announce capture.
  // TODO: add an interface method to IGui for playing music.
  // conductor::play_request(
  //     ts_.rand, conductor::e_request::fife_drum_happy,
  //     conductor::e_request_probability::always );
  string const capture_msg = fmt::format(
      "The [{}] have captured the colony of [{}]!",
      player_display_name( attacking_player_ ), colony_.name );
  co_await attacker_agent_.message_box( capture_msg );
  co_await defender_agent_.message_box( capture_msg );

  // 8. Open colony view.
  if( attacker_agent_.human() ) {
    e_colony_abandoned const abandoned =
        co_await ts_.colony_viewer.show( ts_, colony_.id );
    if( abandoned == e_colony_abandoned::yes )
      // Nothing really special to do here.
      co_return;
  }

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

  wait<> perform_loser_cargo_captures(
      CombatShipAttackShip const& combat );

  wait<> perform_loser_cargo_capture(
      Unit& src, Unit& dst, IEuroAgent& src_agent,
      IEuroAgent& dst_agent, Player const& src_player,
      Player const& dst_player,
      EuroNavalUnitCombatOutcome const& src_outcome );
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

  AnimationSequenceForNavalBattle anim_seq =
      anim_seq_for_naval_battle( ss_, combat );

  // Do the initial slide part of the animation first.
  if( should_animate_seq( ss_, anim_seq.part_1 ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( anim_seq.part_1 );

  // For any unit that lost the battle (whether attacker or de-
  // fender/affected units) we need to check if they need to get
  // their cargo stolen if they've lost. This needs to be done
  // before the depixelation part of the animation (as the OG
  // does) because otherwise it looks strange if e.g. a defender
  // sinks and then we are asked what goods we want to capture
  // from it. Note that the loser will in any case lose all of
  // there cargo (units+commodities) but this step only does the
  // capturing.
  co_await perform_loser_cargo_captures( combat );

  if( should_animate_seq( ss_, anim_seq.part_2 ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( anim_seq.part_2 );

  // Must be done before performing effects (although it should
  // be ok that we've already done the cargo capture above).
  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_naval_unit_combat_effects( ss_, ts_, attacker_,
                                     defender_id_,
                                     combat.attacker.outcome );
  perform_naval_unit_combat_effects( ss_, ts_, defender_,
                                     attacker_id_,
                                     combat.defender.outcome );
  for( auto const& [unit_id, affected] :
       combat.affected_defender_units )
    perform_naval_affected_unit_combat_effects(
        ss_, ts_, attacker_id_, affected );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_agent_, defender_agent_ );

  // This is slightly hacky, but because the attacker may have
  // moved to the defender's square, but the above functions that
  // perform that movement don't do it interactively (see com-
  // ments in that function for why), so here we will rerun it
  // interactively just in case e.g. the ship discovers the pa-
  // cific ocean or another player upon moving.
  if( auto o = combat.attacker.outcome
                   .get_if<EuroNavalUnitCombatOutcome::moved>();
      o.has_value() )
    auto const /*deleted*/ _ =
        co_await UnitOwnershipChanger( ss_, attacker_id_ )
            .change_to_map( ts_, o->to );
}

wait<> NavalBattleHandler::perform_loser_cargo_captures(
    CombatShipAttackShip const& combat ) {
  using Capture = tuple<Unit*, Unit*, IEuroAgent*, IEuroAgent*,
                        Player const*, Player const*,
                        EuroNavalUnitCombatOutcome const*>;
  vector<Capture> v;

  v.push_back( { &attacker_, &defender_,                 //
                 &attacker_agent_, &defender_agent_,     //
                 &attacking_player_, &defending_player_, //
                 &combat.attacker.outcome } );
  v.push_back( { &defender_, &attacker_,                 //
                 &defender_agent_, &attacker_agent_,     //
                 &defending_player_, &attacking_player_, //
                 &combat.defender.outcome } );
  for( auto const& [unit_id, affected] :
       combat.affected_defender_units )
    v.push_back( { &ss_.units.unit_for( unit_id ), &attacker_, //
                   &defender_agent_, &attacker_agent_,         //
                   &defending_player_, &attacking_player_,     //
                   &affected.outcome } );

  for( auto const& tup : v ) {
    auto const& [src, dst, src_agent, dst_agent, src_player,
                 dst_player, outcome] = tup;
    co_await perform_loser_cargo_capture(
        *src, *dst, *src_agent, *dst_agent, *src_player,
        *dst_player, *outcome );
  }
}

wait<> NavalBattleHandler::perform_loser_cargo_capture(
    Unit& src, Unit& dst, IEuroAgent& src_agent,
    IEuroAgent& dst_agent, Player const& src_player,
    Player const& dst_player,
    EuroNavalUnitCombatOutcome const& src_outcome ) {
  // Here we make the assumption that we can infer loser status
  // from outcome, which would seem to be fine.
  if( !src_outcome.holds<EuroNavalUnitCombatOutcome::sunk>() &&
      !src_outcome.holds<EuroNavalUnitCombatOutcome::damaged>() )
    co_return;
  // At this point the src unit has been either damaged or sunk,
  // which entitles the other (dst) unit to capture its cargo if
  // it has sufficient space. Any cargo in the src unit that is
  // not captured will be destroyed. That will happen either au-
  // tomatically if it is sunk or will happen when it is moved
  // for repairs, so we don't need to do that here.
  CapturableCargo const capturable =
      capturable_cargo_items( ss_, src.cargo(), dst.cargo() );
  if( capturable.items.commodities.empty() ) co_return;
  CapturableCargoItems const items =
      co_await dst_agent.select_commodities_to_capture(
          src.id(), dst.id(), capturable );
  for( auto const& stolen : items.commodities )
    co_await src_agent.notify_captured_cargo(
        src_player, dst_player, dst, stolen );
  transfer_capturable_cargo_items( ss_, items, src.cargo(),
                                   dst.cargo() );
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
    co_await attacker_agent_.message_box(
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

  if( AnimationSequence const seq =
          anim_seq_for_euro_attack_euro( ss_, combat );
      should_animate_seq( ss_, seq ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss_, combat );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );
  perform_euro_unit_combat_effects( ss_, ts_, defender_,
                                    combat.defender.outcome );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      attacker_agent_, defender_agent_ );
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
      defender_tribe_.relationship[attacker_.player_type()];
  if( !relationship.player_has_attacked_tribe ) {
    YesNoConfig const config{
      .msg = fmt::format(
          "Shall we attack the [{}]?",
          ts_.gui.identifier_to_display_name(
              base::to_str( defender_tribe_.type ) ) ),
      .yes_label      = "Attack",
      .no_label       = "Cancel",
      .no_comes_first = true };
    maybe<ui::e_confirm> const proceed =
        co_await ts_.gui.optional_yes_no( config );
    if( proceed != ui::e_confirm::yes ) co_return false;
    relationship.player_has_attacked_tribe = true;
  }

  co_return true;
}

wait<> AttackNativeUnitHandler::perform() {
  // Should be done before consuming mv points in Base::perform.
  CombatEuroAttackBrave const combat =
      ts_.combat.euro_attack_brave( attacker_, defender_ );

  co_await Base::perform();

  if( AnimationSequence const seq =
          anim_seq_for_euro_attack_brave( ss_, combat );
      should_animate_seq( ss_, seq ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );

  // The tribal alarm goes up regardless of the battle outcome.
  TribeRelationship& relationship =
      defender_tribe_.relationship[attacker_.player_type()];
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
      attacker_agent_, defender_agent_ );
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
      CombatEuroAttackDwelling const& combat,
      base::function_ref<PhantomCombatAnimatorFunc> func );

  DwellingId dwelling_id_;
  Dwelling& dwelling_;
  Tribe& tribe_;
  INativeAgent& defender_agent_;
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
    defender_agent_( ts.native_agents()[tribe_.type] ),
    relationship_(
        tribe_.relationship[attacking_player_.type] ) {}

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
  string_view const player_name_possessive =
      player_possessive( attacking_player_ );
  co_await attacker_agent_.message_box(
      "[{}] citizens frightened in combat rush to the [{} "
      "mission] as [converts]!",
      tribe_name_possessive, player_name_possessive );

  // Produce the convert on the dwelling tile, then we will ani-
  // mate it enpixelating and then sliding over to the attacker's
  // square. We can use the non-interactive version because the
  // convert, although created on the dwelling square, will be
  // moved over to the attacker's square immediately, where we
  // know there is already a friendly unit, so no interactive
  // stuff need be done.
  Coord const dwelling_coord = attack_dst_;
  UnitId const convert_id = create_unit_on_map_non_interactive(
      ss_, ts_.map_updater(), attacking_player_,
      e_unit_type::native_convert, dwelling_coord );
  native_convert_ = convert_id;
  if( auto const seq = anim_seq_for_convert_produced(
          ss_, convert_id, reverse_direction( direction_ ) );
      should_animate_seq( ss_, seq ) )
    co_await ts_.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );
  // Non-interactive is OK here because the attacker is already
  // on this square.
  UnitOwnershipChanger( ss_, convert_id )
      .change_to_map_non_interactive( ts_.map_updater(),
                                      attacker_coord );
}

wait<> AttackDwellingHandler::with_phantom_brave_combat(
    CombatEuroAttackDwelling const& combat,
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
  string_view const player_name =
      player_display_name( attacking_player_ );
  string_view const player_name_possessive =
      player_possessive( attacking_player_ );
  string_view const player_harbor_name =
      nation_obj( attacking_player_.nation ).harbor_city_name;

  // Set new tribal alarm.
  // ------------------------------------------------------------
  // Tribal alarm. If this is a capital then tribal alarm will be
  // increased more.
  increase_tribal_alarm_from_attacking_dwelling(
      attacking_player_, dwelling_, relationship_ );

  // If we're burning the capital then reduce alarm to content.
  if( dwelling_.is_capital &&
      combat.defender.outcome
          .holds<DwellingCombatOutcome::destruction>() )
    relationship_.tribal_alarm =
        max_tribal_alarm_after_burning_capital();

  // Consume attacker movement points.
  // ------------------------------------------------------------
  // Done in base handler.

  // Check if the tribe has burned our missions.
  // ------------------------------------------------------------
  if( combat.missions_burned ) {
    // TODO: depixelation animation/sound?
    vector<UnitId> const missionaries =
        player_missionaries_in_tribe( ss_, attacking_player_,
                                      tribe_.type );

    // Update player squares to remove missionary status. We
    // don't have to worry about the ones that are currently vis-
    // ible, but the fogged ones would continue to show a cross.
    // It seems reasonable to remove the crosses from the fogged
    // ones because we are giving the player that information.
    // Currently our visibility framework doesn't easily allow
    // surgically editing the contents of the player square to
    // e.g. remove only the fogged missionary. So the simplest
    // way to do this is to just make all the fogged squares with
    // missions visible.
    //
    // Note that we don't use the member viz_ here because that
    // value is constructed from the current viewer, which is not
    // exactly what we want; we want to update the player squares
    // for the player doing the attacking, since they are the
    // ones that become aware of the destruction of the missions.
    vector<Coord> const fogged_dwelling_locations = [&] {
      vector<Coord> res;
      VisibilityForPlayer const viz_player(
          ss_, attacking_player_.type );
      res.reserve( missionaries.size() );
      for( UnitId const missionary : missionaries ) {
        CHECK( ss_.units.unit_for( missionary ).player_type() ==
               attacking_player_.type );
        UNWRAP_CHECK_T( auto const dwelling_id,
                        ss_.units.maybe_dwelling_for_missionary(
                            missionary ) );
        point const location =
            ss_.natives.coord_for( dwelling_id );
        if( viz_player.visible( location ) ==
            e_tile_visibility::fogged )
          res.push_back( location );
      }
      return res;
    }();
    ts_.map_updater().make_squares_visible(
        attacking_player_.type, fogged_dwelling_locations );

    // Destroy the missionaries.
    for( UnitId const missionary : missionaries ) {
      CHECK( ss_.units.unit_for( missionary ).player_type() ==
             attacking_player_.type );
      UnitOwnershipChanger( ss_, missionary ).destroy();
    }

    co_await attacker_agent_.message_box(
        "The [{}] revolt against [{}] missions! "
        "All {} missionaries eliminated!",
        tribe_name, player_name_possessive,
        player_name_possessive );
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
          if( should_animate_seq( ss_, seq ) )
            co_await ts_.planes.get()
                .get_bottom<ILandViewPlane>()
                .animate( seq );
        } );
    perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                      combat.attacker.outcome );
    co_await show_combat_effects_msg(
        effects_msg, attacker_agent_, defender_agent_ );
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
          if( should_animate_seq( ss_, seq ) )
            co_await ts_.planes.get()
                .get_bottom<ILandViewPlane>()
                .animate( seq );
        } );
    perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                      combat.attacker.outcome );
    co_await show_combat_effects_msg(
        effects_msg, attacker_agent_, defender_agent_ );
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

  // Save the actual missionary in the dwelling which may be dif-
  // ferent from the one to be released (if any are to be re-
  // leased).
  maybe<UnitId> const missionary_in_dwelling =
      ss_.units.missionary_from_dwelling( dwelling_id_ );

  // If missionary needs releasing, release it under the
  // dwelling. It should continue to be hidden until the dwelling
  // starts depixelating, then it should gradually become visi-
  // ble.
  if( destruction.missionary_to_release.has_value() ) {
    CHECK_EQ(
        ss_.units.unit_for( *destruction.missionary_to_release )
            .player_type(),
        attacking_player_.type );
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
        .change_to_map_non_interactive( ts_.map_updater(),
                                        dwelling_location );
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
        if( should_animate_seq( ss_, seq ) )
          co_await ts_.planes.get()
              .get_bottom<ILandViewPlane>()
              .animate( seq );
      } );

  bool const was_capital = dwelling_.is_capital;
  // Kill dwelling, free braves owned by the dwelling, and any
  // owned land of the dwelling. This will also remove the road
  // under the dwelling.
  destroy_dwelling( ss_, ts_.map_updater(), dwelling_id_ );
  perform_euro_unit_combat_effects( ss_, ts_, attacker_,
                                    combat.attacker.outcome );

  co_await show_combat_effects_msg( effects_msg, attacker_agent_,
                                    defender_agent_ );

  // Check if convert produced.
  if( destruction.convert_produced ) co_await produce_convert();

  // Show message saying that dwelling was destroyed, and that
  // missionary fled if applicable. Also, if there is a treasure,
  // include the amount.
  string msg =
      fmt::format( "[{}] {} burned by the [{}]!", tribe_name,
                   dwelling_label, player_name );
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
        config_text.special_chars.currency, player_harbor_name );
  co_await attacker_agent_.message_box( msg );

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
            ss_, ts_.map_updater(), attacking_player_,
            treasure_comp, dwelling_location );
    treasure_ = treasure_id;
    if( AnimationSequence const seq =
            anim_seq_for_treasure_enpixelation( ss_,
                                                treasure_id );
        should_animate_seq( ss_, seq ) )
      co_await ts_.planes.get()
          .get_bottom<ILandViewPlane>()
          .animate( seq );
    // Just in case e.g. the treasure appeared next to a brave
    // from unencountered tribe, or the pacific ocean.
    [[maybe_unused]] auto const unit_deleted =
        co_await UnitOwnershipChanger( ss_, treasure_id )
            .change_to_map( ts_, dwelling_location );
  }

  if( was_capital && !destruction.tribe_destroyed.has_value() )
    co_await attacker_agent_.message_box(
        "The [{}] bow before the might of the [{}]!", tribe_name,
        player_name );

  // Check if the tribe is now destroyed.
  if( destruction.tribe_destroyed.has_value() )
    // At this point there should be nothing left
    co_await destroy_tribe_interactive(
        ss_, attacker_agent_, ts_.map_updater(), tribe_.type );

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
