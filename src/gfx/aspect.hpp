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

// C++ standard library
#include <numeric>
#include <span>

namespace gfx {

/****************************************************************
** Aspect Ratios.
*****************************************************************/
// Represents a monitor aspect ratio. Keeps its contents as a re-
// duced fraction.
struct AspectRatio {
  constexpr AspectRatio() = default;

  constexpr AspectRatio( size const ratio ) {
    if( ratio.h == 0 ) {
      if( ratio.w == 0 )
        ratio_ = size{ .w = 0, .h = 0 };
      else
        ratio_ = size{ .w = 1, .h = 0 };
      return;
    }
    auto const common = std::gcd( ratio.w, ratio.h );
    size const reduced{ .w = ratio.w / common,
                        .h = ratio.h / common };
    ratio_ = reduced;
  }

  constexpr size get() const { return ratio_; }

  constexpr base::maybe<double> scalar() const {
    if( ratio_.h <= 0 ) return base::nothing;
    return ratio_.w / double( ratio_.h );
  }

  friend void to_str( AspectRatio const& o, std::string& out,
                      base::tag<AspectRatio> );

  constexpr bool operator==( AspectRatio const& ) const =
      default;

 private:
  size ratio_ = {};
};

/****************************************************************
** Public API.
*****************************************************************/
AspectRatio named_aspect_ratio( e_named_aspect_ratio ratio );

base::maybe<e_named_aspect_ratio>
find_closest_named_aspect_ratio( AspectRatio target );

std::string named_ratio_canonical_name( e_named_aspect_ratio r );

} // namespace gfx
