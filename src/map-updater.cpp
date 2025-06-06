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
#include "depletion.hpp"
#include "fog-conv.hpp"
#include "render-terrain.hpp"
#include "tiles.hpp"
#include "visibility.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// render
#include "render/renderer.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/logger.hpp"

// C++ standard library
#include <unordered_set>

using namespace std;

namespace rn {

namespace {

using clear      = FogStatus::clear;
using explored   = PlayerSquare::explored;
using fogged     = FogStatus::fogged;
using unexplored = PlayerSquare::unexplored;

} // namespace

/****************************************************************
** NonRenderingMapUpdater
*****************************************************************/
NonRenderingMapUpdater::NonRenderingMapUpdater(
    SS& ss, MapUpdaterOptions const& initial_options )
  : IMapUpdater( initial_options ), ss_( ss ) {}

NonRenderingMapUpdater::NonRenderingMapUpdater( SS& ss )
  : NonRenderingMapUpdater( ss, MapUpdaterOptions{} ) {}

BuffersUpdated NonRenderingMapUpdater::modify_map_square(
    Coord tile, SquareUpdateFunc mutator ) {
  MapSquare const old_square = ss_.terrain.square_at( tile );
  mutator( ss_.mutable_terrain_use_with_care.mutable_square_at(
      tile ) );
  MapSquare const& new_square = ss_.terrain.square_at( tile );
  remove_depletion_counter_if_needed( ss_, tile );
  // Check if the rendered map needs its buffer updated.
  return BuffersUpdated{
    .tile      = tile,
    .landscape = ( new_square != old_square ) &&
                 ( !options().player.has_value() ||
                   does_player_have_fog_removed_on_square(
                       ss_, *options().player, tile ) ) };
}

vector<BuffersUpdated>
NonRenderingMapUpdater::make_squares_visible(
    e_player player, vector<Coord> const& tiles ) {
  auto& map = ss_.mutable_terrain_use_with_care
                  .mutable_player_terrain( player )
                  .map;

  unordered_set<Coord> hit;

  // First make the specified tiles clear.
  vector<BuffersUpdated> res;
  for( Coord const tile : tiles ) {
    // The Visibility::visible function also works with off-map
    // ("proto") tiles, but we don't anticipate having those
    // here, so just check that for sanity.
    CHECK( tile.is_inside( map.rect() ) );
    if( hit.contains( tile ) ) continue;
    hit.insert( tile );
    BuffersUpdated& buffers_updated = res.emplace_back();
    buffers_updated.tile            = tile;

    SWITCH( map[tile] ) {
      CASE( unexplored ) {
        buffers_updated.landscape   = true;
        buffers_updated.obfuscation = true;
        map[tile]
            .emplace<explored>()
            .fog_status.emplace<clear>();
        // !! Current alternative invalidated here.
        break;
      }
      CASE( explored ) {
        SWITCH( explored.fog_status ) {
          CASE( fogged ) {
            auto& frozen_square = fogged.contents;
            MapSquare const& real_square =
                ss_.terrain.square_at( tile );
            if( frozen_square.square != real_square )
              buffers_updated.landscape = true;
            map[tile]
                .emplace<PlayerSquare::explored>()
                .fog_status.emplace<clear>();
            // !! Current alternative invalidated here.
            if( options().render_fog_of_war )
              buffers_updated.obfuscation = true;
            break;
          }
          CASE( clear ) { break; }
        }
        break;
      }
    }
  }

  CHECK( res.size() <= tiles.size() );
  return res;
}

// Implement IMapUpdater.
vector<BuffersUpdated>
NonRenderingMapUpdater::make_squares_fogged(
    e_player player, vector<Coord> const& tiles ) {
  auto& map = ss_.mutable_terrain_use_with_care
                  .mutable_player_terrain( player )
                  .map;

  vector<BuffersUpdated> res;
  for( Coord const tile : tiles ) {
    BuffersUpdated& buffers_updated = res.emplace_back();
    buffers_updated.tile            = tile;
    SWITCH( map[tile] ) {
      CASE( unexplored ) { break; }
      CASE( explored ) {
        SWITCH( explored.fog_status ) {
          CASE( fogged ) { break; }
          CASE( clear ) {
            FrozenSquare& frozen_square =
                map[tile]
                    .emplace<PlayerSquare::explored>()
                    .fog_status.emplace<fogged>()
                    .contents;
            // !! Current alternative invalidated here.
            copy_real_square_to_frozen_square( ss_, tile,
                                               frozen_square );
            if( options().render_fog_of_war )
              buffers_updated.obfuscation = true;
            break;
          }
        }
        break;
      }
    }
  }

  CHECK( res.size() == tiles.size() );
  return res;
}

vector<BuffersUpdated>
NonRenderingMapUpdater::force_redraw_tiles(
    vector<Coord> const& ) {
  return {};
}

void NonRenderingMapUpdater::modify_entire_map_no_redraw(
    MapUpdateFunc mutator ) {
  ss_.mutable_terrain_use_with_care.modify_entire_map( mutator );
  // Go over the entire map and remove any depletion counter that
  // is set for a tile for which it is no longer relevant.
  remove_depletion_counters_where_needed( ss_ );
}

void NonRenderingMapUpdater::redraw() {}

void NonRenderingMapUpdater::unrender() {}

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
    rr::e_render_buffer annex_buffer,
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
  //
  // TODO: this seems to work fine for standard map sizes, but
  // may need to be scaled up for large map sizes. This was
  // prompted by the observation that on an 800x800 map, when de-
  // stroying all of the native tribes with full map visibility,
  // the rerendering done under the villages as a result caused
  // the entire map to be redraw ~20 times. Increasing this
  // number to 100,000 decreased that to two.
  static int constexpr kRedrawThreshold = 10000;
  // The > is defensive.
  if( buffer_tracking.tiles_redrawn >= kRedrawThreshold )
    redraw_buffer();
  lg.trace( "{} tiles redrawn: {}",
            refl::enum_value_name( annex_buffer ),
            buffer_tracking.tiles_redrawn );
}

