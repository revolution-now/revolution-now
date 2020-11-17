/****************************************************************
**post-process.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-16.
*
* Description: Processes a parsed (valid) Rnl object.
*
*****************************************************************/
#pragma once

// rnlc
#include "expr.hpp"

namespace rnl {

// Takes in a parsed and valid Rnl object and performs some
// transformations on it. This may include setting default
// values, optimizations, etc.
void post_process( expr::Rnl& rnl );

} // namespace rnl