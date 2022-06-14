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
#include "logger.hpp"
#include "render-terrain.hpp"
#include "tiles.hpp"

using namespace std;

namespace rn {

/****************************************************************
** MapUpdater
*****************************************************************/
void MapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) {
  MapSquare old_square = terrain_state_.square_at( tile );
  mutator( terrain_state_.mutable_square_at( tile ) );
  MapSquare new_square = terrain_state_.square_at( tile );
  if( new_square == old_square )
    // Return before beginning the rendering, that way we a)
    // won't add more vertices to the landscape buffer and b) we
    // won't trigger the buffer to get re-uploaded to the GPU.
    // This really helps with efficiency in the map editor.
    return;

  auto& renderer = renderer_;
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );
  SCOPED_RENDERER_MOD_SET(
      buffer_mods.buffer,
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

  ++tiles_updated_;
  // If we've updated 50 tiles then we've redrawn 50*5*5=1250
  // squares, so probably a good time to just redraw the entire
  // thing. In fact we need to do this because otherwise the or-
  // dering of the renderered tiles in the buffer (and/or the
  // fact that they overlap but are in different places in the
  // buffer) appears to significantly slow down rendering. This
  // will renormalize the buffer.
  if( tiles_updated_ == 50 ) just_redraw_map();
}

void MapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  mutator( terrain_state_.mutable_world_map() );
  just_redraw_map();
}

void MapUpdater::just_redraw_map() {
  render_terrain( terrain_state_, renderer_ );
  // Reset this since we just redrew the map.
  tiles_updated_ = 0;
}

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
void NonRenderingMapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) {
  mutator( terrain_state_.mutable_square_at( tile ) );
}

void NonRenderingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  mutator( terrain_state_.mutable_world_map() );
}

void NonRenderingMapUpdater::just_redraw_map() {}

/****************************************************************
** TrappingMapUpdater
*****************************************************************/
void TrappingMapUpdater::modify_map_square(
    Coord, base::function_ref<void( MapSquare& )> ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::just_redraw_map() {}

} // namespace rn
