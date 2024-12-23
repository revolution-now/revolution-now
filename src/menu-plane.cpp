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
#include "logger.hpp"
#include "menu-bar.hpp"
#include "menu-body.hpp"
#include "menu-render.hpp"
#include "plane.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/menu.rds.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/to-str.hpp"

// C++ standard library
#include <stack>

using namespace std;

namespace rn {

namespace {

using ::refl::enum_map;

} // namespace

/****************************************************************
** MenuPlane::Impl
*****************************************************************/
struct MenuPlane::Impl : IPlane, IMenuServer {
  // State.
  MenuThreads menu_threads_;
  MenuBar bar_;
  maybe<wait<>> bar_thread_;
  vector<e_menu> bar_contents_;
  // This cheat menu on/off state unfortunately has to be kept
  // separate from the game settings chean enabled flag since
  // this menu plane is not associated with a single game state;
  // it can live throughout the duration of the program. So this
  // needs to be kept in sync with whatever the current loaded
  // game is configured to have.
  bool cheat_menu_ = false;
  enum_map<e_menu_item, stack<IPlane*>> handlers_;

  Impl() : menu_threads_( *this ), bar_( *this ) {
    populate_menu_bar_contents();
    start_bar_thread_if_not_running();
  }

  IPlane& impl() override { return *this; }

  void populate_menu_bar_contents() {
    bar_contents_.clear();
    for( e_menu const menu : config_menu.menu_bar ) {
      if( menu == e_menu::cheat && !cheat_menu_ ) continue;
      bar_contents_.push_back( menu );
    }
  }

  void start_bar_thread_if_not_running() {
    if( !bar_is_running() )
      bar_thread_ = bar_.run_thread( bar_contents_ );
  }

  void restart_bar_thread_if_running() {
    if( bar_is_running() ) {
      bar_thread_.reset();
      start_bar_thread_if_not_running();
    }
  }

  wait<maybe<e_menu_item>> open_menu(
      e_menu const menu,
      MenuAllowedPositions const& positions ) override {
    co_return co_await menu_threads_.open_menu( menu,
                                                positions );
  }

  bool bar_is_running() const { return bar_thread_.has_value(); }

  void show_menu_bar( bool const show ) override {
    if( !show ) {
      bar_thread_.reset();
      return;
    }
    start_bar_thread_if_not_running();
  }

  void enable_cheat_menu( bool const show ) override {
    cheat_menu_ = show;
    populate_menu_bar_contents();
    restart_bar_thread_if_running();
  }

  void close_all_menus() override {
    if( menu_threads_.open_count() != 0 )
      menu_threads_.send_event( MenuEventRaw::close_all{} );
  }

  void on_logical_resolution_changed(
      e_resolution const /*resolution*/ ) override {
    close_all_menus();
    restart_bar_thread_if_running();
  }

  void draw( rr::Renderer& renderer ) const override {
    if( bar_is_running() ) {
      auto const& anim_state    = bar_.anim_state();
      auto const& render_layout = bar_.render_layout();
      render_menu_bar( renderer, anim_state, render_layout );
    }
    for( int const menu_id : menu_threads_.open_menu_ids() ) {
      auto const& anim_state =
          menu_threads_.anim_state( menu_id );
      auto const& render_layout =
          menu_threads_.render_layout( menu_id );
      auto const enabled_fn =
          [&]( MenuItemRenderLayout const& item_layout ) {
            return menu_threads_.enabled( item_layout );
          };
      render_menu_body( renderer, anim_state, render_layout,
                        enabled_fn );
    }
  }

  e_input_handled input( input::event_t const& event ) override {
    if( event.holds<input::cheat_event_t>() )
      // Let this pass through because we don't handle it here
      // and the alt-WIN key combination can sometimes cause
      // menus to pop open which interfere with the cheat event
      // making it down to lower planes.
      return e_input_handled::no;

    if( bar_is_running() ) {
      auto const raw = MenuBarEventRaw::device{ .event = event };
      if( bar_.send_event( raw ) ) return e_input_handled::yes;
    }

    if( menu_threads_.open_count() != 0 ) { // Menu threads.
      auto const raw = MenuEventRaw::device{ .event = event };
      menu_threads_.send_event( raw );
      return e_input_handled::yes;
    }

    return e_input_handled::no;
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
    lg.info( "clicked on menu item {}.", item );
    // The above function should have checked that we can do the
    // following safely.
    handlers_[item].top()->handle_menu_click( item );
    return true;
  }
};

/****************************************************************
** MenuPlane
*****************************************************************/
MenuPlane::~MenuPlane() = default;

MenuPlane::MenuPlane() : impl_( new Impl() ) {}

IPlane& MenuPlane::impl() { return impl_->impl(); }

wait<maybe<e_menu_item>> MenuPlane::open_menu(
    e_menu const menu, MenuAllowedPositions const& positions ) {
  return impl_->open_menu( menu, positions );
}

void MenuPlane::show_menu_bar( bool const show ) {
  impl_->show_menu_bar( show );
}

void MenuPlane::close_all_menus() { impl_->close_all_menus(); }

void MenuPlane::enable_cheat_menu( bool const show ) {
  impl_->enable_cheat_menu( show );
}

MenuPlane::Deregistrar MenuPlane::register_handler(
    e_menu_item const item, IPlane& plane ) {
  return impl_->register_handler( item, plane );
}

void MenuPlane::unregister_handler( e_menu_item const item,
                                    IPlane& plane ) {
  impl_->unregister_handler( item, plane );
}

bool MenuPlane::can_handle_menu_click(
    e_menu_item const item ) const {
  return impl_->can_handle_menu_click( item );
}

bool MenuPlane::click_item( e_menu_item const item ) {
  return impl_->click_item( item );
}

} // namespace rn
