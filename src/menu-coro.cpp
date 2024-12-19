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
#include "input.hpp"
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
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rl    = base::rl;
namespace views = std::ranges::views;

namespace rn {

namespace {

using ::gfx::e_side;
using ::gfx::point;

struct RoutedMenuEventRaw {
  int menu_id = {};
  MenuEventRaw input;
};

} // namespace

/****************************************************************
** MenuThreads::OpenMenu
*****************************************************************/
struct MenuThreads::OpenMenu {
  MenuContents contents;
  MenuPosition position;
  MenuAnimState anim_state;
  MenuRenderLayout render_layout;
  co::stream<RoutedMenuEventRaw> routed_input;
  co::stream<MenuEvent> events;

  maybe<MenuItemRenderLayout const&> layout_for_selected() {
    if( !anim_state.highlighted.has_value() ) return nothing;
    for( auto const& item_layout : render_layout.items )
      if( *anim_state.highlighted == item_layout.text )
        return item_layout;
    return nothing;
  }

  void cycle_highlight( auto const rng ) {
    auto const maybe_selected = layout_for_selected().member(
        &MenuItemRenderLayout::text );
    auto it = [&] {
      if( maybe_selected.has_value() ) {
        auto it = ranges::find_if(
            rng, [&]( auto const& item_layout ) {
              return item_layout.text == *maybe_selected;
            } );
        CHECK( it != rng.end() );
        ++it;
        return it;
      } else {
        return rng.begin();
      }
    }();
    if( it == rng.end() ) it = rng.begin();
    events.send( MenuEvent::over{ .text = it->text } );
  }

  void highlight_next() {
    if( render_layout.items.empty() ) return;
    cycle_highlight( ranges::views::all( render_layout.items ) );
  }

