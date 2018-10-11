/****************************************************************
**nation.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-09-03.
*
* Description: Representation of nations.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

enum class ND e_nation { dutch, french, english, spanish };

ND e_nation player_nationality();

} // namespace rn
