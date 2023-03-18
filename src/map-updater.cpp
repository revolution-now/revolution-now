/****************************************************************
**map-updater.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-04.
*
* Description: Concrete implementations of IMapUpdater.
*
*****************************************************************/
#include "map-updater.hpp"

// Revolution Now
#include "logger.hpp"
#include "render-terrain.hpp"
#include "tiles.hpp"
#include "visibility.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// render
#include "render/renderer.hpp"

using namespace std;

namespace rn {

namespace {

TerrainRenderOptions make_terrain_options(
    MapUpdaterOptions const& our_options ) {
  return TerrainRenderOptions{
      .render_forests    = our_options.render_forests,
      .render_resources  = our_options.render_resources,
      .render_lcrs       = our_options.render_lcrs,
      .grid              = our_options.grid,
      .render_fog_of_war = our_options.render_fog_of_war };
}

}

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
NonRenderingMapUpdater::NonRenderingMapUpdater(
    SS& ss, MapUpdaterOptions const& initial_options )
  : IMapUpdater( initial_options ), ss_( ss ) {}

NonRenderingMapUpdater::NonRenderingMapUpdater( SS& ss )
  : NonRenderingMapUpdater( ss, MapUpdaterOptions{} ) {}

BuffersUpdated NonRenderingMapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) {
  MapSquare old_square = ss_.terrain.square_at( tile );
  // NOTE: This is the only non-testing place where this mutable
  // terrain should be used.
  mutator( ss_.mutable_terrain_use_with_care.mutable_square_at(
      tile ) );
  MapSquare      new_square = ss_.terrain.square_at( tile );
  BuffersUpdated buffers_updated;
  if( new_square == old_square ) return buffers_updated;

  // Update player maps.
  refl::enum_map<e_nation, bool> const visible_to_nations =
      nations_with_visibility_of_square( ss_, tile );
  for( auto [nation, visible] : visible_to_nations ) {
    if( !visible ) continue;
    BuffersUpdated const nation_buffers_updated =
        make_square_visible( tile, nation );
    if( options().nation != nation ) continue;
    buffers_updated = nation_buffers_updated;
  }

  return buffers_updated;
}

BuffersUpdated NonRenderingMapUpdater::make_square_visible(
    Coord tile, e_nation nation ) {
  PlayerTerrain& player_terrain =
      ss_.mutable_terrain_use_with_care.mutable_player_terrain(
          nation );
  Matrix<maybe<FogSquare>>& map = player_terrain.map;

  BuffersUpdated buffers_updated;

  // Unexplored status.
  bool const unexplored_before = !map[tile].has_value();
  if( unexplored_before ) {
    buffers_updated.landscape   = true;
    buffers_updated.obfuscation = true;
    map[tile].emplace();
  }

  FogSquare& fog_square         = *map[tile];
  bool const fog_present_before = !fog_square.fog_of_war_removed;
  fog_square.fog_of_war_removed = true;
  if( fog_present_before && options().render_fog_of_war )
    buffers_updated.obfuscation = true;

  MapSquare&       player_square = fog_square.square;
  MapSquare const& real_square   = ss_.terrain.square_at( tile );
  if( player_square != real_square ) {
    buffers_updated.landscape = true;
    player_square             = real_square;
  }

  return buffers_updated;
}

// Implement IMapUpdater.
BuffersUpdated NonRenderingMapUpdater::make_square_fogged(
    Coord tile, e_nation nation ) {
  PlayerTerrain& player_terrain =
      ss_.mutable_terrain_use_with_care.mutable_player_terrain(
          nation );
  Matrix<maybe<FogSquare>>& map = player_terrain.map;

  BuffersUpdated buffers_updated;
  if( !map[tile].has_value() ) return buffers_updated;
  FogSquare& fog_square = *map[tile];
  if( !fog_square.fog_of_war_removed ) return buffers_updated;

  fog_square.fog_of_war_removed = false;
  if( options().render_fog_of_war )
    buffers_updated.obfuscation = true;
  return buffers_updated;
}

void NonRenderingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  ss_.mutable_terrain_use_with_care.modify_entire_map( mutator );
}

void NonRenderingMapUpdater::redraw() {}

