/****************************************************************
**menu-plane.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: Implements the IMenuPlane menu server.
*
*****************************************************************/
#include "menu-plane.hpp"

// Revolution Now
#include "menu-coro.hpp"
#include "menu-render.hpp"
#include "plane.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Menu2Plane::Impl
*****************************************************************/
struct Menu2Plane::Impl : IPlane {
  void on_logical_resolution_changed(
      e_resolution const /*resolution*/ ) override {}

  void draw( rr::Renderer& renderer ) const override {
    menu_threads_.on_all_render_states(
        [&]( MenuRenderState const& state ) {
          MenuRenderer menu_renderer( state );
          menu_renderer.render_body( renderer );
        } );
  }

  e_input_handled on_mouse_move(
      input::mouse_move_event_t const& event ) override {
    auto const raw = MenuEventRaw::device{ .event = event };
    if( menu_threads_.route_raw_input_thread( raw ) )
      return e_input_handled::yes;
    return e_input_handled::no;
  }

  e_input_handled on_mouse_button(
      input::mouse_button_event_t const& event ) override {
    if( menu_threads_.open_count() == 0 )
      return e_input_handled::no;
    // There are some open menus, so from this point on we will
    // always return that we handled the input.

    auto const raw = MenuEventRaw::device{ .event = event };
    if( menu_threads_.route_raw_input_thread( raw ) )
      return e_input_handled::yes;

    if( event.buttons == input::e_mouse_button_event::left_up ) {
      menu_threads_.route_raw_input_thread(
          MenuEventRaw::close_all{} );
      return e_input_handled::yes;
    }

    return e_input_handled::yes;
  }

  MenuThreads menu_threads_;
};

/****************************************************************
** Menu2Plane
*****************************************************************/
IPlane& Menu2Plane::impl() { return *impl_; }

Menu2Plane::~Menu2Plane() = default;

Menu2Plane::Menu2Plane() : impl_( new Impl() ) {}

wait<maybe<e_menu_item>> Menu2Plane::open_menu(
    MenuLayout const& layout, MenuPosition const& position ) {
  co_return co_await impl_->menu_threads_.open_menu( layout,
                                                     position );
}

} // namespace rn
