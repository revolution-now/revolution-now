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
  // Will NOT randomly seed the underlying rng; will just default
  // construct it, thus you must follow this up with a call to
  // reseed if you want to seed it.
  Rand() = default;

  ~Rand() override = default;

  void reseed( rng::entropy const& seed ) override;

 public: // IRand.
  bool bernoulli( double p ) override;

  int uniform_int( int lower, int upper ) override;

  double uniform_double( double lower, double upper ) override;

  rng::seed generate_deterministic_seed() override;

 private:
  rng::random rd_;
};

} // namespace rn
