/****************************************************************
**rand.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-06.
*
* Description: Handles random numbers and all things random.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "error.hpp"
#include "irand.hpp"

// rand
#include "rand/random.hpp"

// C++ standard library
#include <random>

/****************************************************************
** Fwd. Decls.
*****************************************************************/
namespace rng {
struct entropy;
}

namespace rn {

/****************************************************************
** Rand
*****************************************************************/
// Generates real (pseudo-random) numbers. For unit testing in-
// stead use a mock of IRand, not this one.
struct Rand : IRand {
  // Will invoke std::random_device for a seed.
  Rand() = default;

  Rand( uint32_t seed );

  ~Rand() override = default;

  void reseed( rng::entropy const& seed );

  // Implement IRand.
  bool bernoulli( double p ) override;

  // Implement IRand.
  int between_ints( int lower, int upper ) override;

  // Implement IRand.
  double between_doubles( double lower, double upper ) override;

 private:
  rng::random rd_;
};

} // namespace rn
