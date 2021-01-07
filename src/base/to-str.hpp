/****************************************************************
**to-str.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-07.
*
* Description: to_str framework.
*
*****************************************************************/
#pragma once

// C++ standard library
#include <string>

namespace base {

void to_str( int const& o, std::string& out );
void to_str( std::string const& o, std::string& out );

} // namespace base