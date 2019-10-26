/****************************************************************
**plane-ctrl.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-26.
*
* Description: Manages ordering and enablement of planes.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "enum.hpp"
#include "sg-macros.hpp"

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Plane );

// Some of these overlap. For example, `terrain_view` will show
// the viewport with the game world, which `play_game` may also
// do, but `play_game` might also show the europe view, depending
// on where the user left off before going to the main menu.
enum class e_( plane_config, //
               main_menu,    //
               play_game,    //
               terrain_view, //
               europe,       //
               open_console, //
               close_console //
);

// Will change the state (subset and order of planes chosen) for
// the given configuration. Note that the result of calling this
// function will in general depend on previous plane states.
void set_plane_config( e_plane_config conf );

/****************************************************************
** Testing
*****************************************************************/
void test_plane_ctrl();

} // namespace rn
