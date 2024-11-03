/****************************************************************
**aspect.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-09.
*
* Description: Helpers for dealing with aspect ratios.
*
*****************************************************************/
#pragma once

// rds
#include "aspect.rds.hpp"

// gfx
#include "cartesian.hpp"

// base
#include "base/maybe.hpp"

namespace gfx {

/****************************************************************
** e_named_aspect_ratio
*****************************************************************/
// Note that the result will not necessarily be in reduced frac-
// tion form.
size named_aspect_ratio( e_named_aspect_ratio ratio );

/****************************************************************
** Public API.
*****************************************************************/
// The tolerance is used for bucketing of aspect ratios. This is
// a small positive number < 1. For example, a sensible value
// might be 0.04.
base::maybe<e_named_aspect_ratio> find_close_named_aspect_ratio(
    size target, double tolerance );

} // namespace gfx
