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
#include "render-terrain.hpp"
#include "tiles.hpp"
#include "visibility.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/terrain.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/iter.hpp"

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
bool NonRenderingMapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) {
  MapSquare old_square = ss_.terrain.square_at( tile );
  mutator( ss_.mutable_terrain_use_with_care.mutable_square_at(
      tile ) );
  MapSquare new_square = ss_.terrain.square_at( tile );
  if( new_square == old_square ) return false;

  // Update player maps.
  refl::enum_map<e_nation, bool> const visible_to_nations =
      nations_with_visibility_of_square( ss_, tile );
  for( auto [nation, visible] : visible_to_nations )
    if( visible ) make_square_visible( tile, nation );
  return true;
}

bool NonRenderingMapUpdater::make_square_visible(
    Coord tile, e_nation nation ) {
  PlayerTerrain& player_terrain =
      ss_.mutable_terrain_use_with_care.mutable_player_terrain(
          nation );
  Matrix<maybe<FogSquare>>& map = player_terrain.map;

  bool changed = false;
  if( !map[tile].has_value() ) {
    // We need this because the default-constructed map square
    // we're about to make is identical to an ocean square, so we
    // won't know that we weren't visible before.
    changed = true;
    // Note this initializes the tile in such a way that there
    // will be fog on the tile by default.
    map[tile].emplace();
  }
  FogSquare& fog_square = *map[tile];

  bool const fog_removed_before = fog_square.fog_of_war_removed;
  int const  fog_removed_after  = true;
  fog_square.fog_of_war_removed = fog_removed_after;
  if( fog_removed_before != fog_removed_after ) changed = true;

  // TODO: check and update other members of FogSquare here.

  MapSquare&       player_square = fog_square.square;
  MapSquare const& real_square   = ss_.terrain.square_at( tile );

  if( player_square != real_square ) changed = true;
  player_square = real_square;
  return changed;
}

// Implement IMapUpdater.
bool NonRenderingMapUpdater::make_square_fogged(
    Coord tile, e_nation nation ) {
  PlayerTerrain& player_terrain =
      ss_.mutable_terrain_use_with_care.mutable_player_terrain(
          nation );
  Matrix<maybe<FogSquare>>& map = player_terrain.map;

  if( !map[tile].has_value() ) return false;
  FogSquare& fog_square = *map[tile];
  if( !fog_square.fog_of_war_removed ) return false;

  fog_square.fog_of_war_removed = false;
  return true;
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
    SS& ss, rr::Renderer& renderer )
  : NonRenderingMapUpdater( ss ),
    renderer_( renderer ),
    tiles_redrawn_( 0 ),
    tile_bounds_( ss.terrain.world_size_tiles() ) {
  // Something is probably wrong if this happens.
  CHECK_GT( tile_bounds_.size().area(), 0 );
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
void RenderingMapUpdater::redraw_square(
    Visibility const&           viz,
    TerrainRenderOptions const& terrain_options, Coord tile ) {
  auto& renderer = renderer_;
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );
  SCOPED_RENDERER_MOD_SET(
      buffer_mods.buffer,
      rr::e_render_target_buffer::landscape_annex );

  rr::VertexRange const old_bounds = tile_bounds_[tile];
  tile_bounds_[tile]               = renderer_.range_for( [&] {
    render_terrain_square( renderer_, tile * g_tile_delta, ss_,
                                         tile, viz, terrain_options );
  } );

  // Now we zero out the vertices from the old tile. If we don't
  // do this then, over time, as we overwite a tile many times,
  // the tile accumulates so many renderable vertices that frame
  // rate significantly drops when a large number of screen
  // pixels are occupied by such tiles.
  renderer_.zap( old_bounds );

  // If we've redrawn a number of tiles that are beyond the below
  // threshold then we will just redraw the entire thing. We do
  // this for two reasons. First, each time a tile gets redrawn
  // (and the landscape annex buffer gets appended to, the entire
  // annex buffer needs to get re-uploaded to the GPU, which
  // would then happen potentially on each move of a unit (where
  // new terrain is exposed). By redrawing the entire thing peri-
  // odically we clear the annex buffer and eliminate this la-
  // tency. This threshold is set somewhat arbitrarily, but it is
  // set to a value that is not so low as to cause a map redraw
  // too often (which will be slow on large maps) and not too
  // large as to let the annex buffer get too large (which would
  // increase latency of redrawing a tile by having to re-upload
  // a large annex buffer to the GPU).
  ++tiles_redrawn_;
  int const redraw_threshold = 20000;
  // The > is defensive.
  if( tiles_redrawn_ >= redraw_threshold ) redraw();
}

