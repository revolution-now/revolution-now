/****************************************************************
**string.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-28.
*
* Description: String manipulation utilities.
*
*****************************************************************/
#pragma once

#include <string>
#include <string_view>

namespace base {

// Trim leading and trailing whitespace.
std::string trim( std::string_view sv );

} // namespace base