/****************************************************************
**open-gl-perf-test.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-22.
*
* Description: OpenGL performance testing.
*
*****************************************************************/
#include "open-gl-perf-test.hpp"

// Revolution Now
#include "error.hpp"
#include "input.hpp"
#include "render.hpp"
#include "renderer.hpp"
#include "terrain.hpp"
#include "text.hpp"

// gl
#include "gl/init.hpp"

using namespace ::std;
using namespace ::gfx;

namespace rn {
namespace {

#define OUTSIDE( x__, y__, w__, h__ )         \
  painter.draw_empty_rect(                    \
      rect{ .origin = { .x = x__, .y = y__ }, \
            .size   = { .w = w__, .h = h__ } }, \
      rr::Painter::e_border_mode::outside, pixel::red() );

#define INSIDE( x__, y__, w__, h__ )          \
  painter.draw_empty_rect(                    \
      rect{ .origin = { .x = x__, .y = y__ }, \
            .size   = { .w = w__, .h = h__ } }, \
      rr::Painter::e_border_mode::inside, pixel::white() );

void paint_things( rr::Renderer& renderer ) {
  rr::Painter painter = renderer.painter();

  renderer.clear_screen(
      pixel{ .r = 50, .g = 75, .b = 75, .a = 255 } );

  painter.draw_point(
      point{ .x = 50, .y = 50 },
      pixel{ .r = 255, .g = 0, .b = 0, .a = 255 } );

  OUTSIDE( 100, 100, 0, 0 );
  OUTSIDE( 100, 120, 1, 0 );
  OUTSIDE( 100, 140, 0, 1 );
  OUTSIDE( 100, 160, 1, 1 );
  OUTSIDE( 100, 180, 2, 2 );
  OUTSIDE( 100, 200, 3, 3 );
  OUTSIDE( 100, 220, 4, 4 );
  OUTSIDE( 100, 240, 5, 5 );
  OUTSIDE( 200, 100, 50, 50 );

  INSIDE( 100, 100, 0, 0 );
  INSIDE( 100, 120, 1, 0 );
  INSIDE( 100, 140, 0, 1 );
  INSIDE( 100, 160, 1, 1 );
  INSIDE( 100, 180, 2, 2 );
  INSIDE( 100, 200, 3, 3 );
  INSIDE( 100, 220, 4, 4 );
  INSIDE( 100, 240, 5, 5 );
  INSIDE( 200, 100, 50, 50 );

  painter.draw_solid_rect(
      rect{ .origin = { .x = 300, .y = 100 },
            .size   = { .w = 50, .h = 50 } },
      pixel{ .r = 128, .g = 64, .b = 0, .a = 255 } );

  painter
      .with_mods(
          { .depixelate =
                rr::DepixelateInfo{ .stage               = .5,
                                    .target_pixel_offset = {} },
            .alpha = .5 } )
      .draw_solid_rect(
          rect{ .origin = { .x = 325, .y = 125 },
                .size   = { .w = 50, .h = 50 } },
          pixel{ .r = 0, .g = 0, .b = 0, .a = 255 } );

  render_terrain_square( painter, Coord{ 300_x, 200_y },
                         Coord{ 0_x, 0_y } );
  render_terrain_square( painter, Coord{ 364_x, 200_y },
                         Coord{ 1_x, 1_y } );

  render_terrain_square( painter,
                         Rect{ 450_x, 200_y, 128_w, 64_h },
                         Coord{ 0_x, 0_y } );

  {
    auto popper =
        renderer.push_mods( []( rr::RendererMods& mods ) {
          mods.painter_mods.depixelate = rr::DepixelateInfo{
              .stage = .5, .target_pixel_offset = {} };
        } );
    rr::Painter painter_depixel = renderer.painter();
    render_terrain_square( painter_depixel,
                           Coord{ 300_x, 250_y },
                           Coord{ 0_x, 0_y } );
    render_terrain_square( painter_depixel,
                           Coord{ 364_x, 250_y },
                           Coord{ 1_x, 1_y } );
  }

  render_unit( renderer, Coord{ 250_x, 250_y }, 6_id,
               /*with_icon=*/true );

  pixel text_color = { .r = 0, .g = 0, .b = 48, .a = 255 };
  render_text(
      renderer, { .x = 300, .y = 300 }, e_font::_6x6, text_color,
      fmt::format(
          "Color of this text is {}.\nThe End.\n\n-David",
          text_color ) );
}

} // namespace

void open_gl_perf_test() {
  rr::Renderer& renderer =
      global_renderer_use_only_when_needed();
  while( !input::is_q_down() )
    renderer.render_pass( paint_things );
}

} // namespace rn
