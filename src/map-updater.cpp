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

namespace {

TerrainRenderOptions make_terrain_options(
    MapUpdaterOptions const& our_options ) {
  return TerrainRenderOptions{
      .render_forests   = our_options.render_forests,
      .render_resources = our_options.render_resources,
      .render_lcrs      = our_options.render_lcrs };
}

}

/****************************************************************
** MapUpdaterOptions
*****************************************************************/
namespace detail {

MapUpdaterOptionsPopper::~MapUpdaterOptionsPopper() noexcept {
  CHECK( !map_updater_.options_.empty() );
  map_updater_.options_.pop();
  CHECK( !map_updater_.options_.empty() );
  map_updater_.redraw();
}

} // namespace detail

/****************************************************************
** IMapUpdater
*****************************************************************/
IMapUpdater::IMapUpdater() {
  options_.push( MapUpdaterOptions{} );
}

IMapUpdater::Popper IMapUpdater::push_options_and_redraw(
    OptionsUpdateFunc mutator ) {
  CHECK( !options_.empty() );
  MapUpdaterOptions new_options = options_.top();
  mutator( new_options );
  options_.push( std::move( new_options ) );
  redraw();
  return Popper{ *this };
}

MapUpdaterOptions const& IMapUpdater::options() const {
  CHECK( !options_.empty() );
  return options_.top();
}

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
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  for( Coord moved : to_update ) {
    if( !terrain_state_.square_exists( moved ) ) continue;
    render_terrain_square( terrain_state_, renderer_,
                           moved * g_tile_scale, moved,
                           terrain_options );
  }

  ++tiles_updated_;
  // If we've updated 50 tiles then we've redrawn 50*5*5=1250
  // squares, so probably a good time to just redraw the entire
  // thing. In fact we need to do this because otherwise the or-
  // dering of the renderered tiles in the buffer (and/or the
  // fact that they overlap but are in different places in the
  // buffer) appears to significantly slow down rendering. This
  // will renormalize the buffer.
  if( tiles_updated_ == 50 ) redraw();
}

void MapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  mutator( terrain_state_.mutable_world_map() );
  redraw();
}

void MapUpdater::redraw() {
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  render_terrain( terrain_state_, renderer_, terrain_options );
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

void NonRenderingMapUpdater::redraw() {}

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

void TrappingMapUpdater::redraw() {}

} // namespace rn
