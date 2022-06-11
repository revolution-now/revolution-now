/****************************************************************
**app-ctrl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Handles the top-level game state machines.
*
*****************************************************************/
#include "app-ctrl.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "console.hpp"
#include "main-menu.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Top-Level Application Flow.
*****************************************************************/
wait<> revolution_now() {
  Planes&      planes    = Planes::global();
  PlaneGroup&  top_group = planes.top_group();
  ConsolePlane console_plane;
  OmniPlane    omni_plane;
  top_group.push( console_plane );
  top_group.push( omni_plane );
  co_await run_main_menu( planes );
}

} // namespace rn
