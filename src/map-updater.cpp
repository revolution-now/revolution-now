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
  SCOPED_RENDERER_MOD( painter_mods.repos.use_camera, true );
  SCOPED_RENDERER_MOD( buffer_mods.buffer,
                       rr::e_render_target_buffer::landscape );

  // Re-render the square and all adjacent squares. Actually, we
  // need to do two levels of adjacency because some water tiles
  // can derive their ground terrain from their neighbors, and
  // those in turn can affect their neighbors. Though changes of
  // this kind only happen in the map editor.
  Rect to_update = Rect::from( tile - Delta( 2_w, 2_h ),
                               tile + Delta( 3_w, 3_h ) );
  for( Coord moved : to_update ) {
    if( !terrain_state_.square_exists( moved ) ) continue;
    render_terrain_square( terrain_state_, renderer_,
                           moved * g_tile_scale, moved );
  }
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
