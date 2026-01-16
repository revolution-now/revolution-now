/****************************************************************
**random.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Basic randomness facilities.
*
*****************************************************************/
#include "random.hpp"

// rand
#include "entropy.hpp"

namespace rng {

namespace {

using namespace std;

// Lemire's algorithm for fast unbiased uniform ints in [0, s).
// The rng() must produce uniform uint32_t values.
// https://arxiv.org/pdf/1805.10941
//
// We are implementing our own here because the standard does not
// guarantee the algorithm used, and we want our random number
// generation to be consistent across platforms. This requires
// not only that the underlying pseudo random number generator be
// consistent and deterministic, but also the the distribution
// algorithms used.
//
// WARNING: only call this with a proper random number generator
// otherwise it could go into an infinite loop.
template<class Rng>
[[nodiscard]] uint32_t uniform_u32_below( Rng& rng,
                                          uint32_t const s ) {
  static_assert( is_unsigned_v<decltype( rng() )> );
  static_assert( sizeof( decltype( rng() ) ) >=
                 sizeof( uint32_t ) );
  CHECK_GT( s, 0u );

  uint32_t x = {};
  uint64_t m = {};
  uint32_t l = {};

  auto const one_round = [&] {
    x = rng();
    m = uint64_t( x ) * uint64_t( s );
    l = uint32_t( m ); // low part.
  };

  one_round();
  if( l < s ) [[unlikely]] {
    // (2^32 mod s) == ((2^32-s) mod s) == ((-s) mod s) by using
    // the uint32 wrap-around.
    uint32_t const t = -s % s;
    while( l < t ) one_round(); // rejection loop.
  }
  return m >> 32;
}

} // namespace

/****************************************************************
** random
*****************************************************************/
random::result_type random::raw() { return engine_(); }

random::random( entropy const& seed ) { reseed( seed ); }

void random::reseed( entropy const& seed_unmixed ) {
  // NOTE: this should not be changed otherwise it will change
  // the meaning of a given seed.
  engine_ = engine_t( seed_unmixed.mixed().consume<uint64_t>() );
}

entropy random::generate_deterministic_seed() {
  return entropy{
    .e1 = uniform<uint32_t>(),
    .e2 = uniform<uint32_t>(),
    .e3 = uniform<uint32_t>(),
    .e4 = uniform<uint32_t>(),
  };
}

bool random::bernoulli( double const p ) {
  CHECK_GE( p, 0 );
  CHECK_LE( p, 1.0 );
  // TODO: make this deterministic.
  return bernoulli_distribution( p )( engine_ );
}

int random::uniform_int( int const lower, int const upper ) {
  CHECK_LE( lower, upper );
  if( upper == lower ) return lower;
  auto const rand = [&] -> uint32_t {
    auto const delta =
        uint32_t( int64_t( upper ) - int64_t( lower ) );
    static constexpr auto kMax = numeric_limits<uint32_t>::max();
    if( delta == kMax ) return uniform<uint32_t>();
    return uniform_u32_below( engine_, delta + 1 );
  }();
  return int( int64_t( lower ) + int64_t( rand ) );
}

double random::uniform_double( double const lower,
                               double const upper ) {
  CHECK_LE( lower, upper );
  // TODO: make this deterministic.
  return uniform_real_distribution<double>( lower,
                                            upper )( engine_ );
}

} // namespace rng