void RenderingMapUpdater::redraw_buffers_for_tiles_where_needed(
    vector<BuffersUpdated> const& buffers_updated ) {
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  unique_ptr<IVisibility const> const viz =
      create_visibility_for( ss_, options().player );

  // Note: in the below, we need to draw the surrounding squares
  // because a visibility change in one square can reveal part of
  // the adjacent tiles even if they are not visible. In some
  // edge cases with map rendering we need to go two squares away
  // to properly rerender the map in response to a change in one
  // tile. However, those currently only happen when changing the
  // ground type of a tile with the map editor during the game,
  // and so it doesn't seem worth it to take the performance hit
  // of re-rendering an additional 16 tiles just to support that
  // case, which is not part of a normal game anyway.
  //
  // In any case, in practice when we are rerendering multiple
  // tiles in a single region, the addition of these surrounding
  // tiles will cause many tiles to be redrawn redundantly, and
  // so we first find a unique set, then render each tile ones.
  unordered_set<Coord> landscape_updates;
  unordered_set<Coord> obfuscation_updates;

  for( int i = 0; i < int( buffers_updated.size() ); ++i ) {
    Coord const tile = buffers_updated[i].tile;
    if( buffers_updated[i].landscape ) {
      for( e_cdirection d : refl::enum_values<e_cdirection> ) {
        Coord const moved = tile.moved( d );
        if( !ss_.terrain.square_exists( moved ) ) continue;
        landscape_updates.insert( moved );
      }
    }
    if( buffers_updated[i].obfuscation ) {
      for( e_cdirection d : refl::enum_values<e_cdirection> ) {
        Coord const moved = tile.moved( d );
        if( !ss_.terrain.square_exists( moved ) ) continue;
        obfuscation_updates.insert( moved );
      }
    }
  }

  for( Coord const tile : landscape_updates )
    redraw_square_single_buffer(
        tile, landscape_tracking_,
        rr::e_render_buffer::landscape_annex,
        [&] {
          render_landscape_square_if_not_fully_hidden(
              renderer_, tile * g_tile_delta, tile, *viz,
              terrain_options );
        },
        [&] { redraw_landscape_buffer(); } );

  for( Coord const tile : obfuscation_updates )
    redraw_square_single_buffer(
        tile, obfuscation_tracking_,
        rr::e_render_buffer::obfuscation_annex,
        [&] {
          render_obfuscation_overlay( renderer_,
                                      tile * g_tile_delta, tile,
                                      *viz, terrain_options );
        },
        [&] { redraw_obfuscation_buffer(); } );
}