/****************************************************************
** RenderingMapUpdater
*****************************************************************/
RenderingMapUpdater::RenderingMapUpdater(
    SS& ss, rr::Renderer& renderer,
    MapUpdaterOptions const& initial_options )
  : NonRenderingMapUpdater( ss, initial_options ),
    renderer_( renderer ),
    landscape_tracking_( ss.terrain.world_size_tiles() ),
    obfuscation_tracking_( ss.terrain.world_size_tiles() ) {
  // Something is probably wrong if this happens.
  CHECK_GT( landscape_tracking_.tile_bounds.size().area(), 0 );
  CHECK_GT( obfuscation_tracking_.tile_bounds.size().area(), 0 );
}

// FIXME: The approach used here, which consists of rendering the
// entire map to one buffer, then redrawing individual tiles to
// the annex buffer (with zeroing of old vertices) with periodic
// redrawing, may not be ideal. Probably what is best and sim-
// plest is to just divide the map into chunks and give each its
// own buffer, and each time a tile changes in a chunk the entire
// chunk gets redrawn. This is simpler and also solves the one
// remaining issue with the current approach which is that peri-
// odically the entire map has to get redrawn, which is not ideal
// for large maps. The downside to the chunk approach will be
// that there will be more draw calls per frame, but that might
// not matter, since all the rest of the (non-map) drawing in the
// game is in a single draw call.
void RenderingMapUpdater::redraw_square_single_buffer(
    Coord tile, BufferTracking& buffer_tracking,
    rr::e_render_buffer        annex_buffer,
    base::function_ref<void()> render_square,
    base::function_ref<void()> redraw_buffer ) {
  auto& renderer = renderer_;
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );
  SCOPED_RENDERER_MOD_SET( buffer_mods.buffer, annex_buffer );
  rr::VertexRange const old_bounds =
      buffer_tracking.tile_bounds[tile];

  buffer_tracking.tile_bounds[tile] =
      renderer_.range_for( [&] { render_square(); } );

  // Now we zero out the vertices from the old tile. If we don't
  // do this then, over time, as we overwite a tile many times,
  // the tile accumulates so many renderable vertices that frame
  // rate significantly drops when a large number of screen
  // pixels are occupied by such tiles.
  renderer_.zap( old_bounds );

  // If we've redrawn a number of tiles that are beyond the below
  // threshold then we will just redraw the entire thing. We do
  // this for two reasons. First, each time a tile gets redrawn
  // (and the annex buffer gets appended to, the entire annex
  // buffer needs to get re-uploaded to the GPU, which would then
  // happen potentially on each move of a unit (where new terrain
  // is exposed). By redrawing the entire thing periodically we
  // clear the annex buffer and eliminate this latency. This
  // threshold is set somewhat arbitrarily, but it is set to a
  // value that is not so low as to cause a map redraw too often
  // (which will be slow on large maps) and not too large as to
  // let the annex buffer get too large (which would increase la-
  // tency of redrawing a tile by having to re-upload a large
  // annex buffer to the GPU).
  ++buffer_tracking.tiles_redrawn;
  // Around 20k total tiles redrawn seems to be a good number,
  // but we have two buffers (landscape and obfuscation) and so
  // we'll give them about 10k each.
  static int constexpr kRedrawThreshold = 10000;
  // The > is defensive.
  if( buffer_tracking.tiles_redrawn >= kRedrawThreshold )
    redraw_buffer();
  lg.trace( "{} tiles redrawn: {}",
            refl::enum_value_name( annex_buffer ),
            buffer_tracking.tiles_redrawn );
}

