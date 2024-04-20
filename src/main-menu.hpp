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

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Planes;

/****************************************************************
** API
*****************************************************************/
wait<> run_main_menu( Planes& planes );

} // namespace rn