BuffersUpdated RenderingMapUpdater::modify_map_square(
    Coord tile, SquareUpdateFunc mutator ) {
  BuffersUpdated const buffers_updated =
      this->Base::modify_map_square( tile, mutator );
  redraw_buffers_for_tiles_where_needed( { buffers_updated } );
  return buffers_updated;
}

vector<BuffersUpdated> RenderingMapUpdater::make_squares_visible(
    e_player player, vector<Coord> const& tiles ) {
  vector<BuffersUpdated> buffers_updated =
      this->Base::make_squares_visible( player, tiles );
  // If entire map is visible then there is no need to render.
  if( !options().player.has_value() ) return buffers_updated;
  // If it's another player then not relevant for rendering.
  if( player != *options().player ) return buffers_updated;
  redraw_buffers_for_tiles_where_needed( buffers_updated );
  return buffers_updated;
}

vector<BuffersUpdated> RenderingMapUpdater::make_squares_fogged(
    e_player player, vector<Coord> const& tiles ) {
  vector<BuffersUpdated> buffers_updated =
      this->Base::make_squares_fogged( player, tiles );
  vector<BuffersUpdated> empty( tiles.size() );
  // If the entire map is visible then there is also no fog ren-
  // dered anywhere, so no need to re-render.
  if( !options().player.has_value() ) return buffers_updated;
  // If it's another player then not relevant for rendering.
  if( player != *options().player ) return buffers_updated;
  redraw_buffers_for_tiles_where_needed( buffers_updated );
  return buffers_updated;
}

vector<BuffersUpdated> RenderingMapUpdater::force_redraw_tiles(
    vector<Coord> const& tiles ) {
  vector<BuffersUpdated> buffers_updated;
  buffers_updated.reserve( tiles.size() );
  for( Coord const tile : tiles )
    buffers_updated.push_back(
        BuffersUpdated{ .tile = tile, .landscape = true } );
  redraw_buffers_for_tiles_where_needed( buffers_updated );
  return buffers_updated;
}

void RenderingMapUpdater::modify_entire_map_no_redraw(
    MapUpdateFunc mutator ) {
  this->Base::modify_entire_map_no_redraw( mutator );
}

void RenderingMapUpdater::redraw_landscape_buffer() {
  // No changing map size mid game.
  CHECK( ss_.terrain.world_size_tiles() ==
         landscape_tracking_.tile_bounds.size() );
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  unique_ptr<IVisibility const> const viz =
      create_visibility_for( ss_, options().player );
  render_landscape_buffer( renderer_, *viz, terrain_options,
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
  unique_ptr<IVisibility const> const viz =
      create_visibility_for( ss_, options().player );
  render_obfuscation_buffer( renderer_, *viz, terrain_options,
                             obfuscation_tracking_.tile_bounds );
  // Reset this since we just redrew the map.
  obfuscation_tracking_.tiles_redrawn = 0;
}

void RenderingMapUpdater::redraw() {
  this->Base::redraw();
  redraw_landscape_buffer();
  redraw_obfuscation_buffer();
}

void RenderingMapUpdater::unrender() {
  this->Base::unrender();
  // This won't cause the data to be removed from the GPU, but
  // the effect will be the same, because when we run the shader
  // program on a buffer we specify the number of vertices to run
  // it on, which will be taken from the size of the CPU buffer,
  // which will be zero, so will have the same effect.

  // landscape buffer.
  renderer_.clear_buffer( rr::e_render_buffer::landscape );
  renderer_.clear_buffer( rr::e_render_buffer::landscape_annex );
  landscape_tracking_ =
      BufferTracking( ss_.terrain.world_size_tiles() );

  // obfuscation buffer.
  renderer_.clear_buffer( rr::e_render_buffer::obfuscation );
  renderer_.clear_buffer(
      rr::e_render_buffer::obfuscation_annex );
  obfuscation_tracking_ =
      BufferTracking( ss_.terrain.world_size_tiles() );
}

} // namespace rn
