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
#include "enum.hpp"

// base-util
#include "base-util/macros.hpp"

// C++ standard library
#include <functional>
#include <string>

namespace rn {

enum class e_( menu,
               game,   //
               view,   //
               orders, //
               pedia   //
);

enum class e_( menu_item,
               about,        //
               exit,         //
               zoom_in,      //
               zoom_out,     //
               restore_zoom, //
               sentry,       //
               fortify,      //
               units_help    //
);

#define MENU_ITEM_HANDLER( item, handler_func,              \
                           is_enabled_func )                \
  STARTUP() {                                               \
    register_menu_item_handler(                             \
        e_menu_item::item, handler_func, is_enabled_func ); \
  }

#define MENU_HANDLERS( menu, is_enabled_func ) \
  STARTUP() {}

// menu_state = menus_off
//           | menus_closed
//           | menu_open

// menu_animation_state = none
//                     | opening
//                     | switching
//                     | clicking
//                     | closing

void register_menu_item_handler(
    e_menu_item                        item,
    std::function<void( void )> const& on_click,
    std::function<bool( void )> const& is_enabled );

void initialize_menus();
void cleanup_menus();

struct Plane;
Plane* menu_plane();

} // namespace rn
