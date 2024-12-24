/****************************************************************
**menu-body.hpp
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
#include "menu-body.rds.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "imenu-server.rds.hpp"
#include "wait.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/heap-value.hpp"
#include "base/range-lite.hpp"

// C++ standard library
#include <map>

namespace rn {

struct IMenuServer;
struct MenuAnimState;
struct MenuRenderLayout;
struct MenuItemRenderLayout;

enum class e_menu_item;

/****************************************************************
** MenuThreads
*****************************************************************/
struct MenuThreads {
  MenuThreads( IMenuServer const& menu_server );
  ~MenuThreads();

  int open_count() const;

  void send_event( MenuEventRaw const& event );

  wait<maybe<e_menu_item>> open_menu(
      e_menu menu, MenuAllowedPositions const positions );

  MenuAnimState const& anim_state( int menu_id ) const;
  MenuRenderLayout const& render_layout( int menu_id ) const;

  // Returns a view on the keys of the open_ map, i.e a range of
  // menu IDs.
  auto open_menu_ids() const {
    return base::rl::all( open_ ).keys();
  }

  bool enabled( e_menu_item item ) const;
  bool enabled( MenuItemRenderLayout const& item ) const;

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

  // This really should be const because otherwise it'd be pos-
  // sible to circumvent constness in this class, since we could
  // call a non-const method on the menu server (which owns this
  // MenuThreads object) which could then call a non-const method
  // on us.
  IMenuServer const& menu_server_;
  int next_menu_id_ = 1;
  // We need to support multiple menus running at a time in order
  // to support sub-menus, which will each have their own Open-
  // Menu state.
  std::map<int, base::heap_value<OpenMenu>> open_;
};

} // namespace rn
