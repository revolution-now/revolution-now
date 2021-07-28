/****************************************************************
**post-process.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-16.
*
* Description: Processes a parsed (valid) Rds object.
*
*****************************************************************/
#pragma once

// rdsc
#include "expr.hpp"

namespace rds {

// Takes in a parsed and valid Rds object and performs some
// transformations on it. This may include setting default
// values, optimizations, etc.
void post_process( expr::Rds& rds );

} // namespace rds