/****************************************************************
**sons-of-liberty.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-19.
*
# Description: Representation for state relating to SoL.
*
*****************************************************************/
#include "sons-of-liberty.hpp"

// luapp
#include "luapp/register.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** SonsOfLiberty
*****************************************************************/
base::valid_or<string> SonsOfLiberty::validate() const {
  REFL_VALIDATE(
      num_rebels_from_bells_only >= 0.0,
      "fractional rebel count in colony must be >= 0." );

  REFL_VALIDATE( last_sons_of_liberty_integral_percent >= 0,
                 "last_sons_of_liberty_integral_percent in "
                 "colony must be >= 0" );
  REFL_VALIDATE( last_sons_of_liberty_integral_percent <= 100,
                 "last_sons_of_liberty_integral_percent in "
                 "colony must be >= 1.0" );
  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::SonsOfLiberty;
  auto u  = st.usertype.create<U>();

  u["num_rebels_from_bells_only"] =
      &U::num_rebels_from_bells_only;
  u["last_sons_of_liberty_integral_percent"] =
      &U::last_sons_of_liberty_integral_percent;
};

} // namespace

} // namespace rn
