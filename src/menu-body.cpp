/****************************************************************
**menu-body.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-15.
*
* Description: Coroutines for running menus.
*
*****************************************************************/
#include "menu-body.hpp"

// rds
#include "menu-body-impl.rds.hpp"

// Revolution Now
#include "co-time.hpp"
#include "co-wait.hpp"
#include "imenu-server.hpp"
#include "input.hpp"
#include "menu-render.hpp"
#include "throttler.hpp"

// config
#include "config/menu-items.rds.hpp"
#include "config/menu.rds.hpp"
#include "config/ui.rds.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/function-ref.hpp"
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

using ::base::function_ref;
using ::gfx::e_side;
using ::gfx::point;
using ::gfx::rect;

} // namespace

/****************************************************************
** MenuThreads::OpenMenu
*****************************************************************/
struct MenuThreads::OpenMenu {
  OpenMenu( MenuRenderLayout render_layout )
    : render_layout( render_layout ) {}

  e_menu menu                    = {};
  MenuAllowedPositions positions = {};
  MenuAnimState anim_state       = {};
  MenuRenderLayout const render_layout;
  co::stream<MenuEventRaw> routed_input = {};
  co::stream<MenuEvent> events          = {};
  maybe<wait<>> hover_timer             = {};

  maybe<MenuItemRenderLayout const&> layout_for_selected()
      const {
    if( !anim_state.highlighted.has_value() ) return nothing;
    for( auto const& item_layout : render_layout.items )
      if( *anim_state.highlighted == item_layout.text )
        return item_layout;
    return nothing;
  }

  maybe<config::menu::MenuElement const&> find_element(
      string const& text ) const {
    auto it              = render_layout.items.begin();
    auto const& contents = config_menu.layout[menu].contents;
    for( auto const& elem : contents ) {
      if( !elem.has_value() ) continue;
      CHECK( it != render_layout.items.end() );
      if( it->text == text ) return elem;
      ++it;
    }
    return nothing;
  }

  using EnabledFn = bool( MenuItemRenderLayout const& ) const;

  void cycle_highlight(
      ranges::view auto const rng,
      function_ref<EnabledFn> const enabled_fn ) {
    if( rng.empty() ) return;

    auto const current =
        layout_for_selected()
            .member( &MenuItemRenderLayout::text )
            .value_or( ( rng.end() - 1 )->text );

    auto const next =
        rl::all( rng )
            .cycle()
            .drop_while_L( _.text != current )
            .drop( 1 )          // drop current.
            .take( rng.size() ) // prevent infinite loops.
            .remove_if_L( !enabled_fn( _ ) )
            .head();

    if( next.has_value() )
      events.send( MenuEvent::highlight{ .text = next->text } );
  }

  void highlight_next(
      function_ref<EnabledFn> const enabled_fn ) {
    cycle_highlight( views::all( render_layout.items ),
                     enabled_fn );
  }

  void highlight_previous(
      function_ref<EnabledFn> const enabled_fn ) {
    cycle_highlight( views::reverse( render_layout.items ),
                     enabled_fn );
  }

  // Need to make this a member function as opposed to a lambda
  // for lifetime reasons.
  wait<> hover_timer_thread() {
    co_await 300ms;
    events.send( MenuEvent::hover{} );
  }

  void set_hover_timer() {
    hover_timer.reset();
    // Need to make this a member function as opposed to a lambda
    // for lifetime reasons.
    hover_timer = hover_timer_thread();
  }

  MenuAllowedPositions prioritized_submenu_positions(
      MenuItemRenderLayout const& layout ) const {
    auto const submenu_position =
        [&]( e_diagonal_direction const parent,
             e_diagonal_direction const child ) {
          return MenuAllowedPosition{
            .where =
                layout.bounds_absolute.corner( parent ).with_x(
                    render_layout.bounds.corner( parent ).x ),
            .orientation = child,
            .parent_side = reverse( side_for( parent ) ) };
        };
    // Sub-menu positions relative to the rect of the row that is
    // currently being selected. So e.g. (ne,nw) here means that
    // the origin of the submenu will be placed on the ne corner
    // of the row rect (adjusted for the outter menu border) and
    // the orientation will be "nw", meaning that the nw corner
    // of the submenu will be at that point.
    //
    // Note that "overlapping" below means that the child menu
    // will be on top of the parent menu, thus those configura-
    // tions are given less priority.
    switch( render_layout.position.parent_side.value_or(
        e_side::left ) ) {
      using enum e_diagonal_direction;
      case e_side::right:
        return MenuAllowedPositions{
          .positions_allowed = {
            submenu_position( nw, ne ),
            submenu_position( sw, se ),
            submenu_position( ne, nw ),
            submenu_position( se, sw ),
            // Overlapping.
            submenu_position( nw, nw ),
            submenu_position( sw, sw ),
            submenu_position( ne, ne ),
            submenu_position( se, se ),
          } };
      case e_side::left:
        return MenuAllowedPositions{
          .positions_allowed = {
            submenu_position( ne, nw ),
            submenu_position( se, sw ),
            submenu_position( nw, ne ),
            submenu_position( sw, se ),
            // Overlapping.
            submenu_position( ne, ne ),
            submenu_position( se, se ),
            submenu_position( nw, nw ),
            submenu_position( sw, sw ),
          } };
    }
  }
};

