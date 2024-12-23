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
#include "config/menu-items.rds.hpp" // FIXME: remove

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

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
  MenuBarAnimState anim_state;
  MenuBarRenderedLayout render_layout;
  co::stream<MenuBarEvent> events;
  co::stream<MenuBarEventRaw> raw_events;
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
          switch( key_event.keycode ) {
            case ::SDLK_ESCAPE:
              return send_event( MenuBarEventRaw::close{} );
            default:
              break;
          }
          break;
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

static MenuContents tmp_build_contents() {
  MenuContents const orders_contents{
    .groups = {
      MenuItemGroup{
        .elems =
            {
              MenuElement::leaf{ .item = e_menu_item::fortify },
              MenuElement::leaf{ .item = e_menu_item::sentry },
              MenuElement::leaf{ .item = e_menu_item::dump },
            } },
    } };
  MenuContents const zoom_contents{
    .groups = {
      MenuItemGroup{
        .elems =
            {
              MenuElement::node{ .text = "Orders",
                                 .menu = orders_contents },
              MenuElement::leaf{ .item = e_menu_item::zoom_in },
              MenuElement::leaf{ .item = e_menu_item::zoom_out },
            } },
    } };
  MenuContents const contents{
    .groups = {
      MenuItemGroup{
        .elems =
            {
              MenuElement::leaf{ .item = e_menu_item::fortify },
              MenuElement::leaf{ .item = e_menu_item::sentry },
              MenuElement::leaf{ .item = e_menu_item::disband },
              MenuElement::leaf{ .item = e_menu_item::dump },
            } },
      MenuItemGroup{
        .elems =
            {
              MenuElement::node{ .text = "Zoom",
                                 .menu = zoom_contents },
            } },
      MenuItemGroup{
        .elems =
            {
              MenuElement::leaf{ .item = e_menu_item::plow },
              MenuElement::leaf{ .item = e_menu_item::road },
              MenuElement::leaf{ .item =
                                     e_menu_item::build_colony },
            } },
      MenuItemGroup{
        .elems =
            {
              MenuElement::node{ .text = "Zoom2",
                                 .menu = zoom_contents },
            } },
    } };
  return contents;
}

wait<> MenuBar::run_thread( MenuBarContents const& contents ) {
  state_ = make_unique<BarState>();
  SCOPE_EXIT { state_ = nullptr; };

  auto& st         = *state_;
  st.render_layout = build_menu_bar_rendered_layout( contents );

  wait<> const translater = translate_input_thread( st );
  using MenuResult        = e_menu_item;
  using MenuStream        = co::stream<MenuResult>;
  maybe<wait<>> menu_thread;
  auto menu_opener =
      [&]( MenuStream& stream, e_menu const menu,
           MenuContents const contents,
           MenuAllowedPositions const positions ) -> wait<> {
    st.anim_state.set_opened( menu );
    SCOPE_EXIT { st.anim_state.clear_focus(); };
    auto const item =
        co_await menu_server_.open_menu( contents, positions );
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
        MenuContents contents = tmp_build_contents();
        MenuAllowedPositions const positions =
            positions_for_header( st.render_layout,
                                  header_layout );
        // Must reset the thread before assigning to it because
        // it has cleanup actions that must run before the new
        // menu opener function is called and runs its own setup
        // stuff.
        menu_thread.reset();
        menu_thread = menu_opener( menu_stream, click.menu,
                                   contents, positions );
        break;
      }
    }
  }
  co_return;
}

} // namespace rn
