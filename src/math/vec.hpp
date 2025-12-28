/****************************************************************
**vec.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-27.
*
* Description: Simple vector type for mathematical ops.
*
*****************************************************************/
#pragma once

namespace math {

struct vec2 {
  double x = {};
  double y = {};

  friend vec2 operator*( vec2 const l, double const r ) {
    return vec2{ .x = l.x * r, .y = l.y * r };
  }

  friend vec2 operator/( vec2 const l, double const r ) {
    return vec2{ .x = l.x / r, .y = l.y / r };
  }
};

} // namespace math
