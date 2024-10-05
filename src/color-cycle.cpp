/****************************************************************
**color-cycle.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-04.
*
* Description: Game-specific color-cycling logic.
*
*****************************************************************/
#include "color-cycle.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"

// render
#include "render/irenderer.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
wait<> cycle_map_colors_thread( rr::IRenderer& renderer,
                                IGui&          gui,
                                bool const&    enabled ) {
  auto const cycle_span = renderer.get_color_cycle_span();
  CHECK_GT( cycle_span, 0 );
  SCOPE_EXIT { renderer.set_color_cycle_stage( 0 ); };
  int stage = 0;

  while( true ) {
    // The "stage > 0" clause is so that we do a smooth transi-
    // tion from enabled to disabled. This is because when we are
    // disabled we must have stage=0 for things to look good, and
    // we don't want to just jump there.
    bool const on = enabled || stage > 0;
    renderer.set_color_cycle_stage( on ? stage++ : 0 );
    stage %= cycle_span;
    co_await gui.wait_for( 600ms );
  }
}

} // namespace rn
