/****************************************************************
**validate.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-13.
*
* Description: Validation of an RNL AST.
*
*****************************************************************/
#pragma once

// rnlc
#include "expr.hpp"

// C++ standard library
#include <string>
#include <vector>

namespace rnl {

// If the resuling vector is empty, then all is good. Otherwise,
// each string in the vector contains an error message.
std::vector<std::string> validate( expr::Rnl const& rnl );

} // namespace rnl