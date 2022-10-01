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
      .render_forests   = our_options.render_forests,
      .render_resources = our_options.render_resources,
      .render_lcrs      = our_options.render_lcrs,
      .grid             = our_options.grid };
}

}

/****************************************************************
** MapUpdaterOptions
*****************************************************************/
namespace detail {

MapUpdaterOptionsPopper::~MapUpdaterOptionsPopper() noexcept {
  CHECK( !map_updater_.options_.empty() );
  MapUpdaterOptions const popped = map_updater_.options_.top();
  map_updater_.options_.pop();
  CHECK( !map_updater_.options_.empty() );
  if( popped != map_updater_.options_.top() )
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

void IMapUpdater::mutate_options_and_redraw(
    OptionsUpdateFunc mutator ) {
  CHECK( !options_.empty() );
  MapUpdaterOptions const old_options = options();
  mutator( options_.top() );
  if( options_.top() != old_options ) redraw();
}

void to_str( IMapUpdater const&, string& out, base::ADL_t ) {
  out += "IMapUpdater";
}

/****************************************************************
** MapUpdater
*****************************************************************/
void MapUpdater::redraw_square(
    Visibility const&           viz,
    TerrainRenderOptions const& terrain_options, Coord tile ) {
  auto& renderer = renderer_;
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );
  SCOPED_RENDERER_MOD_SET(
      buffer_mods.buffer,
      rr::e_render_target_buffer::landscape );

  render_terrain_square( renderer_, tile * g_tile_delta, tile,
                         viz, terrain_options );

  ++tiles_redrawn_;
  // If we've redrawn 1000 tiles then probably a good time to
  // just redraw the entire thing. In fact we need to do this be-
  // cause otherwise rendering significantly slows down when we
  // have a large number of screen pixels covering tiles that
  // have been updated multiple times (presumably because the
  // fragment shaders for each layer are still being run even
  // though the under layers are not visible).
  if( tiles_redrawn_ == 1000 ) redraw();
}

Visibility MapUpdater::visibility() const {
  maybe<PlayerTerrain const&> player_terrain =
      options().nation.has_value()
          ? ss_.terrain.player_terrain( *options().nation )
          : base::nothing;
  return Visibility( ss_.terrain, player_terrain );
}

bool MapUpdater::modify_map_square(
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
  Visibility const viz = visibility();
  for( Coord moved : to_update )
    if( ss_.terrain.square_exists( moved ) )
      redraw_square( viz, terrain_options, moved );

  return changed;
}

bool MapUpdater::make_square_visible( Coord const tile,
                                      e_nation    nation ) {
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
  Visibility const viz = visibility();
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

void MapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  this->Base::modify_entire_map( mutator );
}

void MapUpdater::redraw() {
  this->Base::redraw();
  TerrainRenderOptions const terrain_options =
      make_terrain_options( options() );
  render_terrain( renderer_, visibility(), terrain_options );
  // Reset this since we just redrew the map.
  tiles_redrawn_ = 0;
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
    if( visible ) //
      make_square_visible( tile, nation );
  return true;
}

bool NonRenderingMapUpdater::make_square_visible(
    Coord tile, e_nation nation ) {
  PlayerTerrain& player_terrain =
      ss_.mutable_terrain_use_with_care.mutable_player_terrain(
          nation );
  Matrix<maybe<FogSquare>>& map     = player_terrain.map;
  bool                      changed = false;
  if( !map[tile].has_value() ) {
    // We need this because the default-constructed map square
    // we're about to make is identical to an ocean square, so we
    // won't know that we weren't visible before.
    changed = true;
    map[tile].emplace();
  }
  FogSquare&       fog_square    = *map[tile];
  MapSquare&       player_square = fog_square.square;
  MapSquare const& real_square   = ss_.terrain.square_at( tile );

  // TODO: check and update other members of FogSquare here.

  changed |= ( player_square != real_square );
  player_square = real_square;
  return changed;
}

void NonRenderingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> mutator ) {
  mutator(
      ss_.mutable_terrain_use_with_care.mutable_world_map() );
}

void NonRenderingMapUpdater::redraw() {}

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

void TrappingMapUpdater::modify_entire_map(
    base::function_ref<void( Matrix<MapSquare>& )> ) {
  SHOULD_NOT_BE_HERE;
}

void TrappingMapUpdater::redraw() { SHOULD_NOT_BE_HERE; }

} // namespace rn
