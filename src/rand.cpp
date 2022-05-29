/****************************************************************
**rand.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-06.
*
* Description: Handles random numbers and all things random.
*
*****************************************************************/
#include "rand.hpp"

// Revolution Now
#include "error.hpp"
#include "init.hpp"

// C++ standard library
#include <random>

using namespace std;

namespace rn {

namespace rng {

namespace {

bernoulli_distribution coin( 0.5 );

// Will return nothing until the engine has been seeded. This is
// to prevent anyone from using the engine before it has been
// seeded.
auto& maybe_engine() {
  static maybe<default_random_engine> engine;
  return engine;
}

} // namespace

default_random_engine& engine() {
  auto& e = maybe_engine();
  // Check that the engine has been seeded.
  CHECK( e.has_value(),
         "the random number engine must be seeded"
         " first with init(...)" );
  return *e;
}

bool flip_coin() { return coin( engine() ); }

bool flip_coin( double p ) {
  CHECK( 0 <= p && p <= 1.0 );
  bernoulli_distribution biased_coin( p );
  return biased_coin( engine() );
}

int between( int lower, int upper, e_interval type ) {
  auto real_upper = upper;
  switch( type ) {
    case e_interval::closed: //
      CHECK( lower <= upper );
      break;
    case e_interval::half_open: //
      CHECK( lower < upper );
      real_upper--;
      break;
  };
  uniform_int_distribution<int> uniform_dist( lower,
                                              real_upper );
  return uniform_dist( engine() );
}

double between( double lower, double upper ) {
  uniform_real_distribution<double> uniform_dist( lower, upper );
  return uniform_dist( engine() );
}

int random_int() {
  return between( numeric_limits<int>::min(),
                  numeric_limits<int>::max(),
                  e_interval::closed );
}

void reseed( uint32_t seed ) {
  CHECK( maybe_engine().has_value(),
         "engine has not been seeded for the first time." );
  rng::maybe_engine() = default_random_engine( seed );
}

} // namespace rng

namespace {

// If a seed is not provided then it will use one from std::ran-
// dom_device.
void init_rng() {
  // Change this to get a fixed seed.
  maybe<uint32_t> maybe_seed = nothing;
  auto            seed =
      maybe_seed.value_or( uint32_t( random_device{}() ) );
  rng::maybe_engine() = default_random_engine( seed );
}

void cleanup_rng() {}

} // namespace

//
//
//
REGISTER_INIT_ROUTINE( rng );

} // namespace rn
