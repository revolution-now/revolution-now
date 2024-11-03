/****************************************************************
**difficulty-screen-2.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-03.
*
* Description: Screen where player chooses difficulty level.
*
*****************************************************************/
#include "difficulty-screen-2.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "input.hpp"
#include "interrupts.hpp"
#include "logger.hpp"
#include "plane-stack.hpp"

// ss
#include "ss/difficulty.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** DfficultyScreen
*****************************************************************/
struct DifficultyScreen : public IPlane {
  // State
  wait_promise<maybe<e_difficulty>> result_ = {};

 public:
  DifficultyScreen() {
    // TODO
  }

  void on_logical_resolution_changed( e_resolution ) override {
    // TODO
  }

  void draw( rr::Renderer& renderer ) const override {
    // TODO
    (void)renderer;
  }

  e_input_handled on_key(
      input::key_event_t const& event ) override {
    if( event.change != input::e_key_change::down )
      return e_input_handled::no;
    if( input::has_mod_key( event ) ) return e_input_handled::no;
    auto handled = e_input_handled::no;
    switch( event.keycode ) {
      case ::SDLK_SPACE:
      case ::SDLK_RETURN:
      case ::SDLK_KP_ENTER:
      case ::SDLK_KP_5:
        // TODO
        break;
      case ::SDLK_ESCAPE:
        result_.set_value( nothing );
        break;
      case ::SDLK_LEFT:
      case ::SDLK_KP_4:
        // TODO
        break;
      case ::SDLK_RIGHT:
      case ::SDLK_KP_6:
        // TODO
        break;
      case ::SDLK_UP:
      case ::SDLK_KP_8:
        // TODO
        break;
      case ::SDLK_DOWN:
      case ::SDLK_KP_2:
        // TODO
        break;
    }
    return handled;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    if( event.buttons != input::e_mouse_button_event::left_up )
      return e_input_handled::no;
    auto handled = e_input_handled::no;
    // TODO
    return handled;
  }

  wait<e_difficulty> run() {
    auto const difficulty = co_await result_.wait();
    if( !difficulty.has_value() ) throw main_menu_interrupt{};
    lg.info( "selected difficulty level: {}", *difficulty );
    co_return *difficulty;
  }
};

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<e_difficulty> choose_difficulty_screen_2( Planes& planes ) {
  auto        owner = planes.push();
  PlaneGroup& group = owner.group;

  DifficultyScreen difficulty_screen;
  group.bottom = &difficulty_screen;

  co_return co_await difficulty_screen.run();
}

} // namespace rn
