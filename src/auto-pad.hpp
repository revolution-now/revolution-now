/****************************************************************
**auto-pad.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-04-05.
*
* Description: Auto merged padding around UI elements.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "views.hpp"

// C++ standard library
#include <vector>

namespace rn {

void autopad( UPtr<ui::View>& view );

void test_autopad();

} // namespace rn
