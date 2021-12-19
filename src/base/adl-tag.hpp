/****************************************************************
**adl-tag.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-18.
*
* Description: Tag to be used to help ADL find things in base/.
*
*****************************************************************/
#pragma once

#include "config.hpp"

namespace base {

struct ADL_t {};

inline constexpr ADL_t ADL{};

} // namespace base