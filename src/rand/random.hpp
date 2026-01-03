/****************************************************************
**random.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-24.
*
* Description: Basic randomness facilities.
*
*****************************************************************/
#pragma once

// base
#include "base/attributes.hpp"
#include "base/error.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <random>
#include <vector>

namespace rng {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct entropy;

/****************************************************************
** random
*****************************************************************/
struct random {
 public:
  // It seems the default engine (linear congruential) does not
  // produce high-enough quality results. Given that there has
  // always been so much talk about issues with the random number
  // generator in the OG, we definitely don't want to risk any
  // issues there in the NG, and this Mersenne Twister should
  // guarantee that.
  using engine_t = std::mt19937;

  static_assert( sizeof( engine_t::result_type ) >=
                 sizeof( uint32_t ) );
  using result_type = engine_t::result_type;

 public:
  random() = default;

  void reseed( entropy const& new_seed );

  // Just get a raw random value from the engine of the type it
  // is defined with.
  [[nodiscard]] result_type raw();

  // Biased coin flip.
  [[nodiscard]] bool bernoulli( double p );

  // Closed on both ends.
  [[nodiscard]] int uniform_int( int lower, int upper );

  // Closed on both ends.
  [[nodiscard]] double uniform_double( double lower,
                                       double upper );

  // Uniform over all values of the type.
  template<typename T>
  requires( std::is_integral_v<T> && std::is_unsigned_v<T> &&
            !std::is_same_v<T, bool> )
  [[nodiscard]] T uniform() {
    static_assert( sizeof( decltype( raw() ) ) >= sizeof( T ) );
    // This is effectively a modulus to get the desired uniform
    // distribution. It is ok in this case because the range of
    // desired values will always evenly divide into the total
    // possible values returned by raw(), since we're always
    // dealing with powers of two. Thus we don't need any rejec-
    // tion step.
    return raw() & std::numeric_limits<T>::max();
  }

  template<typename T>
  base::maybe<T const&> pick_one_safe(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    if( v.empty() ) return base::nothing;
    return v[uniform_int( 0, v.size() - 1 )];
  }

  template<typename T>
  [[nodiscard]] T const& pick_one(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    UNWRAP_CHECK( res, pick_one_safe( v ) );
    return res;
  }

 private:
  engine_t engine_;
};

} // namespace rng
