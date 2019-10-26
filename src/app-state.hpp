/****************************************************************
**app-state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-25.
*
* Description: Handles the top-level game state machines.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
//#include "dummy.hpp"

namespace rn {

ND bool advance_app_state();

bool back_to_main_menu();

/****************************************************************
** Testing
*****************************************************************/
void test_app_state();

} // namespace rn
