/****************************************************************
**menu-bar.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-22.
*
* Description: Runs the top menu bar.
*
*****************************************************************/
#include "menu-bar.hpp"

// rds
#include "menu-bar-impl.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "imenu-server.hpp"
#include "menu-render.hpp"

// config
#include "config/menu.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rl    = base::rl;
namespace views = std::ranges::views;

namespace rn {

namespace {

using ::gfx::point;

MenuAllowedPositions positions_for_header(
    MenuBarRenderedLayout const& bar_layout,
    MenuHeaderRenderLayout const& layout ) {
  auto const menu_position =
      [&]( e_diagonal_direction const parent,
           e_diagonal_direction const child ) {
        return MenuAllowedPosition{
          .where =
              layout.bounds_absolute.corner( parent ).with_y(
                  bar_layout.bounds.bottom() ),
          .orientations_allowed = { child },
        };
      };
  using enum e_diagonal_direction;
  return MenuAllowedPositions{
    .positions_allowed = { menu_position( sw, nw ),
                           menu_position( se, ne ) } };
}

} // namespace

/****************************************************************
** MenuBar::BarState
*****************************************************************/
struct MenuBar::BarState {
  MenuBarContents contents;
  MenuBarAnimState anim_state;
  MenuBarRenderedLayout render_layout;
  co::stream<MenuBarEvent> events;
  co::stream<MenuBarEventRaw> raw_events;

  e_menu cycle_focus( ranges::view auto const rng ) const {
    CHECK( !rng.empty() );
    UNWRAP_CHECK( current, anim_state.focused );
    auto const next =
        rl::all( rng )
            .cycle()
            .drop_while_L( _ != current )
            .drop( 1 )          // drop current.
            .take( rng.size() ) // prevent infinite loops.
            .head();
    CHECK( next.has_value() );
    return *next;
  }

  void focus_next() {
    e_menu const new_menu =
        cycle_focus( views::all( contents.menus ) );
    events.send( MenuBarEvent::over{ .menu = new_menu } );
  }

