/****************************************************************
**console.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-02-03.
*
* Description: The developer/mod console.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// C++ standard library
#include <string>

namespace rn {

void log_to_debug_console( std::string const& msg );
void log_to_debug_console( std::string&& msg );

struct Plane;
Plane* console_plane();

} // namespace rn
