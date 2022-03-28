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
#include "terrain.hpp"
#include "tiles.hpp"
#include "world-map.hpp"

// render
#include "render/painter.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Plowing State
*****************************************************************/
void plow_square( TerrainState& terrain_state, Coord tile ) {
  CHECK( is_land( terrain_state, tile ) );
  MapSquare& square = square_at( terrain_state, tile );
  if( maybe<e_terrain> cleared =
          cleared_forest( square.terrain );
      cleared.has_value() ) {
    square.terrain = *cleared;
    return;
  }
  if( can_irrigate( square.terrain ) ) {
    square.irrigation = true;
    return;
  }
  FATAL( "terrain type {} cannot be plowed.", square.terrain );
}

bool can_plow( TerrainState const& terrain_state, Coord tile ) {
  MapSquare const& square = square_at( terrain_state, tile );
  return can_plow( square.terrain ) && !square.irrigation;
}

bool can_irrigate( TerrainState const& terrain_state,
                   Coord               tile ) {
  MapSquare const& square = square_at( terrain_state, tile );
  return can_irrigate( square.terrain ) && !square.irrigation;
}

void clear_irrigation( TerrainState& terrain_state,
                       Coord         tile ) {
  MapSquare& square = square_at( terrain_state, tile );
  square.irrigation = false;
}

bool has_irrigation( TerrainState const& terrain_state,
                     Coord               tile ) {
  MapSquare const& square = square_at( terrain_state, tile );
  return square.irrigation;
}

/****************************************************************
** Unit State
*****************************************************************/
void perform_plow_work( UnitsState const& units_state,
                        TerrainState&     terrain_state,
                        Unit&             unit ) {
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
    // TODO: if we are clearing a forest then we should add a
    // certain amount of lumber to a nearby colony (see strategy
    // guide for formula).
    plow_square( terrain_state, location );
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
  if( !is_land( tile ) )
    st.error( "cannot plow on water tile {}.", tile );
  plow_square( GameState::terrain(), tile );
}

LUA_FN( clear_irrigation, void, Coord tile ) {
  clear_irrigation( GameState::terrain(), tile );
}

} // namespace

} // namespace rn
