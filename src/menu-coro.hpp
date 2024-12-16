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
#include "base/function-ref.hpp"
#include "base/heap-value.hpp"

// C++ standard library
#include <map>

namespace rn {

struct MenuRenderState;

enum class e_menu_item;

/****************************************************************
** MenuThreads
*****************************************************************/
struct MenuThreads {
  MenuThreads();
  ~MenuThreads();

  int open_count() const;

  bool route_raw_input_thread( MenuEventRaw const& /*event*/ );

  wait<maybe<e_menu_item>> open_menu(
      MenuLayout const layout, MenuPosition const& position );

  void on_all_render_states(
      base::function_ref<void( MenuRenderState const& )> fn )
      const;

 private:
  int next_menu_id();

  void unregister_menu( int const menu_id );

  wait<> translate_routed_input_thread();

  static wait<> animate_click(
      MenuRenderState& /*render_state*/ );

  struct RoutedMenuEventRaw {
    int menu_id = {};
    MenuEventRaw input;
  };

  struct MenuState;

  int next_menu_id_ = 1;
  co::stream<RoutedMenuEventRaw> routed_input_;
  std::map<int, base::heap_value<MenuState>> open_;
};

} // namespace rn
