/****************************************************************
**menu-coro.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-15.
*
* Description: Coroutines for running menus.
*
*****************************************************************/
#include "menu-coro.hpp"

// rds
#include "menu-coro-impl.rds.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "logger.hpp"
#include "menu-render.hpp"

// config
#include "config/menu-items.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;

} // namespace

struct MenuThreads::MenuState {
  MenuRenderState render_state;
  co::stream<MenuEvent> events;
};

/****************************************************************
** MenuThreads
*****************************************************************/
MenuThreads::MenuThreads() = default;

MenuThreads::~MenuThreads() = default;

int MenuThreads::open_count() const { return open_.size(); }

int MenuThreads::next_menu_id() {
  int const menu_id = next_menu_id_++;
  CHECK( !open_.contains( menu_id ) );
  return menu_id;
}

void MenuThreads::unregister_menu( int const menu_id ) {
  open_.erase( menu_id );
}

void MenuThreads::on_all_render_states(
    base::function_ref<void( MenuRenderState const& )> const fn )
    const {
  for( auto const& [_, state] : open_ )
    fn( state.get().render_state );
}

bool MenuThreads::route_raw_input_thread(
    MenuEventRaw const& event ) {
  SWITCH( event ) {
    CASE( click ) {
      // TODO
      break;
    }
    CASE( close_all ) {
      for( auto& [menu_id, state] : open_ )
        state.get().events.send( MenuEvent::close{} );
      break;
    }
    CASE( device ) {
      switch( device.event.to_enum() ) {
        case input::e_input_event::mouse_button_event: {
          auto& val =
              device.event.get<input::mouse_button_event_t>();
          // TODO: need to reverse iteration.
          for( auto const& [menu_id, state] : open_ ) {
            MenuRenderer menu_renderer( state->render_state );
            if( val.pos.is_inside(
                    menu_renderer.bounding_rect() ) ) {
              routed_input_.send( RoutedMenuEventRaw{
                .menu_id = menu_id, .input = event } );
              return true;
            }
          }
          break;
        }
        default:
          break;
      }
      break;
    }
  }

  return false;
}

wait<> MenuThreads::translate_routed_input_thread() {
  // TODO
  co_return;
}

wait<> MenuThreads::animate_click(
    MenuRenderState& /*render_state*/ ) {
  // TODO
  co_return;
}

wait<maybe<e_menu_item>> MenuThreads::open_menu(
    MenuLayout const layout, MenuPosition const& position ) {
  lg.info( "opening menu: {}", layout );
  int const menu_id = next_menu_id();
  SCOPE_EXIT { unregister_menu( menu_id ); };

  MenuState& state            = open_[menu_id].get();
  state.render_state.layout   = layout;
  state.render_state.position = position;

  while( true ) {
    auto const event = co_await state.events.next();
    SWITCH( event ) {
      CASE( close ) { co_return nothing; }
      CASE( hover ) {
        state.render_state.highlighted = hover.item;
        break;
      }
      CASE( click ) {
        state.render_state.highlighted = click.item;
        co_await animate_click( state.render_state );
        co_return click.item;
      }
    }
  }
}

} // namespace rn
