/****************************************************************
**menu-coro.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-14.
*
* Description: Coroutines for running menus.
*
*****************************************************************/
#pragma once

// rds
#include "menu-coro.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "imenu-plane.rds.hpp"
#include "wait.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/heap-value.hpp"
#include "base/range-lite.hpp"

// C++ standard library
#include <map>

namespace rn {

struct MenuAnimState;
struct MenuRenderLayout;

enum class e_menu_item;

/****************************************************************
** MenuThreads
*****************************************************************/
struct MenuThreads {
  MenuThreads();
  ~MenuThreads();

  int open_count() const;

  void route_raw_input_thread( MenuEventRaw const& /*event*/ );

  wait<maybe<e_menu_item>> open_menu(
      MenuContents const contents, MenuPosition const position );

  MenuAnimState const& anim_state( int menu_id ) const;
  MenuRenderLayout const& render_layout( int menu_id ) const;

  // Returns a view on the keys of the open_ map, i.e a range of
  // menu IDs.
  auto open_menu_ids() const {
    return base::rl::all( open_ ).keys();
  }

 private:
  struct OpenMenu;

  int next_menu_id();

  void unregister_menu( int const menu_id );

  wait<> translate_routed_input_thread( int menu_id );

  void handle_key_event( OpenMenu& open_menu,
                         input::key_event_t const& key_event );

  maybe<int> menu_from_point( gfx::point p ) const;

  static wait<> animate_click( MenuAnimState& render_state,
                               std::string const& text );

  int next_menu_id_ = 1;
  std::map<int, base::heap_value<OpenMenu>> open_;
};

} // namespace rn
