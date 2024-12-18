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
#include "co-time.hpp"
#include "co-wait.hpp"
#include "logger.hpp"
#include "menu-render.hpp"
#include "throttler.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/ui.rds.hpp"

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

struct MenuThreads::OpenMenu {
  MenuAnimState anim_state;
  MenuRenderLayout render_layout;
  co::stream<MenuEvent> events;
};

/****************************************************************
** MenuThreads
*****************************************************************/
MenuThreads::MenuThreads() = default;

MenuThreads::~MenuThreads() = default;

MenuAnimState const& MenuThreads::anim_state(
    int const menu_id ) const {
  auto const it = open_.find( menu_id );
  CHECK( it != open_.end() );
  return it->second.get().anim_state;
}

MenuRenderLayout const& MenuThreads::render_layout(
    int const menu_id ) const {
  auto const it = open_.find( menu_id );
  CHECK( it != open_.end() );
  return it->second.get().render_layout;
}

int MenuThreads::open_count() const { return open_.size(); }

int MenuThreads::next_menu_id() {
  int const menu_id = next_menu_id_++;
  CHECK( !open_.contains( menu_id ) );
  return menu_id;
}

void MenuThreads::unregister_menu( int const menu_id ) {
  open_.erase( menu_id );
}

bool MenuThreads::route_raw_input_thread(
    MenuEventRaw const& event ) {
  SWITCH( event ) {
    CASE( click ) {
      // TODO
      NOT_IMPLEMENTED;
    }
    CASE( close_all ) {
      for( auto& [menu_id, state] : open_ )
        routed_input_.send( RoutedMenuEventRaw{
          .menu_id = menu_id, .input = event } );
      break;
    }
    CASE( device ) {
      SWITCH( device.event ) {
        CASE( mouse_move_event ) {
          // TODO: need to reverse iteration.
          for( auto const& [menu_id, state] : open_ ) {
            auto const& bounds = state->render_layout.bounds;
            if( mouse_move_event.pos.is_inside( bounds ) ) {
              routed_input_.send( RoutedMenuEventRaw{
                .menu_id = menu_id, .input = event } );
              return true;
            }
          }
          break;
        }
        CASE( mouse_button_event ) {
          // TODO: need to reverse iteration.
          for( auto const& [menu_id, state] : open_ ) {
            auto const& bounds = state->render_layout.bounds;
            if( mouse_button_event.pos.is_inside( bounds ) ) {
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
  while( true ) {
    auto const routed = co_await routed_input_.next();
    int const menu_id = routed.menu_id;
    CHECK( open_.contains( menu_id ) );
    OpenMenu& menu     = open_[menu_id].get();
    auto const& layout = menu.render_layout;
    auto& sink         = menu.events;
    SWITCH( routed.input ) {
      CASE( click ) {
        sink.send( MenuEvent::click{ .item = click.item } );
        break;
      }
      CASE( close_all ) {
        sink.send( MenuEvent::close{} );
        break;
      }
      CASE( device ) {
        SWITCH( device.event ) {
          CASE( mouse_move_event ) {
            for( auto const& [item, item_layout] :
                 layout.items ) {
              auto const& bounds = item_layout.bounds_absolute;
              if( mouse_move_event.pos.is_inside( bounds ) ) {
                sink.send( MenuEvent::hover{ .item = item } );
                break;
              }
            }
            break;
          }
          CASE( mouse_button_event ) {
            for( auto const& [item, item_layout] :
                 layout.items ) {
              auto const& bounds = item_layout.bounds_absolute;
              if( mouse_button_event.pos.is_inside( bounds ) ) {
                sink.send( MenuEvent::click{ .item = item } );
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

// The click animation goes like this:
//
//   ----------------------------
//   |  on  |  off |  fade-time |
//   ----------------------------
//
// where each box is one "blink duration" and the "fade-time" is
// the time over which the entire menu body will fade away.
wait<> MenuThreads::animate_click( MenuAnimState& anim_state,
                                   e_menu_item const item ) {
  using namespace std::chrono;
  auto& conf = config_ui.menus;
  if( !conf.enable_click_animation ) co_return;
  milliseconds const kBlinkDuration =
      conf.click_animation_blink_duration_millis;
  milliseconds const kFadeDuration =
      conf.click_animation_fade_duration_millis;
  int const kBlinkCycles = conf.click_animation_blink_cycles;
  long const kFadeFrames = kFadeDuration / kFrameDuration;

  // Blinking.
  for( int i = 0; i < kBlinkCycles; ++i ) {
    anim_state.highlighted = nothing;
    co_await kBlinkDuration;
    anim_state.highlighted = item;
    co_await kBlinkDuration;
  }

  // Fading.
  anim_state.alpha = 1.0;
  AnimThrottler throttle( kAlmostStandardFrame );
  while( anim_state.alpha > 0 ) {
    co_await throttle();
    anim_state.alpha = std::max(
        0.0, anim_state.alpha - ( 1.0 / kFadeFrames ) );
  }
}

wait<maybe<e_menu_item>> MenuThreads::open_menu(
    MenuContents const contents, MenuPosition const& position ) {
  lg.info( "opening menu: {}", contents );
  int const menu_id = next_menu_id();
  SCOPE_EXIT { unregister_menu( menu_id ); };

  OpenMenu& om = open_[menu_id].get();
  om.render_layout =
      build_menu_rendered_layout( contents, position );

  wait<> const translater = translate_routed_input_thread();
  while( true ) {
    auto const event = co_await om.events.next();
    SWITCH( event ) {
      CASE( close ) { co_return nothing; }
      CASE( hover ) {
        om.anim_state.highlighted = hover.item;
        break;
      }
      CASE( click ) {
        om.anim_state.highlighted = click.item;
        co_await animate_click( om.anim_state, click.item );
        co_return click.item;
      }
    }
  }
}

} // namespace rn