bool RenderingMapUpdater::modify_map_square(
    Coord                                  tile,
    base::function_ref<void( MapSquare& )> mutator ) {
  bool const changed =
      this->Base::modify_map_square( tile, mutator );
  if( !changed )
    // Return before beginning the rendering, that way we a)
    // won't add more vertices to the landscape buffer and b) we
    // won't trigger the buffer to get re-uploaded to the GPU.
    // This really helps with efficiency in the map editor.
    return changed;

  // Re-render the square and all adjacent squares. Actually, we
  // need to do two levels of adjacency because some water tiles
  // can derive their ground terrain from their neighbors, and
  // those in turn can affect their neighbors. Though changes of
  // this kind only happen in the map editor.
  Rect const to_update =
      Rect::from( tile, Delta{ .w = 1, .h = 1 } )
          .with_border_added( 2 );
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );
  for( Rect moved : gfx::subrects( to_update ) )
    if( ss_.terrain.square_exists( moved.upper_left() ) )
      redraw_square( viz, terrain_options, moved.upper_left() );

  return changed;
}

bool RenderingMapUpdater::make_square_visible(
    Coord const tile, e_nation nation ) {
  bool const changed =
      this->Base::make_square_visible( tile, nation );
  if( !changed ) return changed;

  // We need to check if it has a value here because if it has no
  // value then that means the entire map is visible and we
  // should proceed to render.
  if( options().nation.has_value() &&
      options().nation != nation )
    return changed;

  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );
  redraw_square( viz, terrain_options, tile );
  // We need to draw the surrounding squares because a visibility
  // change in one square can reveal part of the adjacent files
  // even if they are not visible. In some edge cases with map
  // rendering we need to go two squares away to properly
  // rerender the map in response to a change in one tile. How-
  // ever, those currently only happen when changing the ground
  // type of a tile with the map editor during the game, and so
  // it doesn't seem worth it to take the performance hit of
  // re-rendering an additional 16 tiles just to support that
  // case, which is not part of a normal game anyway.
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const moved = tile.moved( d );
    if( !ss_.terrain.square_exists( moved ) ) continue;
    redraw_square( viz, terrain_options, moved );
  }

  return changed;
}

bool RenderingMapUpdater::make_square_fogged( Coord    tile,
                                              e_nation nation ) {
  bool const changed =
      this->Base::make_square_fogged( tile, nation );
  if( !changed ) return changed;

  // If the entire map is visible then there is also no fog ren-
  // dered anywhere, so no need to rerender.
  if( !options().nation.has_value() ) return changed;
  e_nation const curr_nation = *options().nation;

  // If it's another nation then not relevant for rendering.
  if( curr_nation != nation ) return changed;

  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );
  redraw_square( viz, terrain_options, tile );

  // !! NOTE: not redrawing surrounding squares here, unlike in
  // make_square_visible.  This may be changed in the future.
  for( e_direction d : refl::enum_values<e_direction> ) {
    Coord const moved = tile.moved( d );
    if( !ss_.terrain.square_exists( moved ) ) continue;
    redraw_square( viz, terrain_options, moved );
  }

  return changed;
}

void RenderingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  this->Base::modify_entire_map( mutator );
}

void RenderingMapUpdater::redraw() {
  this->Base::redraw();
  // No changing map size mid game.
  CHECK( ss_.terrain.world_size_tiles() == tile_bounds_.size() );
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  Visibility const viz =
      Visibility::create( ss_, options().nation );
  render_terrain( renderer_, ss_, viz, terrain_options,
                  tile_bounds_ );
  // Reset this since we just redrew the map.
  tiles_redrawn_ = 0;
}

/****************************************************************
** TrappingMapUpdater
*****************************************************************/
bool TrappingMapUpdater::modify_map_square(
    Coord, base::function_ref<void( MapSquare& )> ) {
  SHOULD_NOT_BE_HERE;
}

bool TrappingMapUpdater::make_square_visible( Coord, e_nation ) {
  SHOULD_NOT_BE_HERE;
}

bool TrappingMapUpdater::make_square_fogged( Coord, e_nation ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::redraw() { SHOULD_NOT_BE_HERE; }

} // namespace rn
