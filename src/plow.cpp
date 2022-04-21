/****************************************************************
**plow.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Plowing rendering and state changes.
*
*****************************************************************/
#include "plow.hpp"

// Revolution Now
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "renderer.hpp" // FIXME: remove
#include "terrain.hpp"
#include "tiles.hpp"

// render
#include "render/painter.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

void clear_irrigation( IMapUpdater const& map_updater,
                       Coord              tile ) {
  map_updater.modify_map_square( tile, []( MapSquare& square ) {
    square.irrigation = false;
  } );
}

void irrigate( IMapUpdater const& map_updater, Coord tile ) {
  map_updater.modify_map_square(
      tile, []( MapSquare& square ) { irrigate( square ); } );
}

void clear_forest( IMapUpdater const& map_updater, Coord tile ) {
  map_updater.modify_map_square( tile, []( MapSquare& square ) {
    clear_forest( square );
  } );
}

} // namespace

/****************************************************************
** Plowing State
*****************************************************************/
void plow_square( TerrainState const& terrain_state,
                  IMapUpdater const& map_updater, Coord tile ) {
  CHECK( terrain_state.is_land( tile ) );
  MapSquare const& square = terrain_state.square_at( tile );
  if( has_forest( square ) ) {
    clear_forest( map_updater, tile );
    return;
  }
  CHECK( !square.irrigation,
         "tile {} already has irrigation and thus cannot be "
         "plowed.",
         tile );
  if( can_irrigate( square ) ) {
    irrigate( map_updater, tile );
    return;
  }
  FATAL( "terrain type {} cannot be plowed: square={}",
         effective_terrain( square ), square );
}

bool can_plow( TerrainState const& terrain_state, Coord tile ) {
  MapSquare const& square = terrain_state.square_at( tile );
  return can_plow( square );
}

bool can_irrigate( TerrainState const& terrain_state,
                   Coord               tile ) {
  MapSquare const& square = terrain_state.square_at( tile );
  return can_irrigate( square );
}

bool has_irrigation( TerrainState const& terrain_state,
                     Coord               tile ) {
  MapSquare const& square = terrain_state.square_at( tile );
  return square.irrigation;
}

/****************************************************************
** Unit State
*****************************************************************/
void perform_plow_work( UnitsState const&   units_state,
                        TerrainState const& terrain_state,
                        IMapUpdater const&  map_updater,
                        Unit&               unit ) {
  Coord location = units_state.coord_for( unit.id() );
  CHECK( unit.orders() == e_unit_orders::plow );
  CHECK( unit.type() == e_unit_type::pioneer ||
             unit.type() == e_unit_type::hardy_pioneer,
         "unit type {} should not be plowing.", unit.type() );
  CHECK_GT( unit.movement_points(), 0 );
  auto log = [&]( string_view status ) {
    lg.debug( "plow work {} for unit {} with {} tools left.",
              status, debug_string( unit ),
              unit.composition()[e_unit_inventory::tools] );
  };

  // First check if there is already irrigation on this square.
  // This could happen if there are two units plowing on the same
  // square and the other unit finished first. In that case, we
  // will just clear this unit's orders and not charge it any
  // tools.
  if( has_irrigation( terrain_state, location ) ) {
    log( "cancelled" );
    unit.clear_orders();
    unit.set_turns_worked( 0 );
    return;
  }
  // The unit is still plowing.
  int turns_worked = unit.turns_worked();
  UNWRAP_CHECK( plow_turns, unit.desc().plow_turns );
  CHECK_LE( turns_worked, plow_turns );
  if( turns_worked == plow_turns ) {
    // We're finished plowing.
    plow_square( terrain_state, map_updater, location );
    unit.clear_orders();
    unit.set_turns_worked( 0 );
    unit.consume_20_tools();
    log( "finished" );
    return;
  }
  // We need more work.
  log( "ongoing" );
  unit.forfeight_mv_points();
  unit.set_turns_worked( turns_worked + 1 );
}

bool can_plow( Unit const& unit ) {
  return unit.desc().plow_turns.has_value();
}

/****************************************************************
** Rendering
*****************************************************************/
void render_plow_if_present( rr::Painter& painter, Coord where,
                             TerrainState const& terrain_state,
                             Coord               world_tile ) {
  if( !has_irrigation( terrain_state, world_tile ) ) return;
  render_sprite( painter, where, e_tile::irrigation );
}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_FN( plow_square, void, Coord tile ) {
  TerrainState& terrain_state = GameState::terrain();
  if( !terrain_state.is_land( tile ) )
    st.error( "cannot plow on water tile {}.", tile );
  plow_square(
      terrain_state,
      MapUpdater( terrain_state,
                  // FIXME
                  global_renderer_use_only_when_needed() ),
      tile );
}

LUA_FN( clear_irrigation, void, Coord tile ) {
  clear_irrigation(
      MapUpdater( GameState::terrain(),
                  // FIXME
                  global_renderer_use_only_when_needed() ),
      tile );
}

} // namespace

} // namespace rn
