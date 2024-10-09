/****************************************************************
**aspect.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-09.
*
* Description: Handles screen aspect ratio and resolution selec-
*              tion.
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
#include <span>

namespace gfx {

/****************************************************************
** AspectRatio
*****************************************************************/
// Represents a monitor aspect ratio. Keeps its contents as a re-
// duced fraction that is always non-zero and with a denominator
// that is larger than zero.
struct AspectRatio {
  static base::maybe<AspectRatio> from_size( size resolution );

  static AspectRatio from_named( e_named_aspect_ratio ratio );

  // AspectRatios for all named ratios.
  static std::span<AspectRatio const> named_all();

  size get() const { return ratio_; }

  double scalar() const;

  friend void to_str( AspectRatio const& o, std::string& out,
                      base::ADL_t );

  bool operator==( AspectRatio const& ) const = default;

 private:
  AspectRatio( size resolution );
  size ratio_;
};

/****************************************************************
** Public API.
*****************************************************************/
base::maybe<AspectRatio> find_closest_aspect_ratio(
    std::span<AspectRatio const> ratios_all,
    AspectRatio target_ratio, double tolerance );

// Returns the default tolerance used for bucketing of aspect ra-
// tios. This is a small positive number < 1.
double default_ratio_tolerance();

} // namespace gfx
