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
//
// We are implementing our own here because the standard does not
// guarantee the algorithm used, and we want our random number
// generation to be deterministic which requires not only the un-
// derlying pseudo random number generator must be consistent and
// deterministic, but also the the distribution algorithms used.
//
// WARNING: only call this with a proper random number generator
// otherwise it could go into an infinite loop.
template<class Rng>
[[nodiscard]] uint32_t uniform_u32_below( Rng& rng,
                                          uint32_t const s ) {
  static_assert( is_unsigned_v<decltype( rng() )> );
  static_assert( sizeof( decltype( rng() ) ) >=
                 sizeof( uint32_t ) );

  // We want threshold to be equal to (2^32 mod s). This can be
  // computed as (-s) % s using the uint32 wrap-around.
  CHECK( s != 0 );
  uint32_t const threshold = (uint32_t)( -s ) % s;

  for( ;; ) {
    uint32_t const x = (uint32_t)rng(); // uniform in [0, 2^32).
    uint64_t const m = (uint64_t)x * s; // full 64-bit product.
    uint32_t const l = (uint32_t)m;     // low 32 bits.
    if( l >= threshold )
      return (uint32_t)( m >> 32 ); // high 32 bits in [0, s).
  }
}

} // namespace

/****************************************************************
** random
*****************************************************************/
random::result_type random::raw() { return engine_(); }

void random::reseed( entropy const& seed_unmixed ) {
  engine_ = engine_t( seed_unmixed.mixed().consume<uint64_t>() );
}

bool random::bernoulli( double const p ) {
  CHECK_GE( p, 0 );
  CHECK_LE( p, 1.0 );
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
  return uniform_real_distribution<double>( lower,
                                            upper )( engine_ );
}

} // namespace rng
