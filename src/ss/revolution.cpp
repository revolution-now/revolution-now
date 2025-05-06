/****************************************************************
**revolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-06.
*
* Description: Holds state related to the war of independence.
*
*****************************************************************/
#include "revolution.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** RevolutionState
*****************************************************************/
valid_or<string> RevolutionState::validate() const {
  REFL_VALIDATE( rebel_sentiment >= 0 && rebel_sentiment <= 100,
                 "rebel sentiment must be in the range 0-100, "
                 "but is equal to {}.",
                 rebel_sentiment );

  return valid;
}

} // namespace rn

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// ExpeditionaryForce
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::ExpeditionaryForce;
  auto u  = st.usertype.create<U>();

  u["regulars"]  = &U::regulars;
  u["cavalry"]   = &U::cavalry;
  u["artillery"] = &U::artillery;
  u["men_o_war"] = &U::men_o_war;
};

// RevolutionState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::RevolutionState;
  auto u  = st.usertype.create<U>();

  u["rebel_sentiment"] = &U::rebel_sentiment;
  u["status"]          = &U::status;
  u["armies_promoted"] = &U::armies_promoted;
  u["intervention_force_deployed"] =
      &U::intervention_force_deployed;
  u["ref_will_forfeit"]    = &U::ref_will_forfeit;
  u["expeditionary_force"] = &U::expeditionary_force;
};

} // namespace
