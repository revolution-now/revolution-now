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

// base
#include "base/maybe.hpp"

// c++ standard library
#include <string>

namespace rnl {

base::maybe<std::string> generate_code( expr::Rnl const& rnl );

} // namespace rnl
