/****************************************************************
**map-updater.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Handlers when a map square needs to be modified.
*
*****************************************************************/
#include "map-updater.hpp"

// Revolution Now
#include "gs-terrain.hpp"
#include "render-terrain.hpp"
#include "tiles.hpp"

using namespace std;

namespace rn {

void modify_map_square(
    TerrainState& terrain_state, Coord tile,
    rr::Renderer&                          renderer,
    base::function_ref<void( MapSquare& )> mutator ) {
  mutator( terrain_state.mutable_square_at( tile ) );

  SCOPED_RENDERER_MOD( buffer_mods.buffer,
                       rr::e_render_target_buffer::landscape );

  // Re-render the square and all adjacent squares.
  for( e_direction d : refl::enum_values<e_direction> )
    if( terrain_state.square_exists( tile.moved( d ) ) )
      render_terrain_square( terrain_state, renderer,
                             tile * g_tile_scale,
                             tile.moved( d ) );
}

} // namespace rn
