/****************************************************************
**render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#include "render.hpp"

// Revolution Now
#include "errors.hpp"
#include "globals.hpp"
#include "logging.hpp"
#include "movement.hpp"
#include "ownership.hpp"
#include "sdl-util.hpp"
#include "viewport.hpp"
#include "world.hpp"

// SDL
#include "SDL.h"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {

// This is conceptually a stack in how elements are added and re-
// moved, but we need to be able to iterate through all elements
// for rendering, hence vector.
vector<RenderFunc> g_render_stack;

} // namespace

/****************************************************************
** Rendering Stack
*****************************************************************/
void render_frame() {
  ::SDL_SetRenderTarget( g_renderer, nullptr );

  for( auto const& renderer : g_render_stack ) {
    ::SDL_SetRenderTarget( g_renderer, nullptr );
    renderer();
  }

  render_panel();
  ::SDL_RenderPresent( g_renderer );
}

RenderStacker::RenderStacker( RenderFunc const& func ) {
  g_render_stack.push_back( func );
}

RenderStacker::~RenderStacker() {
  // Check for empty stack but don't throw because this is a de-
  // structor.
  if( g_render_stack.empty() ) {
    logger->critical(
        "Attempting to pop from empty render stack!" );
    return;
  }
  g_render_stack.pop_back();
}

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
// Expects the rendering target to already be set.
void render_landscape( Coord world_square, Coord pixel_coord ) {
  auto   s    = square_at( world_square );
  g_tile tile = s.land ? g_tile::land : g_tile::water;
  render_sprite( tile, pixel_coord, 0, 0 );
}

// Expects the rendering target to already be set.
void render_unit( UnitId id, Coord pixel_coord ) {
  auto const& unit = unit_from_id( id );
  render_sprite( unit.desc().tile, pixel_coord, 0, 0 );
}

/****************************************************************
** Viewport Rendering: these render to the viewport texture.
*****************************************************************/
void ViewportRenderOptions::assert_invariants() const {
  // If we're blinking a unit, then...
  if( unit_to_blink ) {
    auto id = *unit_to_blink;
    // make sure that we're not also skipping that unit.
    CHECK( !units_to_skip.contains( id ) );
    auto maybe_coord = coords_for_unit_safe( id );
    // make sure that the unit is on the map.
    CHECK( maybe_coord );
    // make sure the unit's square is not being skipped.
    CHECK( !squares_with_no_units.contains( *maybe_coord ) );
  }
}

void ViewportRenderOptions::reset() {
  *this = ViewportRenderOptions{};
  // why not...
  assert_invariants();
}

void render_world_viewport(
    ViewportRenderOptions const& options ) {
  set_render_target( g_texture_viewport );

  options.assert_invariants();

  auto covered = viewport().covered_tiles();

  Opt<Coord> blink_coords;
  if( options.unit_to_blink.has_value() )
    blink_coords = coords_for_unit( *options.unit_to_blink );

  for( auto coord : covered ) {
    Coord pixel_coord =
        Coord{} + ( coord - covered.upper_left() );
    pixel_coord *= g_tile_scale;

    // First the land.
    render_landscape( coord, pixel_coord );

    bool no_units =
        options.squares_with_no_units.contains( coord );
    bool is_blink_square = ( coord == blink_coords );

    // Next the units.

    // If this square has been requested to be blank OR if there
    // is a blinking unit on it then skip rendering units on it.
    if( !no_units && !is_blink_square ) {
      // Render all units on this square as usual.
      for( auto id : units_from_coord( coord ) )
        if( !options.units_to_skip.contains( id ) )
          render_unit( id, pixel_coord );
    }

    // Now do a blinking unit, if any.
    if( is_blink_square ) {
      using namespace std::chrono;
      using namespace std::literals::chrono_literals;
      auto time        = system_clock::now().time_since_epoch();
      auto one_second  = 1000ms;
      auto half_second = 500ms;
      if( time % one_second > half_second )
        render_unit( *options.unit_to_blink, pixel_coord );
    }
  }
}

void render_mv_unit( UnitId mv_id, Coord const& target,
                     double percent ) {
  set_render_target( g_texture_viewport );

  Coord coords = coords_for_unit( mv_id );
  Delta delta  = target - coords;
  CHECK( -1 <= delta.w && delta.w <= 1 );
  CHECK( -1 <= delta.h && delta.h <= 1 );
  delta *= g_tile_scale;
  Delta pixel_delta{W( int( delta.w._ * percent ) ),
                    H( int( delta.h._ * percent ) )};

  auto  covered = viewport().covered_tiles();
  Coord pixel_coord =
      Coord{} + ( coords - covered.upper_left() );
  pixel_coord *= g_tile_scale;
  pixel_coord += pixel_delta;
  auto const& unit = unit_from_id( mv_id );
  render_sprite( unit.desc().tile, pixel_coord, 0, 0 );
}

void render_copy_viewport_texture() {
  copy_texture( g_texture_viewport, nullopt,
                viewport().get_render_src_rect(),
                viewport().get_render_dest_rect() );
}

/****************************************************************
** Miscellaneous Rendering
*****************************************************************/
void render_panel() {
  constexpr int panel_width{6};
  auto          bottom_bar = 0_y + screen_height_tiles() - 1;
  auto left_side = 0_x + screen_width_tiles() - panel_width;
  // bottom edge
  for( X i( 0 ); i - 0_x < screen_width_tiles() - panel_width;
       ++i )
    render_sprite_grid( g_tile::panel_edge_left, bottom_bar, i,
                        1, 0 );
  // left edge
  for( Y i( 0 ); i - 0_y < screen_height_tiles() - 1; ++i )
    render_sprite_grid( g_tile::panel_edge_left, i, left_side, 0,
                        0 );
  // bottom left corner of main panel
  render_sprite_grid( g_tile::panel, bottom_bar, left_side, 0,
                      0 );

  for( Y i( 0 ); i - 0_y < screen_height_tiles(); ++i )
    for( X j( left_side + 1 ); j - 0_x < screen_width_tiles();
         ++j )
      render_sprite_grid( g_tile::panel, i, j, 0, 0 );
}

RenderFunc render_fade_to_dark( chrono::milliseconds wait,
                                chrono::milliseconds fade,
                                uint8_t target_alpha ) {
  auto now        = chrono::system_clock::now();
  auto start_time = now + wait;
  auto end_time   = start_time + fade;
  CHECK( now < start_time && start_time < end_time );
  return [start_time, end_time, target_alpha] {
    using namespace chrono;
    auto now = system_clock::now();
    if( now < start_time ) return;
    uint8_t alpha = target_alpha;
    if( now < end_time ) {
      milliseconds delta =
          duration_cast<milliseconds>( end_time - now );
      milliseconds total =
          duration_cast<milliseconds>( end_time - start_time );
      double ratio = double( delta.count() ) / total.count();
      CHECK( ratio >= 0.0 && ratio <= 1.0 );
      alpha =
          uint8_t( double( target_alpha ) * ( 1.0 - ratio ) );
    }
    // TODO: this may not require changing alpha to work.
    render_fill_rect( nullopt, Color( 0, 0, 0, alpha ),
                      screen_logical_rect() );
  };
}

} // namespace rn
