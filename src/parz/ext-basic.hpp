/****************************************************************
**ext-basic.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-30.
*
* Description: Parser extension point for basic types.
*
*****************************************************************/
#pragma once

// parz
#include "ext.hpp"

// C++ standard library
#include <string>

namespace parz {

parser<int> parser_for( tag<int> );

parser<std::string> parser_for( tag<std::string> );

} // namespace parz
