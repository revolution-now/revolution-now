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

// Rds
#include "rds/menu.hpp"

// base-util
#include "base-util/macros.hpp"

// C++ standard library
#include <functional>
#include <string>

namespace rn {

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
