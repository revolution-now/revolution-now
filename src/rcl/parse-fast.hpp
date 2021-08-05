/****************************************************************
**parse-fast.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-04.
*
* Description: Fast parser for the rcl config language.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rcl
#include "model.hpp"

namespace rcl {

// This is a parser for the rcl language that is essentially
// written in C, and as a result it is super fast.
//
// WARNING: this function is NOT thread safe as it uses global
//          state for speed and simplicity.
base::expect<rawdoc, std::string> parse_fast(
    std::string_view filename, std::string_view in );

} // namespace rcl
