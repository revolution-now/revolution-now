/****************************************************************
**native-turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Implements the logic needed to run a native
*              tribe's turn independent of any particular AI
*              model.  It just enforces game rules.
*
*****************************************************************/
#include "native-turn.hpp"

// Revolution Now
#include "agents.hpp"
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "ieuro-agent.hpp"
#include "inative-agent.hpp"
#include "iraid.rds.hpp"
#include "itribe-evolve.rds.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "mv-calc.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "roles.hpp"
#include "show-anim.hpp"
#include "society.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// config
#include "config/natives.hpp"
#include "config/text.rds.hpp" // FIXME

// ss
#include "ss/colonies.hpp"
#include "ss/native-unit.rds.hpp"
#include "ss/natives.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/terrain.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/coord.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/timer.hpp"

// C++ standard library
#include <map>

using namespace std;

namespace rn {

namespace {

wait<> handle_native_unit_attack( SS& ss, TS& ts,
                                  IRaid const& raid,
                                  NativeUnit& native_unit,
                                  e_direction direction,
                                  e_player const player_type ) {
  // In case there are multiple human players.
  ScopedMapViewer const _( ss, ts, player_type );

  Coord const src      = ss.units.coord_for( native_unit.id );
  Coord const dst      = src.moved( direction );
  NativeUnit& attacker = native_unit;
  NativeUnitId const attacker_id = attacker.id;

  Player& player =
      player_for_player_or_die( ss.players, player_type );
  IEuroAgent& euro_agent = ts.euro_agents()[player_type];
  co_await show_woodcut_if_needed( player, euro_agent,
                                   e_woodcut::indian_raid );

  if( maybe<ColonyId> const colony_id =
          ss.colonies.maybe_from_coord( dst );
      colony_id.has_value() )
    co_await raid.raid_colony(
        attacker, ss.colonies.colony_for( *colony_id ) );
  else
    co_await raid.raid_unit( attacker, dst );

  // !! Native unit may no longer exist here.
  if( !ss.units.exists( attacker_id ) ) co_return;

  native_unit.movement_points = 0;
}

// TODO: temporary.
wait<> handle_native_unit_talk( SS& ss, TS& ts,
                                NativeUnit& native_unit,
                                e_direction direction,
                                e_player const player_type ) {
  // In case there are multiple human players.
  ScopedMapViewer const _( ss, ts, player_type );

  Player& player =
      player_for_player_or_die( ss.players, player_type );
  IEuroAgent& euro_agent = ts.euro_agents()[player_type];

  if( auto const seq = anim_seq_for_unit_talk(
          ss, native_unit.id, direction );
      should_animate_seq( ss, seq ) )
    co_await ts.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );

  player.money += 5;

  // TODO: create a new module native-talk here to handle all the
  // ways that natives can interact other than attacking.
  string const& tribe_name =
      config_natives
          .tribes[tribe_type_for_unit( ss, native_unit )]
          .name_singular;
  co_await euro_agent.message_box(
      "You've received [5{}] from the [{}].",
      config_text.special_chars.currency, tribe_name );

