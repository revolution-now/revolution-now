/****************************************************************
**rds-parser.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-21.
*
* Description: RDS Parser.
*
*****************************************************************/
#pragma once

// rdsc
#include "expr.hpp"

// C++ standard library
#include <string_view>

namespace rds {

expr::Rds parse( std::string_view filename,
                 std::string_view preamble_file );

}
