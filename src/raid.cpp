/****************************************************************
**raid.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-10.
*
* Description: Orchestrates the process of a brave raiding
*              a colony.
*
*****************************************************************/
#include "raid.hpp"

// Revolution Now
#include "anim-builders.hpp"
#include "co-wait.hpp" // IWYU pragma: keep
#include "colony-mgr.hpp"
#include "combat-effects.hpp"
#include "damaged.rds.hpp"
#include "icombat.hpp"
#include "ieuro-mind.hpp"
#include "inative-mind.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-search.hpp"
#include "minds.hpp"
#include "plane-stack.hpp"
#include "raid-effects.hpp"
#include "roles.hpp"
#include "unit-mgr.hpp"
#include "unit-stack.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/fog-square.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

wait<> surprise_raid_msg( SSConst const& ss,
                          IEuroMind&     euro_mind,
                          Coord          defender_loc,
                          e_tribe        tribe_type ) {
  e_nation const friendly_nation = euro_mind.nation();
  string         where;
  maybe<FogColony const&> const closest =
      find_close_explored_colony(
          ss, friendly_nation, defender_loc,
          /*max_distance=*/
          config_colony.search_dist_for_nearby_colony );
  if( closest.has_value() ) {
    std::string const conjunction =
        ( closest->location == defender_loc ) ? "of" : "near";
    where =
        fmt::format( " {} [{}]", conjunction, closest->name );
  }
  co_await euro_mind.message_box(
      "[{}] make surprise raid{}! Terror descends upon "
      "colonists! {} chief unavailable for comment.",
      config_natives.tribes[tribe_type].name_plural, where,
      config_natives.tribes[tribe_type].name_possessive );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> raid_unit( SS& ss, TS& ts, NativeUnit& attacker,
                  Coord dst ) {
  UnitId const defender_id =
      select_euro_unit_defender( ss, dst );
  Visibility const viz(
      ss, player_for_role( ss, e_player_role::viewer ) );
  e_tribe const tribe_type = tribe_type_for_unit( ss, attacker );
  INativeMind&  native_mind = ts.native_minds[tribe_type];
  Unit&         defender    = ss.units.unit_for( defender_id );
  CombatBraveAttackEuro const combat =
      ts.combat.brave_attack_euro( as_const( attacker ),
                                   as_const( defender ) );
  Coord const src       = ss.units.coord_for( attacker.id );
  IEuroMind&  euro_mind = ts.euro_minds[defender.nation()];

  // Note that for attacks the "show indian moves" game flag is
  // not relevant, since there is really no natural way to show
  // an attack without the slide and depixelation animations, es-
  // pecially if it results in a unit or colony disappearing. So
  // all that we really care about here is if the viewer can see
  // either the src or dst square.
  bool const viewable = should_animate_move( viz, src, dst );

  if( viewable ) {
    co_await ts.planes.land_view().ensure_visible( src );
    co_await ts.planes.land_view().ensure_visible( dst );
  }

  co_await surprise_raid_msg( ss, euro_mind, dst, tribe_type );

  if( viewable )
    co_await ts.planes.land_view().animate(
        anim_seq_for_brave_attack_euro( ss, combat ) );

  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss, combat );
  perform_euro_unit_combat_effects( ss, ts, defender,
                                    combat.defender.outcome );
  perform_native_unit_combat_effects( ss, attacker,
                                      combat.attacker.outcome );
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      native_mind, euro_mind );
  native_mind.on_attack_unit_finished( combat );
}

// This is when there is a military unit defending the colony and
// so it ends up as just a normal battle. We still need a special
// handler for it because there can still be side effects (e.g.
// money stolen) which wouldn't be handled if we were to delegate
// to the normal brave attack handler.
static wait<> raid_colony_battle(
    SS& ss, TS& ts, NativeUnit& attacker, Colony& colony,
    e_tribe tribe_type, CombatBraveAttackColony const& combat ) {
  CHECK( !combat.colony_destroyed );
  IEuroMind& euro_mind = ts.euro_minds[colony.nation];
  Unit&      defender  = ss.units.unit_for( combat.defender.id );
  // Note: there are there still side effects if the brave
  // loses. We only suppress the side effect if the colony is
  // destroyed, because many of those effects don't really make
  // sense if the colony is gone.
  BraveAttackColonyEffect const side_effect =
      combat.colony_destroyed
          ? BraveAttackColonyEffect::none{}
          : select_brave_attack_colony_effect( ss, ts.rand,
                                               colony );
  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss, combat );

  // Perform all effects.
  perform_brave_attack_colony_effect( ss, ts, colony,
                                      side_effect );
  perform_native_unit_combat_effects( ss, attacker,
                                      combat.attacker.outcome );
  perform_euro_unit_combat_effects( ss, ts, defender,
                                    combat.defender.outcome );

  // !! NOTE: the attacker will no longer exist at this point.

  INativeMind& native_mind = ts.native_minds[tribe_type];
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      native_mind, euro_mind );
  co_await display_brave_attack_colony_effect_msg(
      ss, euro_mind, colony, side_effect, tribe_type );
  native_mind.on_attack_colony_finished( combat, side_effect );
}

