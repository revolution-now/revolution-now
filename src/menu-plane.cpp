/****************************************************************
**menu-plane.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: Implements the IMenuServer menu server.
*
*****************************************************************/
#include "menu-plane.hpp"

// Revolution Now
#include "menu-coro.hpp"
#include "menu-render.hpp"
#include "plane.hpp"

// config
#include "config/menu-items.rds.hpp"

// refl
#include "refl/enum-map.hpp"

// C++ standard library
#include <stack>

using namespace std;

namespace rn {

namespace {

using ::refl::enum_map;

} // namespace

/****************************************************************
** Menu2Plane::Impl
*****************************************************************/
struct Menu2Plane::Impl : IPlane, IMenuServer {
  // State.
  MenuThreads menu_threads_;
  enum_map<e_menu_item, stack<IPlane*>> handlers_;

  IPlane& impl() override { return *this; }

  wait<maybe<e_menu_item>> open_menu(
      MenuContents const& contents,
      MenuAllowedPositions const& positions ) override {
    co_return co_await menu_threads_.open_menu( contents,
                                                positions );
  }

  void on_logical_resolution_changed(
      e_resolution const /*resolution*/ ) override {}

  void draw( rr::Renderer& renderer ) const override {
    for( int const menu_id : menu_threads_.open_menu_ids() ) {
      auto const& anim_state =
          menu_threads_.anim_state( menu_id );
      auto const& render_layout =
          menu_threads_.render_layout( menu_id );
      render_menu_body( renderer, anim_state, render_layout,
                        *this );
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

  IMenuServer::Deregistrar register_handler(
      e_menu_item item, IPlane& plane ) override {
    handlers_[item].push( &plane );
    return IMenuServer::Deregistrar( *this, plane, item );
  }

  void unregister_handler( e_menu_item item,
                           IPlane& plane ) override {
    CHECK( !handlers_[item].empty() );
    CHECK( handlers_[item].top() == &plane );
    handlers_[item].pop();
  }

  bool can_handle_menu_click(
      e_menu_item const item ) const override {
    auto const& st = handlers_[item];
    if( st.empty() ) return false;
    CHECK( st.top() != nullptr );
    IPlane& plane = *st.top();
    return plane.will_handle_menu_click( item );
  }

  bool click_item( e_menu_item const item ) override {
    if( !can_handle_menu_click( item ) )
      // It is ok to call this on a menu item for which there is
      // no handler, even though it is not expected that will
      // happen. That is just to allow scenarios such as where
      // the user clicks an item and then while the click anima-
      // tion is running, something changes in another plane that
      // causes the item to not have a handler anymore. Returns
      // true if the click was actually made.
      return false;
    // The above function should have checked that we can do the
    // following safely.
    handlers_[item].top()->handle_menu_click( item );
    return true;
  }
};

/****************************************************************
** Menu2Plane
*****************************************************************/
Menu2Plane::~Menu2Plane() = default;

Menu2Plane::Menu2Plane() : impl_( new Impl() ) {}

IPlane& Menu2Plane::impl() { return impl_->impl(); }

wait<maybe<e_menu_item>> Menu2Plane::open_menu(
    MenuContents const& contents,
    MenuAllowedPositions const& positions ) {
  return impl_->open_menu( contents, positions );
}

Menu2Plane::Deregistrar Menu2Plane::register_handler(
    e_menu_item const item, IPlane& plane ) {
  return impl_->register_handler( item, plane );
}

void Menu2Plane::unregister_handler( e_menu_item const item,
                                     IPlane& plane ) {
  impl_->unregister_handler( item, plane );
}

bool Menu2Plane::can_handle_menu_click(
    e_menu_item const item ) const {
  return impl_->can_handle_menu_click( item );
}

bool Menu2Plane::click_item( e_menu_item const item ) {
  return impl_->click_item( item );
}

} // namespace rn