  native_unit.movement_points = 0;
}

wait<> handle_native_unit_travel( SS& ss, TS& ts,
                                  NativeUnit& native_unit,
                                  e_direction direction ) {
  Coord const src = ss.units.coord_for( native_unit.id );
  Coord const dst = src.moved( direction );
  MovementPoints const needed = movement_points_required(
      ss.terrain.square_at( src ), ss.terrain.square_at( dst ),
      direction );
  MovementPointsAnalysis const mv_analysis =
      can_native_unit_move_based_on_mv_points( ts, native_unit,
                                               needed );
  // Note that the AI may select a move that ends up not being
  // allowed on the basis of movement points since the unit may
  // have less movement points that required and hence it comes
  // down to probabilitity.
  MovementPoints const to_subtract =
      mv_analysis.points_to_subtract();
  CHECK_GT( to_subtract, 0 );
  native_unit.movement_points -= to_subtract;
  CHECK_GE( native_unit.movement_points, 0 );
  if( !mv_analysis.allowed() ) {
    CHECK_EQ( native_unit.movement_points, 0 );
    co_return;
  }
  if( auto const seq = anim_seq_for_unit_move(
          ss, native_unit.id, direction );
      should_animate_seq( ss, seq ) )
    co_await ts.planes.get()
        .get_bottom<ILandViewPlane>()
        .animate( seq );
  co_await UnitOnMapMover::native_unit_to_map_interactive(
      ss, ts, native_unit.id, dst );
}

wait<> handle_native_unit_command(
    SS& ss, TS& ts, IRaid const& raid, e_tribe tribe_type,
    NativeUnit& native_unit, NativeUnitCommand const& command ) {
  SWITCH( command ) {
    CASE( forfeight ) {
      native_unit.movement_points = 0;
      break;
    }
    CASE( equip ) {
      //  According to game rules, this is only allowed when the
      //  brave is sitting over a dwelling.
      CHECK( ss.natives.tribe_type_for(
                 ss.natives.dwelling_from_coord(
                     ss.units.coord_for( native_unit.id ) ) ) ==
                 tribe_type,
             "a tribe cannot equip a brave with muskets or "
             "horses unless it is sitting atop a dwelling." );
      Tribe& tribe = ss.natives.tribe_for( tribe_type );
      tribe.muskets += equip.how.muskets_delta;
      tribe.horse_breeding += equip.how.horse_breeding_delta;
      native_unit.type = equip.how.type;
      // In the OG a brave appears to forfeight its movement
      // points when being equipped.
      native_unit.movement_points = 0;
      CHECK_GE( tribe.muskets, 0 );
      CHECK_GE( tribe.horse_breeding, 0 );
      break;
    }
    CASE( move ) {
      Coord const src = ss.units.coord_for( native_unit.id );
      Coord const dst = src.moved( move.direction );
      maybe<Society> const society =
          society_on_square( ss, dst );
      if( !society.has_value() ) {
        co_await handle_native_unit_travel( ss, ts, native_unit,
                                            move.direction );
      } else {
        SWITCH( *society ) {
          CASE( native ) {
            CHECK( native.tribe == tribe_type );
            co_await handle_native_unit_travel(
                ss, ts, native_unit, move.direction );
            break;
          }
          CASE( european ) {
            co_await handle_native_unit_attack(
                ss, ts, raid, native_unit, move.direction,
                european.player );
            break;
          }
        }
      }
      break;
    }
    CASE( talk ) {
      //  TODO: this is just temporary.
      Coord const src = ss.units.coord_for( native_unit.id );
      Coord const dst = src.moved( talk.direction );
      UNWRAP_CHECK( society, society_on_square( ss, dst ) );
      SWITCH( society ) {
        CASE( native ) { SHOULD_NOT_BE_HERE; }
        CASE( european ) {
          co_await handle_native_unit_talk( ss, ts, native_unit,
                                            talk.direction,
                                            european.player );
          break;
        }
      }
    }
    break;
  }

  // !! Note that the unit may no longer exist here.
}

wait<> tribe_turn( SS& ss, TS& ts, INativeAgent& agent,
                   IRaid const& raid,
                   ITribeEvolve const& tribe_evolver ) {
  // Evolve those aspects/properties of the tribe that are common
  // to the entire tribe, i.e. not dwellingor unit-specific.
  tribe_evolver.evolve_tribe_common( agent.tribe_type() );

  // Evolve non-unit aspects of the tribe. This must be done be-
  // fore moving the units because it may result in a brave get-
  // ting created.
  tribe_evolver.evolve_dwellings_for_tribe( agent.tribe_type() );

  // Gather all units. This must be done after evolving the
  // dwellings so that it includes any new units that are cre-
  // ated. At this point in the turn no further native units
  // should be created, since we've already evolved the
  // dwellings. However, native units can be lost. We use a
  // std::set here for two reasons: 1) we get deterministic iter-
  // ation over the units, and 2) it turned out to be twice as
  // fast on large maps when profiled, surprisingly.
  set<NativeUnitId> units =
      units_for_tribe_ordered( ss, agent.tribe_type() );

  // Note: unlike for european units, it is ok to do this reset-
  // ting of the movement points here because we know that the
  // game will never be saved and reloaded during the natives'
  // turn.
  for( NativeUnitId const id : units ) {
    NativeUnit& unit = ss.units.unit_for( id );
    unit.movement_points =
        unit_attr( unit.type ).movement_points;
  }

  // As a circuit breaker to prevent the AI from never exhausting
  // all of the movement points of all of its units, we'll give
  // the AI a maximum of 100 tries per unit. If it can't finish
  // all unit movements in that time, then there is probably
  // something wrong with the AI.
  unordered_map<NativeUnitId, int> tries_for_unit;
  tries_for_unit.reserve( units.size() );
  int const kMaxTries = 100;
  // Twelve is chosen for this because, in practice, it should be
  // the upper limit for number of moves, since it represents the
  // max number of moves that a brave could make, which happens
  // when the unit has 4 movement points (mounted brave or
  // mounted warrior) and it is traveling entirely along a road.
  int const kTriesWarn = 12;
  while( !units.empty() ) {
    NativeUnitId const native_unit_id =
        agent.select_unit( as_const( units ) );
    CHECK( units.contains( native_unit_id ) );
    NativeUnit& native_unit =
        ss.units.unit_for( native_unit_id );
    CHECK_GT( native_unit.movement_points, 0 );
    int& tries = tries_for_unit[native_unit_id];
    if( tries >= kTriesWarn )
      lg.warn(
          "the AI took more than {} attempts to exhaust the "
          "movement points of native unit {}.",
          kTriesWarn, native_unit );
    CHECK( tries++ < kMaxTries,
           "the AI had {} attempts to exhaust the movement "
           "points of unit {} but did not do so.",
           kMaxTries, native_unit_id );

    co_await handle_native_unit_command(
        ss, ts, raid, agent.tribe_type(), native_unit,
        agent.command_for( native_unit_id ) );

    // !! Unit may no longer exist at this point.
    if( !ss.units.exists( native_unit_id ) ) {
      units.erase( native_unit_id );
      continue;
    }

    if( native_unit.movement_points == 0 )
      units.erase( native_unit_id );
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> natives_turn( SS& ss, TS& ts, IRaid const& raid,
                     ITribeEvolve const& tribe_evolver ) {
  base::ScopedTimer timer( "native turns" );
  timer.options().no_checkpoints_logging = true;

  for( e_tribe const tribe : refl::enum_values<e_tribe> ) {
    if( !ss.natives.tribe_exists( tribe ) ) continue;
    INativeAgent& agent = ts.native_agents()[tribe];
    timer.checkpoint( "{}", tribe );
    co_await tribe_turn( ss, ts, agent, raid, tribe_evolver );
  }
}

} // namespace rn
