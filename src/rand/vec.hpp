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

// base
#include "base/to-str.hpp"

// C++ standard library
#include <compare>

namespace rng {

// This in theory could be put (or taken from) elsewhere, but the
// idea is that we want to avoid introducing more dependencies in
// the rand library, so we'll provide one here.
struct vec2 {
  double x = {};
  double y = {};

  [[nodiscard]] friend vec2 operator*( vec2 const l,
                                       double const r ) {
    return vec2{ .x = l.x * r, .y = l.y * r };
  }

  [[nodiscard]] friend vec2 operator/( vec2 const l,
                                       double const r ) {
    return vec2{ .x = l.x / r, .y = l.y / r };
  }

  friend void to_str( vec2 const& o, std::string& out,
                      base::tag<vec2> ) {
    out += "vec2{x=";
    base::to_str( o.x, out );
    out += ",y=";
    base::to_str( o.y, out );
    out += '}';
  }
};

} // namespace rng
