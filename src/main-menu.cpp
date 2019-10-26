/****************************************************************
**main-menu.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#include "main-menu.hpp"

// Revolution Now
#include "plane.hpp"

using namespace std;

namespace rn {

namespace {

struct MainMenuPlane : public Plane {
  MainMenuPlane() = default;
  bool enabled() const override { return true; }
  bool covers_screen() const override { return true; }
  void draw( Texture& tx ) const override { (void)tx; }
  bool input( input::event_t const& event ) override {
    bool handled = false;
    switch_( event ) {
      case_( input::unknown_event_t ) {}
      case_( input::quit_event_t ) {}
      case_( input::key_event_t ) {}
      case_( input::mouse_wheel_event_t ) {}
      case_( input::mouse_button_event_t ) {}
      switch_non_exhaustive;
    }
    return handled;
  }
};

MainMenuPlane g_main_menu_plane;

} // namespace

Plane* main_menu_plane() { return &g_main_menu_plane; }

/****************************************************************
** Public API
*****************************************************************/
// ...

/****************************************************************
** Testing
*****************************************************************/
void test_main_menu() {
  //
}

} // namespace rn
