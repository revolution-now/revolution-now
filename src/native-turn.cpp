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

bool should_animate_native_travel( SSConst const& ss, Coord src,
                                   Coord dst ) {
  if( !ss.settings.game_options
           .flags[e_game_flag_option::show_indian_moves] )
    return false;
  // The player want's to see animations of native units. So now
  // we just check if the unit is actually visible to the viewing
  // player.
  maybe<e_nation> const viewer =
      player_for_role( ss, e_player_role::viewer );
  if( !viewer.has_value() )
    // No viewer is interpreted as "everything visible."
    return true;
  Visibility const viz( ss, *viewer );

  bool const src_clear =
      viz.visible( src ) == e_tile_visibility::visible_and_clear;
  bool const dst_clear =
      viz.visible( dst ) == e_tile_visibility::visible_and_clear;
  return src_clear || dst_clear;
}

wait<> handle_native_unit_command(
    SS& ss, TS& ts, NativeUnit& native_unit,
    NativeUnitCommand const& command ) {
  SWITCH( command ) {
    CASE( forfeight ) {
      native_unit.movement_points = 0;
      break;
    }
    CASE( attack ) {
      // TODO: if the unit is lost here then it needs to be re-
      // moved from the map.
      co_await ts.gui.message_box( "Brave attack." );
      co_await ts.planes.land_view().animate(
          anim_seq_for_unit_move( native_unit.id,
                                  attack.direction ) );
      native_unit.movement_points = 0;
      break;
    }
    CASE( travel ) {
      Coord const src = ss.units.coord_for( native_unit.id );
      Coord const dst = src.moved( travel.direction );
      CHECK_GT( travel.consumed, 0 );
      native_unit.movement_points -= travel.consumed;
      CHECK_GE( native_unit.movement_points, 0 );

      if( should_animate_native_travel( ss, src, dst ) )
        co_await ts.planes.land_view().animate(
            anim_seq_for_unit_move( native_unit.id,
                                    travel.direction ) );
      co_await UnitOnMapMover::native_unit_to_map_interactive(
          ss, ts, native_unit.id, dst,
          ss.units.dwelling_for( native_unit.id ) );
      break;
    }
    END_CASES;
  }

  // !! Note that the unit may no longer exist here.
}

wait<> tribe_turn( SS& ss, TS& ts, INativeMind& mind,
                   set<NativeUnitId>& units ) {
  while( !units.empty() ) {
    NativeUnitId const native_unit_id =
        mind.select_unit( as_const( units ) );
    CHECK( units.contains( native_unit_id ) );
    NativeUnit& native_unit =
        ss.units.unit_for( native_unit_id );
    CHECK_GT( native_unit.movement_points, 0 );
    NativeUnitCommand const command =
        mind.command_for( native_unit_id );
    co_await handle_native_unit_command( ss, ts, native_unit,
                                         command );

    // !! Unit may no longer exist at this point.
    if( !ss.units.exists( native_unit_id ) ) {
      units.erase( native_unit_id );
      continue;
    }

    // Should be last.
    if( native_unit.movement_points == 0 )
      units.erase( native_unit_id );
  }

  co_return;
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> natives_turn( SS& ss, TS& ts ) {
  base::ScopedTimer timer( "native turns" );
  timer.options().no_checkpoints_logging = true;
  // Evolve dwellings. NOTE: We need to do this first for all
  // tribes so that when we get to the units phase, all of the
  // units that will be created this turn have been created, so
  // that way we only have to iterate over them once to group
  // them into their respective tribes.
  // TODO

  // Collect all native units for each tribe. At this point in
  // the turn no further native units should be created, since
  // we've already evolved the dwellings. However, native units
  // can be lost.
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

  for( e_tribe const tribe : refl::enum_values<e_tribe> ) {
    INativeMind& mind = ts.native_minds[tribe];
    if( !ss.natives.tribe_exists( tribe ) ) continue;
    timer.checkpoint( "{}", tribe );
    co_await tribe_turn( ss, ts, mind, tribe_to_units[tribe] );
    CHECK( tribe_to_units[tribe].empty() );
  }
}

} // namespace rn
