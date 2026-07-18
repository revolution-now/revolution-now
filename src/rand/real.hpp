/****************************************************************
**real.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-07-13.
*
* Description: A platform-independent uniform double generator.
*
*****************************************************************/
#pragma once

#include <random>

namespace rng {

/****************************************************************
** portable_uniform_real_distribution<std::mt19937>
*****************************************************************/
// Getting random floating point numbers (especially within a
// specified range) in a way that is portable across environ-
// ments, OS's, CPUs, etc. is very tricky business. This is a
// best-effort attempt, though it should do the trick on most
// major platforms/environments.
struct portable_uniform_real_distribution {
  // Returns one of 2^53 equally spaced values in [0, 1).
  [[nodiscard]] double operator()( std::mt19937& gen ) const;

  // Returns a value in [min, max). Requires:
  //   * min < max.
  //   * min and max are finite.
  [[nodiscard]] double operator()( std::mt19937& gen,
                                   double const min,
                                   double const max ) const;
};

/****************************************************************
** Helpers.
*****************************************************************/
// This is needed to ensure we have the correct floating point
// environment which is necessary for reproducible floating point
// math (in particular, floating point random number generation).
// Some of these checks can only be checked at runtime.
void check_floating_point_portability();

} // namespace rng
