/****************************************************************
**road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: Road rendering and state changes.
*
*****************************************************************/
#include "road.hpp"

// Revolution Now
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "map-updater.hpp"
#include "renderer.hpp" // FIXME: remove
#include "tiles.hpp"

// render
#include "render/painter.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// C++ standard library
#include <array>

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Road State
*****************************************************************/
void set_road( IMapUpdater const& map_updater, Coord tile ) {
  map_updater.modify_map_square(
      tile, []( MapSquare& square ) { square.road = true; } );
}

void clear_road( IMapUpdater const& map_updater, Coord tile ) {
  map_updater.modify_map_square(
      tile, []( MapSquare& square ) { square.road = false; } );
}

bool has_road( TerrainState const& terrain_state, Coord tile ) {
  MapSquare const& square = terrain_state.square_at( tile );
  return square.road;
}

/****************************************************************
** Unit State
*****************************************************************/
void perform_road_work( UnitsState const&   units_state,
                        TerrainState const& terrain_state,
                        IMapUpdater const&  map_updater,
                        Unit&               unit ) {
  Coord location = units_state.coord_for( unit.id() );
  CHECK( unit.orders() == e_unit_orders::road );
  CHECK( unit.type() == e_unit_type::pioneer ||
             unit.type() == e_unit_type::hardy_pioneer,
         "unit type {} should not be building a road.",
         unit.type() );
  CHECK_GT( unit.movement_points(), 0 );
  auto log = [&]( string_view status ) {
    lg.debug( "road work {} for unit {} with {} tools left.",
              status, debug_string( unit ),
              unit.composition()[e_unit_inventory::tools] );
  };

  // First check if there is already a road on this square.
  // This could happen if there are two units building a road
  // on the same square and the other unit finished first. In
  // that case, we will just clear this unit's orders and not
  // charge it any tools.
  if( has_road( terrain_state, location ) ) {
    log( "cancelled" );
    unit.clear_orders();
    unit.set_turns_worked( 0 );
    return;
  }
  // The unit is still building the road.
  int turns_worked = unit.turns_worked();
  UNWRAP_CHECK( road_turns, unit.desc().road_turns );
  CHECK_LE( turns_worked, road_turns );
  if( turns_worked == road_turns ) {
    // We're finished building the road.
    set_road( map_updater, location );
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

bool can_build_road( Unit const& unit ) {
  return unit.desc().road_turns.has_value();
}

/****************************************************************
** Rendering
*****************************************************************/
void render_road_if_present( rr::Painter& painter, Coord where,
                             TerrainState const& terrain_state,
                             Coord               world_tile ) {
  if( !has_road( terrain_state, world_tile ) ) return;

  static constexpr array<pair<e_direction, e_tile>, 8> const
      road_tiles{
          pair{ e_direction::nw, e_tile::road_nw },
          pair{ e_direction::n, e_tile::road_n },
          pair{ e_direction::ne, e_tile::road_ne },
          pair{ e_direction::e, e_tile::road_e },
          pair{ e_direction::se, e_tile::road_se },
          pair{ e_direction::s, e_tile::road_s },
          pair{ e_direction::sw, e_tile::road_sw },
          pair{ e_direction::w, e_tile::road_w },
      };

  bool road_in_surroundings = false;
  for( auto [direction, tile] : road_tiles ) {
    Coord shifted = world_tile.moved( direction );
    if( !terrain_state.square_exists( shifted ) ||
        !has_road( terrain_state, shifted ) )
      continue;
    road_in_surroundings = true;
    render_sprite( painter, where, tile );
  }
  if( !road_in_surroundings )
    render_sprite( painter, where, e_tile::road_island );
}

/****************************************************************
** Lua
*****************************************************************/
namespace {

LUA_FN( set_road, void, Coord tile ) {
  TerrainState const& terrain_state = GameState::terrain();
  if( !terrain_state.is_land( tile ) )
    st.error( "cannot put road on water tile {}.", tile );
  set_road( MapUpdater( GameState::terrain(),
                        global_renderer_use_only_when_needed() ),
            tile );
}

LUA_FN( clear_road, void, Coord tile ) {
  clear_road(
      MapUpdater( GameState::terrain(),
                  global_renderer_use_only_when_needed() ),
      tile );
}

} // namespace

} // namespace rn
