/****************************************************************
**parse.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser for config files.
*
*****************************************************************/
#pragma once

// cl
#include "model.hpp"

// C++ standard library
#include <string_view>

namespace cl {

// This will keep the string the same length but will ovewrite
// all comments (comment delimiters and comment contents) with
// spaces).
void blankify_comments( std::string& text );

doc parse_file( std::string_view filename );

} // namespace cl
