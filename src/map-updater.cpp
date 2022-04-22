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

/****************************************************************
** MapUpdater
*****************************************************************/
void MapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) const {
  mutator( terrain_state_.mutable_square_at( tile ) );

  auto& renderer = renderer_;
  SCOPED_RENDERER_MOD( buffer_mods.buffer,
                       rr::e_render_target_buffer::landscape );

  // Re-render the square and all adjacent squares.
  for( e_direction d : refl::enum_values<e_direction> )
    if( terrain_state_.square_exists( tile.moved( d ) ) )
      render_terrain_square( terrain_state_, renderer_,
                             tile * g_tile_scale,
                             tile.moved( d ) );
}

void MapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator )
    const {
  mutator( terrain_state_.mutable_world_map() );
  render_terrain( terrain_state_, renderer_ );
}

void MapUpdater::just_redraw_map() const {
  render_terrain( terrain_state_, renderer_ );
}

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
void NonRenderingMapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) const {
  mutator( terrain_state_.mutable_square_at( tile ) );
}

void NonRenderingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator )
    const {
  mutator( terrain_state_.mutable_world_map() );
}

void NonRenderingMapUpdater::just_redraw_map() const {}

} // namespace rn
