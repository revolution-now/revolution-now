/****************************************************************
**validate.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-13.
*
* Description: Validation of an RDS AST.
*
*****************************************************************/
#pragma once

// rdsc
#include "expr.hpp"

// C++ standard library
#include <string>
#include <vector>

namespace rds {

// If the resuling vector is empty, then all is good. Otherwise,
// each string in the vector contains an error message.
std::vector<std::string> validate( expr::Rds const& rds );

} // namespace rds