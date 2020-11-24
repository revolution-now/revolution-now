/****************************************************************
**menu.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Menu bar
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "typed-int.hpp"

// base-util
#include "base-util/macros.hpp"

// C++ standard library
#include <functional>
#include <string>

namespace rn {

enum class e_menu {
  game,
  view,
  orders,
  colony,
  europort,
  advisors,
  music,
  window,
  debug,
  pedia
};

enum class e_menu_item {
  about,
  revolution,
  retire,
  exit,
  zoom_in,
  zoom_out,
  restore_zoom,
  music_play,
  music_stop,
  music_pause,
  music_resume,
  music_next,
  music_prev,
  music_vol_up,
  music_vol_down,
  music_set_player,
  toggle_fullscreen,
  restore_window,
  scale_up,
  scale_down,
  scale_optimal,
  sentry,
  fortify,
  military_advisor,
  economics_advisor,
  european_advisor,
  units_help,
  terrain_help,
  founding_father_help,
  toggle_console,
  europort_view,
  europort_close,
  colony_view_close
};

#define MENU_ITEM_HANDLER( item, handler_func,              \
                           is_enabled_func )                \
  STARTUP() {                                               \
    register_menu_item_handler(                             \
        e_menu_item::item, handler_func, is_enabled_func ); \
  }

#define MENU_HANDLERS( menu, is_enabled_func ) \
  STARTUP() {}

void register_menu_item_handler(
    e_menu_item                        item,
    std::function<void( void )> const& on_click,
    std::function<bool( void )> const& is_enabled );

H menu_height();

struct Plane;
Plane* menu_plane();

} // namespace rn
