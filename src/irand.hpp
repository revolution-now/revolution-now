/****************************************************************
**irand.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-11.
*
* Description: Injectable interface for random number generation.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "irand.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

// base
#include "base/attributes.hpp"
#include "base/error.hpp"

// C++ standard library
#include <vector>

namespace rn {

/****************************************************************
** IRand
*****************************************************************/
// Anything in the game that needs to generate random behavior
// and that also needs to be unit tested should use this inter-
// face for all random number generation.
struct IRand {
  virtual ~IRand() = default;

  // Biased coin flip.  Returns true with probability p.
  virtual bool bernoulli( double p ) = 0;

  // Random integer between tbe bounds, where the meaning of "be-
  // tween" depends on the interval type. If interval is half
  // open then lower must be < upper.
  virtual int between_ints( int lower, int upper,
                            e_interval type ) = 0;

  // Random floating point number in [lower, upper).
  virtual double between_doubles( double lower,
                                  double upper ) = 0;

  // Shuffles the elements. Vector can be empty. Picks a random
  // element.
  template<typename T>
  void shuffle( std::vector<T>& vec );

  // For convenience. Vector must be non-empty. Picks a random
  // element.
  template<typename T>
  T const& pick_one(
      std::vector<T> const& v ATTR_LIFETIMEBOUND ) {
    CHECK( !v.empty() );
    return v[between_ints( 0, v.size(), e_interval::half_open )];
  }
};

void to_str( IRand const& o, std::string& out, base::ADL_t );

template<typename T>
void IRand::shuffle( std::vector<T>& vec ) {
  if( vec.empty() ) return;
  int const last_idx = vec.size() - 1;
  // i < last_idx because we don't want to consider swapping the
  // last element with itself, which would have not purpose.
  for( int i = 0; i < last_idx; ++i ) {
    int source = between_ints( i, last_idx, e_interval::closed );
    using std::swap;
    swap( vec[i], vec[source] );
  }
}

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::IRand, owned_by_cpp ){};

}
