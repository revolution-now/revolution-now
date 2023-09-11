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
#include "anim-builders.hpp"
#include "co-wait.hpp"
#include "igui.hpp"
#include "inative-mind.hpp"
#include "land-view.hpp"
#include "map-square.hpp"
#include "minds.hpp"
#include "mv-calc.hpp"
#include "on-map.hpp"
#include "plane-stack.hpp"
#include "roles.hpp"
#include "ts.hpp"
#include "visibility.hpp"

// config
#include "config/natives.hpp"

// ss
#include "ss/native-unit.rds.hpp"
#include "ss/natives.hpp"
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
#include "base/timer.hpp"

// C++ standard library
#include <map>

using namespace std;

namespace rn {

namespace {

bool should_animate_native_move( SSConst const&    ss,
                                 Visibility const& viz,
                                 Coord src, Coord dst ) {
  if( !ss.settings.game_options
           .flags[e_game_flag_option::show_indian_moves] )
    return false;
  // At the time of writing, the land view renderer will suppress
  // any slide/attack animations where the src and dst tiles are
  // both not visible, so technically we don't have to do this
  // here. However, this is an optimization that hopefully makes
  // a difference for large maps where there are many native
  // units; it will prevent an animation API from getting called
  // for each of the braves, most of which are not visible.
  return should_animate_move( viz, src, dst );
}

wait<> handle_native_unit_command(
    SS& ss, TS& ts, Visibility const& viz,
    NativeUnit& native_unit, NativeUnitCommand const& command ) {
  SWITCH( command ) {
    CASE( forfeight ) {
      native_unit.movement_points = 0;
      break;
    }
    CASE( attack ) {
      Coord const src = ss.units.coord_for( native_unit.id );
      Coord const dst = src.moved( attack.direction );
      if( should_animate_native_move( ss, viz, src, dst ) )
        co_await ts.planes.land_view().animate(
            anim_seq_for_unit_move( native_unit.id,
                                    attack.direction ) );
      // Carry out attack.
      // !! Native unit may no longer exist here.
      native_unit.movement_points = 0;
      break;
    }
    CASE( travel ) {
      Coord const src = ss.units.coord_for( native_unit.id );
      Coord const dst = src.moved( travel.direction );
      MovementPoints const needed = movement_points_required(
          ss.terrain.square_at( src ),
          ss.terrain.square_at( dst ), travel.direction );
      MovementPointsAnalysis const mv_analysis =
          can_native_unit_move_based_on_mv_points(
              ts, native_unit, needed );
      MovementPoints const to_subtract =
          mv_analysis.points_to_subtract();
      CHECK_GT( to_subtract, 0 );
      native_unit.movement_points -= to_subtract;
      CHECK_GE( native_unit.movement_points, 0 );
      if( !mv_analysis.allowed() ) {
        CHECK_EQ( native_unit.movement_points, 0 );
        break;
      }
      if( should_animate_native_move( ss, viz, src, dst ) )
        co_await ts.planes.land_view().animate(
            anim_seq_for_unit_move( native_unit.id,
                                    travel.direction ) );
      co_await UnitOnMapMover::native_unit_to_map_interactive(
          ss, ts, native_unit.id, dst,
          ss.units.dwelling_for( native_unit.id ) );
      break;
    }
  }

  // !! Note that the unit may no longer exist here.
}

wait<> tribe_turn( SS& ss, TS& ts, Visibility const& viz,
                   INativeMind&       mind,
                   set<NativeUnitId>& units ) {
  // As a safety check to prevent the AI from never exhausting
  // all of the movement points of all of its units, we'll give
  // the AI a maximum of 100 tries per unit. If it can't finish
  // all unit movements in that time, then there is probably
  // something wrong with the AI.
  int       tries     = 0;
  int const max_tries = 100;
  while( !units.empty() ) {
    NativeUnitId const native_unit_id =
        mind.select_unit( as_const( units ) );
    CHECK( tries++ < max_tries,
           "the AI had {} attempts to exhaust the movement "
           "points of unit {} but did not do so.",
           max_tries, native_unit_id );
    CHECK( units.contains( native_unit_id ) );
    NativeUnit& native_unit =
        ss.units.unit_for( native_unit_id );
    CHECK_GT( native_unit.movement_points, 0 );
    NativeUnitCommand const command =
        mind.command_for( native_unit_id );
    co_await handle_native_unit_command( ss, ts, viz,
                                         native_unit, command );

    // !! Unit may no longer exist at this point.
    if( !ss.units.exists( native_unit_id ) ) {
      units.erase( native_unit_id );
      continue;
    }

    if( native_unit.movement_points == 0 )
      units.erase( native_unit_id );

    // Should be last.
    if( !units.contains( native_unit_id ) ) tries = 0;
  }
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> natives_turn( SS& ss, TS& ts ) {
  base::ScopedTimer timer( "native turns" );
  timer.options().no_checkpoints_logging = true;

  // TODO: Evolve dwellings. Note: We need to do this first for
  // all tribes so that when we get to the units phase, all of
  // the units that will be created this turn have been created,
  // so that way we only have to iterate over them once to group
  // them into their respective tribes.

  // Collect all native units for each tribe. At this point in
  // the turn no further native units should be created, since
  // we've already evolved the dwellings. However, native units
  // can be lost. We use a std::set here for two reasons: 1) we
  // get deterministic iteration over the units, and 2) it turned
  // out to be twice as fast on large maps when profiled, sur-
  // prisingly.
  auto const& native_units = ss.units.native_all();
  map<e_tribe, set<NativeUnitId>> tribe_to_units;

  timer.checkpoint( "gathering units" );
  for( auto [unit_id, p_state] : native_units ) {
    NativeUnit& unit = ss.units.unit_for( unit_id );
    // Note: unlike for european units, it is ok to do this re-
    // setting of the movement points here because we know that
    // the game will never be saved and reloaded during the na-
    // tives' turn.
    unit.movement_points =
        unit_attr( unit.type ).movement_points;
    UNWRAP_CHECK( world,
                  p_state->ownership
                      .get_if<NativeUnitOwnership::world>() );
    e_tribe const tribe_type =
        ss.natives.tribe_for( world.dwelling_id ).type;
    auto& units = tribe_to_units[tribe_type];
    CHECK( !units.contains( unit_id ),
           "something has gone wrong: {} encountered twice.",
           unit_id );
    units.insert( unit_id );
  }

  Visibility const viz(
      ss, player_for_role( ss, e_player_role::viewer ) );

  for( e_tribe const tribe : refl::enum_values<e_tribe> ) {
    if( !ss.natives.tribe_exists( tribe ) ) continue;
    INativeMind& mind = ts.native_minds[tribe];
    timer.checkpoint( "{}", tribe );
    co_await tribe_turn( ss, ts, viz, mind,
                         tribe_to_units[tribe] );
    CHECK( tribe_to_units[tribe].empty() );
  }
}

} // namespace rn
