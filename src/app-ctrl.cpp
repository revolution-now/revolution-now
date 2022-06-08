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
#include "window.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Top-Level Application Flow.
*****************************************************************/
wait<> revolution_now() {
  PlaneStack& plane_stack = PlaneStack::global();

  // Level 0 planes.
  MainMenuPlane main_menu_plane(
      plane_stack[e_plane_stack_level::bottom] );

  // Level 1 planes.
  MenuPlane menu_plane(
      plane_stack[e_plane_stack_level::middle] );
  WindowPlane window_plane(
      plane_stack[e_plane_stack_level::middle] );

  // Level 2 planes.
  ConsolePlane console_plane(
      plane_stack[e_plane_stack_level::top], menu_plane );
  OmniPlane omni_plane( plane_stack[e_plane_stack_level::top] );

  co_await main_menu_plane.run();
}

} // namespace rn
