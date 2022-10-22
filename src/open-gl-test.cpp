/****************************************************************
**open-gl-test.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-10-22.
*
* Description: OpenGL testing.
*
*****************************************************************/
#include "open-gl-test.hpp"

// Revolution Now
#include "input.hpp"
#include "renderer.hpp"

using namespace ::std;
using namespace ::gfx;

namespace rn {
namespace {

void paint_things( rr::Renderer& renderer ) {
  renderer.clear_screen(
      pixel{ .r = 50, .g = 75, .b = 75, .a = 255 } );
  gfx::rect r;

  float grad_mag = 1.0 / 100.0;

  pixel const kColor = { .r = 255, .g = 0, .b = 0, .a = 255 };

  int x = 50;

  vector<pair<dsize, double>> directions{
      { dsize{ .h = -grad_mag }, 1.0 },
      { dsize{ .w = grad_mag }, 0.0 },
      { dsize{ .h = grad_mag }, 0.0 },
      { dsize{ .w = -grad_mag }, 1.0 },
      { dsize{ .w = -grad_mag, .h = -grad_mag }, 1.0 },
  };

  for( auto [d, base_stage] : directions ) {
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             base_stage );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient, d );
    r = rect{ .origin = { .x = x, .y = 50 },
              .size   = { .w = 100, .h = 100 } };
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        r.origin.to_double() );
    renderer.painter().draw_solid_rect( r, kColor );
    x += 110;
  }
}

} // namespace

void open_gl_test() {
  rr::Renderer& renderer =
      global_renderer_use_only_when_needed();
  while( !input::is_q_down() )
    renderer.render_pass( paint_things );
}

} // namespace rn