  void highlight_previous() {
    if( render_layout.items.empty() ) return;
    cycle_highlight(
        ranges::views::reverse( render_layout.items ) );
  }
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
  CHECK( open_.contains( menu_id ) );
  open_.erase( menu_id );
}

maybe<int> MenuThreads::menu_from_point(
    gfx::point const p ) const {
  // Reverse iteration so that we prefer menus created later,
  // which are "on top" of previous ones.
  for( auto const& [menu_id, open_menu] : rl::rall( open_ ) ) {
    auto const& bounds = open_menu->render_layout.bounds;
    if( p.is_inside( bounds ) ) return menu_id;
  }
  return nothing;
}

void MenuThreads::route_raw_input_thread(
    MenuEventRaw const& event ) {
  SWITCH( event ) {
    CASE( click ) {
      // TODO
      NOT_IMPLEMENTED;
    }
    CASE( close_all ) {
      for( auto& [menu_id, open_menu] : open_ )
        open_menu.get().routed_input.send( RoutedMenuEventRaw{
          .menu_id = menu_id, .input = event } );
      return;
    }
    CASE( device ) {
      SWITCH( device.event ) {
        CASE( key_event ) {
          if( !open_.empty() ) {
            auto& [menu_id, open_menu] = *open_.rbegin();
            open_menu.get().routed_input.send(
                RoutedMenuEventRaw{ .menu_id = menu_id,
                                    .input   = event } );
            return;
          }
          break;
        }
        CASE( mouse_move_event ) {
          UNWRAP_BREAK(
              menu_id, menu_from_point( mouse_move_event.pos ) );
          open_[menu_id].get().routed_input.send(
              RoutedMenuEventRaw{ .menu_id = menu_id,
                                  .input   = event } );
          break;
        }
        CASE( mouse_button_event ) {
          auto const menu_id =
              menu_from_point( mouse_button_event.pos );
          if( menu_id.has_value() ) {
            // Since the mouse is over an open menu we will al-
            // ways return here, though if it is the left button
            // then we pass it through.
            if( mouse_button_event.buttons ==
                input::e_mouse_button_event::left_up )
              open_[*menu_id].get().routed_input.send(
                  RoutedMenuEventRaw{ .menu_id = *menu_id,
                                      .input   = event } );
            return;
          }
          // This ensures that if there is a click outside of the
          // menus that it causes all to close.
          if( mouse_button_event.buttons ==
                  input::e_mouse_button_event::left_up ||
              mouse_button_event.buttons ==
                  input::e_mouse_button_event::right_up )
            route_raw_input_thread( MenuEventRaw::close_all{} );
          return;
        }
        default:
          break;
      }
      break;
    }
  }
}

void MenuThreads::handle_key_event(
    OpenMenu& open_menu, input::key_event_t const& key_event ) {
  if( key_event.change != input::e_key_change::down ) return;
  switch( key_event.keycode ) {
    case ::SDLK_ESCAPE:
      open_menu.events.send( MenuEvent::close{} );
      break;
    case ::SDLK_KP_8:
    case ::SDLK_UP:
      open_menu.highlight_previous();
      break;
    case ::SDLK_KP_2:
    case ::SDLK_DOWN:
      open_menu.highlight_next();
      break;
    case ::SDLK_KP_5:
    case ::SDLK_KP_ENTER:
    case ::SDLK_RETURN: {
      auto const selected_text =
          open_menu.layout_for_selected().member(
              &MenuItemRenderLayout::text );
      if( !selected_text.has_value() ) break;
      open_menu.events.send(
          MenuEvent::click{ .text = *selected_text } );
      break;
    }
    case ::SDLK_KP_4:
    case ::SDLK_LEFT: {
      auto const& pside = open_menu.position.parent_side;
      if( !pside.has_value() ) {
        auto const selected = open_menu.layout_for_selected();
        if( !selected.has_value() ) break;
        if( selected->has_arrow != true ) break;
        open_menu.events.send(
            MenuEvent::click{ .text = selected->text } );
        break;
      }
      switch( *pside ) {
        case e_side::left:
          open_menu.events.send( MenuEvent::close{} );
          break;
        case e_side::right: {
          auto const selected = open_menu.layout_for_selected();
          if( !selected.has_value() ) break;
          if( selected->has_arrow != true ) break;
          open_menu.events.send(
              MenuEvent::click{ .text = selected->text } );
          break;
        }
      }
      break;
    }
    case ::SDLK_KP_6:
    case ::SDLK_RIGHT: {
      auto const& pside = open_menu.position.parent_side;
      if( !pside.has_value() ) {
        auto const selected = open_menu.layout_for_selected();
        if( !selected.has_value() ) break;
        if( selected->has_arrow != true ) break;
        open_menu.events.send(
            MenuEvent::click{ .text = selected->text } );
        break;
      }
      switch( *pside ) {
        case e_side::left: {
          auto const selected = open_menu.layout_for_selected();
          if( !selected.has_value() ) break;
          if( selected->has_arrow != true ) break;
          open_menu.events.send(
              MenuEvent::click{ .text = selected->text } );
          break;
        }
        case e_side::right:
          open_menu.events.send( MenuEvent::close{} );
          break;
      }
      break;
    }
    default:
      break;
  }
}

wait<> MenuThreads::translate_routed_input_thread(
    int const menu_id ) {
  while( true ) {
    auto const routed =
        co_await open_[menu_id]->routed_input.next();
    int const menu_id = routed.menu_id;
    CHECK( open_.contains( menu_id ) );
    OpenMenu& menu     = open_[menu_id].get();
    auto const& layout = menu.render_layout;
    auto& sink         = menu.events;
    SWITCH( routed.input ) {
      CASE( click ) {
        sink.send( MenuEvent::click{ .text = click.text } );
        break;
      }
      CASE( close_all ) {
        sink.send( MenuEvent::close{} );
        break;
      }
      CASE( device ) {
        SWITCH( device.event ) {
          CASE( key_event ) {
            handle_key_event( menu, key_event );
            break;
          }
          CASE( mouse_move_event ) {
            for( auto const& item_layout : layout.items ) {
              auto const& bounds = item_layout.bounds_absolute;
              if( mouse_move_event.pos.is_inside( bounds ) ) {
                sink.send( MenuEvent::over{
                  .text = item_layout.text } );
                break;
              }
            }
            break;
          }
          CASE( mouse_button_event ) {
            for( auto const& item_layout : layout.items ) {
              auto const& bounds = item_layout.bounds_absolute;
              if( mouse_button_event.pos.is_inside( bounds ) ) {
                sink.send( MenuEvent::click{
                  .text = item_layout.text } );
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
                                   string const& text ) {
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
    anim_state.highlighted = text;
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
    MenuContents const contents, MenuPosition const position ) {
  int const menu_id = next_menu_id();
  SCOPE_EXIT { unregister_menu( menu_id ); };

  OpenMenu& om = open_[menu_id].get();
  om.contents  = contents;
  om.position  = position;
  om.render_layout =
      build_menu_rendered_layout( contents, position );

  wait<> const translater =
      translate_routed_input_thread( menu_id );
  using SubMenuResult = maybe<e_menu_item>;
  using SubMenuStream = co::stream<SubMenuResult>;
  maybe<wait<>> sub_menu_thread;
  auto sub_menu_opener =
      [this]( SubMenuStream& stream, MenuContents const contents,
              MenuPosition const position ) -> wait<> {
    stream.send( co_await open_menu( contents, position ) );
  };
  SubMenuStream sub_menu_stream;
  while( true ) {
    auto const next = co_await
        [&]() -> wait<variant<SubMenuResult, MenuEvent>> {
      co_return co_await co::first( sub_menu_stream.next(),
                                    om.events.next() );
    }();
    MenuEvent event;
    switch( next.index() ) {
      case 0: {
        auto const& o = get<0>( next );
        if( o.has_value() ) co_return *o;
        continue;
      }
      case 1: {
        auto const& o = get<1>( next );
        event         = o;
        break;
      }
    }
    SWITCH( event ) {
      CASE( close ) { co_return nothing; }
      CASE( over ) {
        om.anim_state.highlighted = over.text;
        break;
      }
      CASE( click ) {
        om.anim_state.highlighted = click.text;
        auto it = om.render_layout.items.begin();
        for( auto const& grp : contents.groups ) {
          for( auto const& elem : grp.elems ) {
            CHECK( it != om.render_layout.items.end() );
            if( it->text == click.text ) {
              SWITCH( elem ) {
                CASE( leaf ) {
                  // Close sub menus first before animating this
                  // click because we clicked an item in the main
                  // menu.
                  sub_menu_thread.reset();
                  co_await animate_click( om.anim_state,
                                          click.text );
                  co_return leaf.item;
                }
                CASE( node ) {
                  MenuPosition const sub_menu_position{
                    .where       = it->bounds_absolute.se(),
                    .corner      = e_direction::sw,
                    .parent_side = e_side::left };
                  sub_menu_thread = sub_menu_opener(
                      sub_menu_stream, node.menu,
                      sub_menu_position );
                  break;
                }
              }
            }
            ++it;
          }
        }
      }
    }
  }
}

} // namespace rn
