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
#include "aliases.hpp"
#include "errors.hpp"

// C++ standard library
#include <random>

using namespace std;

namespace rn {

namespace rng {

namespace {

bernoulli_distribution coin( 0.5 );

// Will return nullopt until the engine has been seeded. This is
// to prevent anyone from using the engine before it has been
// seeded.
auto& maybe_engine() {
  static Opt<default_random_engine> engine;
  return engine;
}

auto& engine() {
  auto& e = maybe_engine();
  // Check that the engine has been seeded.
  CHECK( e.has_value(),
         "the random number engine must be seeded"
         " first with init(...)" );
  return *e;
}

} // namespace

void init( Opt<uint32_t> maybe_seed ) {
  auto seed =
      maybe_seed.value_or( uint32_t( random_device{}() ) );
  maybe_engine() = default_random_engine( seed );
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
    case e_interval::closed: CHECK( lower <= upper ); break;
    case e_interval::half_open:
      CHECK( lower < upper );
      real_upper--;
      break;
  };
  uniform_int_distribution<int> uniform_dist( lower,
                                              real_upper );
  return uniform_dist( engine() );
}

} // namespace rng

} // namespace rn
