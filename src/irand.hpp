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

  enum class e_interval { half_open, closed };

  // Random integer between tbe bounds, where the meaning of "be-
  // tween" depends on the interval type. If interval is half
  // open then lower must be < upper.
  virtual int between_ints( int lower, int upper,
                            e_interval type ) = 0;

  // Random floating point number in [lower, upper).
  virtual double between_doubles( double lower,
                                  double upper ) = 0;

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

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::IRand, owned_by_cpp ){};

}
