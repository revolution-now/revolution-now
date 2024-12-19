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
    for( int const menu_id : menu_threads_.open_menu_ids() ) {
      auto const& anim_state =
          menu_threads_.anim_state( menu_id );
      auto const& render_layout =
          menu_threads_.render_layout( menu_id );
      render_menu_body( renderer, anim_state, render_layout );
    }
  }

  e_input_handled input( input::event_t const& event ) override {
    if( menu_threads_.open_count() == 0 )
      return e_input_handled::no;
    auto const raw = MenuEventRaw::device{ .event = event };
    menu_threads_.route_raw_input_thread( raw );
    return e_input_handled::yes;
  }

  e_accept_drag can_drag( input::e_mouse_button const /*button*/,
                          Coord const /*origin*/ ) override {
    if( menu_threads_.open_count() == 0 )
      return e_accept_drag::no;
    return e_accept_drag::motion;
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
    MenuContents const& contents,
    MenuPosition const& position ) {
  co_return co_await impl_->menu_threads_.open_menu( contents,
                                                     position );
}

} // namespace rn
