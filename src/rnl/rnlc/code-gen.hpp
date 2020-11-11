/****************************************************************
**code-gen.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-11.
*
* Description: Code generator for the RNL language.
*
*****************************************************************/
#pragma once

// rnlc
#include "expr.hpp"

// c++ standard library
#include <optional>
#include <string>

namespace rnl {

std::optional<std::string> generate_code( expr::Rnl const& rnl );

} // namespace rnl
