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

  rr::Painter painter = renderer.painter();

  painter.draw_solid_rect(
      rect{ .origin = { .x = 50, .y = 50 },
            .size   = { .w = 100, .h = 100 } },
      pixel{ .r = 255, .g = 0, .b = 0, .a = 255 } );
}

} // namespace

void open_gl_test() {
  rr::Renderer& renderer =
      global_renderer_use_only_when_needed();
  while( !input::is_q_down() )
    renderer.render_pass( paint_things );
}

} // namespace rn