  void focus_prev() {
    e_menu const new_menu =
        cycle_focus( views::reverse( contents.menus ) );
    events.send( MenuBarEvent::over{ .menu = new_menu } );
  }
};

/****************************************************************
** MenuBar
*****************************************************************/
MenuBar::MenuBar( IMenuServer& menu_server )
  : menu_server_( menu_server ) {}

MenuBar::~MenuBar() = default;

MenuBar::BarState& MenuBar::state() {
  CHECK( state_ != nullptr );
  return *state_;
}

MenuBar::BarState const& MenuBar::state() const {
  CHECK( state_ != nullptr );
  return *state_;
}

MenuBarAnimState const& MenuBar::anim_state() const {
  return state().anim_state;
}

MenuBarRenderedLayout const& MenuBar::render_layout() const {
  return state().render_layout;
}

maybe<MenuHeaderRenderLayout const&> MenuBar::layout_for_menu(
    e_menu const menu ) const {
  if( !state_ ) return nothing;
  auto& st = state();
  for( auto const& header : st.render_layout.headers )
    if( header.menu == menu ) //
      return header;
  return nothing;
}

maybe<e_menu> MenuBar::header_from_point( point const p ) const {
  if( !state_ ) return nothing;
  auto& st = state();
  for( auto const& header : st.render_layout.headers )
    if( p.is_inside( header.bounds_absolute ) )
      return header.menu;
  return nothing;
}

bool MenuBar::handle_alt_key(
    input::e_key_change const change ) {
  if( !state_ ) return false;
  auto& st = state();

  if( st.anim_state.opened_menu().has_value() &&
      change == input::e_key_change::down ) {
    // Menus are open and the user is pressing an alt key, so
    // close menus.
    st.events.send( MenuBarEvent::close{} );
    return true;
  }

  if( !st.anim_state.focused.has_value() &&
      change == input::e_key_change::down ) {
    // There is no focus and the user has pressed an alt key, so
    // add highlighting to the first menu header.
    if( st.contents.menus.empty() ) return true;
    st.events.send(
        MenuBarEvent::over{ .menu = st.contents.menus[0] } );
    return true;
  }

  if( !st.anim_state.opened_menu().has_value() &&
      change == input::e_key_change::up ) {
    // Menus are closed and the user is releasing an alt key, so
    // remove any highlighting from the menu headers.
    st.anim_state.clear_focus();
    return true;
  }

  return false;
}

bool MenuBar::handle_alt_shortcut(
    input::key_event_t const& key_event ) {
  if( !state_ ) return false;
  auto& st = state();
  // Check for an alt-shortcut key to open a menu.
  for( e_menu const menu : st.contents.menus ) {
    if( key_event.keycode ==
        tolower( config_menu.menus[menu].shortcut ) ) {
      if( st.anim_state.opened_menu() != menu )
        st.events.send( MenuBarEvent::click{ .menu = menu } );
      return true;
    }
  }
  // This prevents an invalid shortcut key from passing through
  // to underlying planes.
  return true;
}

bool MenuBar::handle_key_event(
    input::key_event_t const& key_event ) {
  if( !state_ ) return false;
  auto& st          = state();
  bool const is_alt = ( key_event.keycode == ::SDLK_LALT ||
                        key_event.keycode == ::SDLK_RALT );
  if( is_alt ) return handle_alt_key( key_event.change );
  if( key_event.change != input::e_key_change::down )
    return true;
  if( key_event.mod.alt_down )
    return handle_alt_shortcut( key_event );
  switch( key_event.keycode ) {
    case ::SDLK_ESCAPE:
      menu_server_.close_all_menus();
      return true;
    case ::SDLK_KP_4:
    case ::SDLK_LEFT: {
      if( !st.anim_state.focused.has_value() ) break;
      st.focus_prev();
      return true;
    }
    case ::SDLK_KP_6:
    case ::SDLK_RIGHT: {
      if( !st.anim_state.focused.has_value() ) break;
      st.focus_next();
      return true;
    }
    case ::SDLK_KP_5:
    case ::SDLK_KP_ENTER:
    case ::SDLK_RETURN: {
      if( !st.anim_state.focused.has_value() ) break;
      if( st.anim_state.opened_menu().has_value() )
        // There is a menu open so left it handle this
        // event.
        return false;
      st.events.send( MenuBarEvent::click{
        .menu = *st.anim_state.focused } );
      return true;
    }
    default:
      return false;
  }
  return false;
}

bool MenuBar::send_event( MenuBarEventRaw const& event ) {
  if( !state_ ) return false;
  auto& st = state();
  SWITCH( event ) {
    CASE( close ) {
      st.events.send( MenuBarEvent::close{} );
      return true;
    }
    CASE( device ) {
      SWITCH( device.event ) {
        CASE( key_event ) {
          return handle_key_event( key_event );
        }
        CASE( mouse_move_event ) {
          auto const header =
              header_from_point( mouse_move_event.pos );
          if( header.has_value() ) {
            st.events.send(
                MenuBarEvent::over{ .menu = *header } );
            return true;
          }
          if( st.anim_state.highlighted_menu().has_value() )
            st.anim_state.clear_focus();
          return false;
        }
        CASE( mouse_button_event ) {
          UNWRAP_BREAK( header, header_from_point(
                                    mouse_button_event.pos ) );
          if( mouse_button_event.buttons ==
              input::e_mouse_button_event::left_down )
            st.events.send(
                MenuBarEvent::click{ .menu = header } );
          return true;
        }
        default:
          break;
      }
      break;
    }
  }
  return false;
}

wait<> MenuBar::translate_input_thread( BarState& st ) {
  while( true ) {
    auto const raw     = co_await st.raw_events.next();
    auto& sink         = st.events;
    auto const& layout = st.render_layout;
    SWITCH( raw ) {
      CASE( close ) {
        sink.send( MenuBarEvent::close{} );
        break;
      }
      CASE( device ) {
        SWITCH( device.event ) {
          CASE( key_event ) {
            // handle_key_event( menu, key_event );
            break;
          }
          CASE( mouse_move_event ) {
            for( auto const& header : layout.headers ) {
              auto const& bounds = header.bounds_absolute;
              if( mouse_move_event.pos.is_inside( bounds ) ) {
                sink.send(
                    MenuBarEvent::over{ .menu = header.menu } );
                break;
              }
            }
            break;
          }
          CASE( mouse_button_event ) {
            for( auto const& header : layout.headers ) {
              auto const& bounds = header.bounds_absolute;
              if( mouse_button_event.pos.is_inside( bounds ) ) {
                sink.send(
                    MenuBarEvent::click{ .menu = header.menu } );
                break;
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
  }
  co_return;
}

void MenuBar::send_click( e_menu_item item ) const {
  if( !menu_server_.can_handle_menu_click( item ) ) return;
  menu_server_.click_item( item );
}

wait<> MenuBar::run_thread( MenuBarContents const& contents ) {
  state_ = make_unique<BarState>();
  SCOPE_EXIT { state_ = nullptr; };

  auto& st         = *state_;
  st.contents      = contents;
  st.render_layout = build_menu_bar_rendered_layout( contents );

  wait<> const translater = translate_input_thread( st );
  using MenuResult        = e_menu_item;
  using MenuStream        = co::stream<MenuResult>;
  maybe<wait<>> menu_thread;
  auto menu_opener =
      [&]( MenuStream& stream, e_menu const menu,
           MenuAllowedPositions const positions ) -> wait<> {
    // This is in case there are any menus open that are not as-
    // sociated with the menu bar (e.g. pop up menus).
    menu_server_.close_all_menus();
    st.anim_state.set_opened( menu );
    SCOPE_EXIT { st.anim_state.clear_focus(); };
    auto const item =
        co_await menu_server_.open_menu( menu, positions );
    if( item.has_value() ) stream.send( *item );
  };
  MenuStream menu_stream;
  while( true ) {
    auto const next = co_await
        [&]() -> wait<variant<MenuResult, MenuBarEvent>> {
      co_return co_await co::first( menu_stream.next(),
                                    st.events.next() );
    }();
    MenuBarEvent event;
    switch( next.index() ) {
      case 0: {
        auto const& o = get<0>( next );
        send_click( o );
        continue;
      }
      case 1: {
        auto const& o = get<1>( next );
        event         = o;
        break;
      }
      default:
        SHOULD_NOT_BE_HERE;
    }
    SWITCH( event ) {
      CASE( close ) {
        menu_thread.reset();
        break;
      }
      CASE( over ) {
        if( !st.anim_state.highlighted_menu() &&
            !st.anim_state.opened_menu() ) {
          menu_thread.reset();
          st.anim_state.set_highlighted( over.menu );
          break;
        }
        // There is already focus.
        if( st.anim_state.highlighted_menu() == over.menu ||
            st.anim_state.opened_menu() == over.menu )
          break;

        if( st.anim_state.highlighted_menu().has_value() ) {
          st.anim_state.set_highlighted( over.menu );
          break;
        }

        if( st.anim_state.opened_menu().has_value() ) {
          // There is a menu open but we are hovering over a
          // different one.
          st.events.send(
              MenuBarEvent::click{ .menu = over.menu } );
          break;
        }
        break;
      }
      CASE( click ) {
        if( st.anim_state.opened_menu() == click.menu ) {
          menu_thread.reset();
          st.anim_state.set_highlighted( click.menu );
          break;
        }
        UNWRAP_CHECK( header_layout,
                      layout_for_menu( click.menu ) );
        MenuAllowedPositions const positions =
            positions_for_header( st.render_layout,
                                  header_layout );
        // Must reset the thread before assigning to it because
        // it has cleanup actions that must run before the new
        // menu opener function is called and runs its own setup
        // stuff.
        menu_thread.reset();
        menu_thread =
            menu_opener( menu_stream, click.menu, positions );
        break;
      }
    }
  }
  co_return;
}

} // namespace rn
