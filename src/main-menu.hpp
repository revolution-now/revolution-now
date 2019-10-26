/****************************************************************
**main-menu.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Main application menu.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "enum.hpp"

namespace rn {

enum class e_( main_menu_type, //
               no_game,        //
               in_game         //
);

enum class e_( main_menu_item, //
               resume,         //
               new_,           //
               load,           //
               save,           //
               leave,          //
               quit            //
);

void set_main_menu( e_main_menu_type type );

// When this function returns a value, that value will be reset,
// so another call immediately after will yield no result.
Opt<e_main_menu_item> main_menu_selection();

struct Plane;
Plane* main_menu_plane();

// FIXME: temporary.
void show_main_menu_plane();
void hide_main_menu_plane();

/****************************************************************
** Testing
*****************************************************************/
void test_main_menu();

} // namespace rn