BuffersUpdated
RenderingMapUpdater::redraw_buffers_for_tile_where_needed(
    Coord tile, BuffersUpdated const& buffers_updated ) {
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );

  // Note: in the below, we need to draw the surrounding squares
  // because a visibility change in one square can reveal part of
  // the adjacent files even if they are not visible. In some
  // edge cases with map rendering we need to go two squares away
  // to properly rerender the map in response to a change in one
  // tile. However, those currently only happen when changing the
  // ground type of a tile with the map editor during the game,
  // and so it doesn't seem worth it to take the performance hit
  // of re-rendering an additional 16 tiles just to support that
  // case, which is not part of a normal game anyway.

  if( buffers_updated.landscape ) {
    for( e_cdirection d : refl::enum_values<e_cdirection> ) {
      Coord const moved = tile.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      redraw_square_single_buffer(
          moved, landscape_tracking_,
          rr::e_render_buffer::landscape_annex,
          [&] {
            render_landscape_square_if_not_fully_hidden(
                renderer_, moved * g_tile_delta, ss_, moved, viz,
                terrain_options );
          },
          [&] { redraw_landscape_buffer(); } );
    }
  }

  if( buffers_updated.obfuscation ) {
    for( e_cdirection d : refl::enum_values<e_cdirection> ) {
      Coord const moved = tile.moved( d );
      if( !ss_.terrain.square_exists( moved ) ) continue;
      redraw_square_single_buffer(
          moved, obfuscation_tracking_,
          rr::e_render_buffer::obfuscation_annex,
          [&] {
            render_obfuscation_overlay(
                renderer_, moved * g_tile_delta, moved, viz,
                terrain_options );
          },
          [&] { redraw_obfuscation_buffer(); } );
    }
  }

  return buffers_updated;
}

BuffersUpdated RenderingMapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) {
  BuffersUpdated const buffers_updated =
      this->Base::modify_map_square( tile, mutator );
  return redraw_buffers_for_tile_where_needed( tile,
                                               buffers_updated );
}

BuffersUpdated RenderingMapUpdater::make_square_visible(
    Coord const tile, e_nation nation ) {
  BuffersUpdated const buffers_updated =
      this->Base::make_square_visible( tile, nation );
  // If entire map is visible then there is no need to render.
  if( !options().nation.has_value() ) return BuffersUpdated{};
  // If it's another nation then not relevant for rendering.
  if( nation != *options().nation ) return BuffersUpdated{};
  return redraw_buffers_for_tile_where_needed( tile,
                                               buffers_updated );
}

BuffersUpdated RenderingMapUpdater::make_square_fogged(
    Coord tile, e_nation nation ) {
  BuffersUpdated const buffers_updated =
      this->Base::make_square_fogged( tile, nation );
  // If the entire map is visible then there is also no fog ren-
  // dered anywhere, so no need to re-render.
  if( !options().nation.has_value() ) return BuffersUpdated{};
  // If it's another nation then not relevant for rendering.
  if( nation != *options().nation ) return BuffersUpdated{};
  return redraw_buffers_for_tile_where_needed( tile,
                                               buffers_updated );
}

void RenderingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  this->Base::modify_entire_map( mutator );
}

void RenderingMapUpdater::redraw_landscape_buffer() {
  // No changing map size mid game.
  CHECK( ss_.terrain.world_size_tiles() ==
         landscape_tracking_.tile_bounds.size() );
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );
  render_landscape_buffer( renderer_, ss_, viz, terrain_options,
                           landscape_tracking_.tile_bounds );
  // Reset this since we just redrew the map.
  landscape_tracking_.tiles_redrawn = 0;
}

void RenderingMapUpdater::redraw_obfuscation_buffer() {
  // No changing map size mid game.
  CHECK( ss_.terrain.world_size_tiles() ==
         obfuscation_tracking_.tile_bounds.size() );
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );
  render_obfuscation_buffer( renderer_, viz, terrain_options,
                             obfuscation_tracking_.tile_bounds );
  // Reset this since we just redrew the map.
  obfuscation_tracking_.tiles_redrawn = 0;
}

void RenderingMapUpdater::redraw() {
  this->Base::redraw();
  redraw_landscape_buffer();
  redraw_obfuscation_buffer();
}

/****************************************************************
** TrappingMapUpdater
*****************************************************************/
BuffersUpdated TrappingMapUpdater::modify_map_square(
    Coord, base::function_ref<void( MapSquare& )> ) {
  SHOULD_NOT_BE_HERE;
}

BuffersUpdated TrappingMapUpdater::make_square_visible(
    Coord, e_nation ) {
  SHOULD_NOT_BE_HERE;
}

BuffersUpdated TrappingMapUpdater::make_square_fogged(
    Coord, e_nation ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::redraw() { SHOULD_NOT_BE_HERE; }

} // namespace rn