/****************************************************************
** MenuThreads
*****************************************************************/
MenuThreads::MenuThreads( IMenuServer const& menu_server,
                          rr::ITextometer const& textometer )
  : menu_server_( menu_server ), textometer_( textometer ) {}

MenuThreads::~MenuThreads() = default;

MenuAnimState const& MenuThreads::anim_state(
    int const menu_id ) const {
  auto const it = open_.find( menu_id );
  CHECK( it != open_.end() );
  return it->second->anim_state;
}

MenuRenderLayout const& MenuThreads::render_layout(
    int const menu_id ) const {
  auto const it = open_.find( menu_id );
  CHECK( it != open_.end() );
  return it->second->render_layout;
}

int MenuThreads::open_count() const { return open_.size(); }

bool MenuThreads::enabled(
    MenuItemRenderLayout const& layout ) const {
  if( !layout.item.has_value() )
    // This is a node (opens sub-menu); always enabled.
    return true;
  return menu_server_.can_handle_menu_click( *layout.item );
}

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

void MenuThreads::send_event( MenuEventRaw const& event ) {
  SWITCH( event ) {
    CASE( close_all ) {
      for( auto& [menu_id, open_menu] : open_ )
        open_menu.get().routed_input.send( event );
      return;
    }
    CASE( device ) {
      SWITCH( device.event ) {
        CASE( key_event ) {
          if( !open_.empty() ) {
            auto& [menu_id, open_menu] = *open_.rbegin();
            open_menu.get().routed_input.send( event );
            return;
          }
          break;
        }
        CASE( mouse_move_event ) {
          UNWRAP_BREAK(
              menu_id, menu_from_point( mouse_move_event.pos ) );
          lookup_menu( menu_id ).routed_input.send( event );
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
              lookup_menu( *menu_id ).routed_input.send( event );
            return;
          }
          // This ensures that if there is a click outside of the
          // menus that it causes all to close.
          if( mouse_button_event.buttons ==
                  input::e_mouse_button_event::left_up ||
              mouse_button_event.buttons ==
                  input::e_mouse_button_event::right_up )
            send_event( MenuEventRaw::close_all{} );
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
  auto const enabled_fn =
      [&]( MenuItemRenderLayout const& layout ) {
        return enabled( layout );
      };
  switch( key_event.keycode ) {
    case ::SDLK_ESCAPE:
      open_menu.events.send( MenuEvent::close{} );
      break;
    case ::SDLK_KP_8:
    case ::SDLK_UP:
      open_menu.highlight_previous( enabled_fn );
      break;
    case ::SDLK_KP_2:
    case ::SDLK_DOWN:
      open_menu.highlight_next( enabled_fn );
      break;
    case ::SDLK_KP_5:
    case ::SDLK_KP_ENTER:
    case ::SDLK_RETURN: {
      if( !open_menu.layout_for_selected().has_value() ) break;
      open_menu.events.send( MenuEvent::click{} );
      break;
    }
    case ::SDLK_KP_4:
    case ::SDLK_LEFT: {
      auto const& pside =
          open_menu.render_layout.position.parent_side;
      if( !pside.has_value() ) {
        auto const selected = open_menu.layout_for_selected();
        if( !selected.has_value() ) break;
        if( selected->has_arrow != true ) break;
        open_menu.events.send( MenuEvent::click{} );
        break;
      }
      switch( *pside ) {
        case e_side::left:
          open_menu.events.send( MenuEvent::close{} );
          break;
        case e_side::right: {
          auto const selected = open_menu.layout_for_selected();
          if( !selected.has_value() ) {
            open_menu.highlight_next( enabled_fn );
            break;
          }
          if( selected->has_arrow != true ) break;
          open_menu.events.send( MenuEvent::click{} );
          break;
        }
      }
      break;
    }
    case ::SDLK_KP_6:
    case ::SDLK_RIGHT: {
      auto const& pside =
          open_menu.render_layout.position.parent_side;
      if( !pside.has_value() ) {
        auto const selected = open_menu.layout_for_selected();
        if( !selected.has_value() ) break;
        if( selected->has_arrow != true ) break;
        open_menu.events.send( MenuEvent::click{} );
        break;
      }
      switch( *pside ) {
        case e_side::left: {
          auto const selected = open_menu.layout_for_selected();
          if( !selected.has_value() ) {
            open_menu.highlight_next( enabled_fn );
            break;
          }
          if( selected->has_arrow != true ) break;
          open_menu.events.send( MenuEvent::click{} );
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
        co_await lookup_menu( menu_id ).routed_input.next();
    CHECK( open_.contains( menu_id ) );
    OpenMenu& menu     = lookup_menu( menu_id );
    auto const& layout = menu.render_layout;
    auto& sink         = menu.events;
    SWITCH( routed ) {
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
                if( enabled( item_layout ) )
                  sink.send( MenuEvent::over{
                    .text = item_layout.text } );
                else
                  sink.send( MenuEvent::deselect{} );
                break;
              }
            }
            break;
          }
          CASE( mouse_button_event ) {
            for( auto const& item_layout : layout.items ) {
              auto const& bounds = item_layout.bounds_absolute;
              if( mouse_button_event.pos.is_inside( bounds ) ) {
                if( !enabled( item_layout ) ) break;
                sink.send( MenuEvent::click{} );
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

MenuThreads::OpenMenu& MenuThreads::lookup_menu(
    int const menu_id ) {
  auto const iter = open_.find( menu_id );
  CHECK( iter != open_.end() );
  return *iter->second;
}

MenuThreads::OpenMenu const& MenuThreads::lookup_menu(
    int const menu_id ) const {
  auto const iter = open_.find( menu_id );
  CHECK( iter != open_.end() );
  return *iter->second;
}

wait<maybe<e_menu_item>> MenuThreads::open_menu(
    e_menu const menu, rect const logical_screen_rect,
    MenuAllowedPositions const positions ) {
  int const menu_id = next_menu_id();
  SCOPE_EXIT { unregister_menu( menu_id ); };

  auto const render_layout = build_menu_rendered_layout(
      textometer_, menu, logical_screen_rect, positions );
  OpenMenu& om =
      *open_
           .emplace( piecewise_construct,
                     forward_as_tuple( menu_id ),
                     forward_as_tuple( render_layout ) )
           .first->second;
  om.menu      = menu;
  om.positions = positions;

  wait<> const translater =
      translate_routed_input_thread( menu_id );
  using SubMenuResult = e_menu_item;
  using SubMenuStream = co::stream<SubMenuResult>;
  maybe<wait<>> sub_menu_thread;
  auto sub_menu_opener =
      [&]( SubMenuStream& stream, e_menu const menu,
           MenuAllowedPositions const positions ) -> wait<> {
    auto const item = co_await open_menu(
        menu, logical_screen_rect, positions );
    if( item.has_value() ) stream.send( *item );
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
        co_return o;
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
      CASE( close ) { co_return nothing; }
      CASE( deselect ) {
        // Note that although we deselect, we don't close
        // sub-menus here. This is because it might make it
        // harder for the user to move the mouse over into the
        // sub-menu if we close it here, which would happen if
        // their mouse accidentally touched an adjacent disabled
        // item while trying to move the mouse into the sub-menu.
        om.anim_state.highlighted = nothing;
        break;
      }
      CASE( hover ) {
        om.hover_timer.reset();
        auto const layout = om.layout_for_selected();
        if( !layout.has_value() ) break;
        if( layout->has_arrow )
          om.events.send( MenuEvent::click{} );
        else
          // It seems to produce a good user experience to close
          // any submenus if the selection lingers on a leaf.
          sub_menu_thread.reset();
        break;
      }
      CASE( highlight ) {
        // This event, unlike the `over` event, is sent either in
        // response to keyboard action or some other manual ac-
        // tion, so we don't want the hover timer here.
        om.hover_timer.reset();
        om.anim_state.highlighted = highlight.text;
        break;
      }
      CASE( over ) {
        if( om.anim_state.highlighted != over.text )
          om.set_hover_timer();
        om.anim_state.highlighted = over.text;
        break;
      }
      CASE( click ) {
        om.hover_timer.reset();
        auto const layout = om.layout_for_selected();
        // This is normally expected to have a value if this
        // click is being made via the mouse, since the mouse
        // needs to move over a region before clicking on it. But
        // we'll be defensive just in case either a) someone man-
        // ually sends us this click event, or if there is some
        // strange ordering of input events.
        if( !layout.has_value() ) break;
        UNWRAP_CHECK_T( auto const& elem,
                        om.find_element( layout->text ) );
        SWITCH( elem ) {
          CASE( leaf ) {
            // Close sub menus first before animating this
            // click because we clicked an item in the main
            // menu.
            sub_menu_thread.reset();
            co_await animate_click( om.anim_state,
                                    layout->text );
            co_return leaf.item;
          }
          CASE( node ) {
            sub_menu_thread = sub_menu_opener(
                sub_menu_stream, node.menu,
                om.prioritized_submenu_positions( *layout ) );
            break;
          }
        }
      }
    }
  }
}

} // namespace rn
