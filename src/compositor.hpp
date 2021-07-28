/****************************************************************
**compositor.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-07-04.
*
* Description: Coordinates layout of elements on screen.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"

// Rds
#include "rds/compositor.hpp"

namespace rn::compositor {

// If the section is visible it will return bounds.
maybe<Rect> section( e_section section );

} // namespace rn::compositor
