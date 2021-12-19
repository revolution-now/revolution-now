/****************************************************************
**parse.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Parser for the rcl config language.
*
*****************************************************************/
#pragma once

// rcl
#include "model.hpp"

// base
#include "base/expect.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace rcl {

// Rcl parser.
//
// WARNING: this function is NOT thread safe as it uses global
//          state for speed and simplicity.
//
base::expect<doc> parse( std::string_view filename,
                         std::string_view in );

// For convenience.
//
// WARNING: this function is NOT thread safe as it uses global
//          state for speed and simplicity.
//
base::expect<doc> parse_file( std::string_view filename );

} // namespace rcl
