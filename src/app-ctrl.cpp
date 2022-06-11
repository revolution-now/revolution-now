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
#include "gui.hpp"
#include "main-menu.hpp"
#include "menu.hpp"
#include "omni.hpp"
#include "plane-stack.hpp"
#include "window.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Top-Level Application Flow.
*****************************************************************/
wait<> revolution_now() {
  Planes& planes = Planes::global();

  WindowPlane   window_plane;
  RealGui       gui( window_plane );
  MainMenuPlane main_menu_plane( planes, window_plane, gui );
  ConsolePlane  console_plane( /*menu_plane=*/nothing );
  OmniPlane     omni_plane;

  auto        popper = planes.new_group();
  PlaneGroup& group  = planes.back();

  // The menu plane goes first because we want to hide it behind
  // the main menu screen.
  group.push( main_menu_plane );
  group.push( window_plane );
  group.push( console_plane );
  group.push( omni_plane );

  co_await main_menu_plane.run();
}

} // namespace rn