static wait<> raid_colony_burn(
    SS& ss, TS& ts, NativeUnit& attacker, Colony& colony,
    e_tribe tribe_type, CombatBraveAttackColony const& combat ) {
  IEuroMind& euro_mind = ts.euro_minds[colony.nation];
  Player&    player =
      player_for_nation_or_die( ss.players, colony.nation );
  Unit& defender = ss.units.unit_for( combat.defender.id );
  UnitId const                defender_id = defender.id();
  CombatEffectsMessages const effects_msg =
      combat_effects_msg( ss, combat );

  // Perform all effects.
  perform_native_unit_combat_effects( ss, attacker,
                                      combat.attacker.outcome );
  // This shouldn't be necessary because all it will do is to de-
  // stroy the (non-military) unit, which would happen anyway
  // further below. But we'll do it for consistency.
  perform_euro_unit_combat_effects( ss, ts, defender,
                                    combat.defender.outcome );
  CHECK( !ss.units.exists( defender_id ) );

  // All non-ship units at the gate (which should all be
  // non-military given that the colony is being burned) get de-
  // stroyed. Ships will be sent for repair by the colony de-
  // struction routine. We need to copy this because we can't it-
  // erate over it while destroying units.
  for( auto units_at_gate =
           ss.units.from_coord( colony.location );
       GenericUnitId const id : units_at_gate ) {
    Unit const& unit = ss.units.euro_unit_for( id );
    if( unit.desc().ship ) continue;
    lg.debug( "{} at gate lost in battle.", unit.desc().name );
    destroy_unit( ss, id );
  }

  // One of the things this will do is it will move any remaining
  // ships to be repaired. Note that this does not do any anima-
  // tion, since it is assumed to have happened already.
  co_await run_colony_destruction( ss, ts, colony,
                                   e_ship_damaged_reason::battle,
                                   /*msg=*/nothing );

  // !! NOTE: the colony, attacker, and defender no longer exist
  // at this point.

  co_await show_woodcut_if_needed( player, euro_mind,
                                   e_woodcut::colony_burning );

  INativeMind& native_mind = ts.native_minds[tribe_type];
  co_await show_combat_effects_msg(
      filter_combat_effects_msgs(
          mix_combat_effects_msgs( effects_msg ) ),
      native_mind, euro_mind );
  native_mind.on_attack_colony_finished(
      combat, BraveAttackColonyEffect::none{} );
}

wait<> raid_colony( SS& ss, TS& ts, NativeUnit& attacker,
                    Colony& colony ) {
  // This offboarding is done automatically when the colonist is
  // destroyed (because ships are marked as damaged), but we need
  // to do it first here so that the units therein can be consid-
  // ered when selecting the defender. This is to emulate the be-
  // havior of the OG which doesn't really have a concept of
  // units on ships technically; the units would just be sentried
  // and thus would participate in defending the colony.
  vector<UnitId> const offboarded =
      offboard_units_on_ships( ss, ts, colony.location );
  Unit& defender =
      ss.units.unit_for( select_colony_defender( ss, colony ) );
  CombatBraveAttackColony const combat =
      ts.combat.brave_attack_colony( attacker, defender,
                                     colony );
  IEuroMind&    euro_mind  = ts.euro_minds[colony.nation];
  e_tribe const tribe_type = tribe_type_for_unit( ss, attacker );
  Visibility const viz(
      ss, player_for_role( ss, e_player_role::viewer ) );
  Coord const src = ss.units.coord_for( attacker.id );
  Coord const dst = colony.location;

  // Note that for attacks the "show indian moves" game flag is
  // not relevant, since there is really no natural way to show
  // an attack without the slide and depixelation animations, es-
  // pecially if it results in a unit or colony disappearing.
  bool const viewable = should_animate_move( viz, src, dst );

  if( viewable ) {
    co_await ts.planes.land_view().ensure_visible( src );
    co_await ts.planes.land_view().ensure_visible( dst );
  }

  co_await surprise_raid_msg( ss, euro_mind, colony.location,
                              tribe_type );
  if( !offboarded.empty() )
    co_await euro_mind.message_box(
        "Colonists on ships docked in [{}] have been offboarded "
        "to help defend the colony!",
        colony.name );

  if( viewable )
    co_await ts.planes.land_view().animate(
        anim_seq_for_brave_attack_colony( ss, combat ) );

  if( combat.colony_destroyed )
    co_await raid_colony_burn( ss, ts, attacker, colony,
                               tribe_type, combat );
  else
    co_await raid_colony_battle( ss, ts, attacker, colony,
                                 tribe_type, combat );
}

} // namespace rn
