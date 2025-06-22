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
  // Check that rebel sentiment is in the right range.
  REFL_VALIDATE( rebel_sentiment >= 0 && rebel_sentiment <= 100,
                 "rebel sentiment must be in the range 0-100, "
                 "but is equal to {}.",
                 rebel_sentiment );

  // Check that, if we haven't declared, that the
  // post-declaration event flags are not set.
  if( status < e_revolution_status::declared ) {
    REFL_VALIDATE( !continental_army_mobilized,
                   "the continental_army_mobilized flag cannot "
                   "be set prior to the declaration." );
    REFL_VALIDATE( !gave_independence_war_hints,
                   "the gave_independence_war_hints flag cannot "
                   "be set prior to the declaration." );
    REFL_VALIDATE( !intervention_force_deployed,
                   "the intervention_force_deployed flag cannot "
                   "be set prior to the declaration." );
  }

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

  u["regular"]   = &U::regular;
  u["cavalry"]   = &U::cavalry;
  u["artillery"] = &U::artillery;
  u["man_o_war"] = &U::man_o_war;
};

// RevolutionState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::RevolutionState;
  auto u  = st.usertype.create<U>();

  u["rebel_sentiment"] = &U::rebel_sentiment;
  u["status"]          = &U::status;
  u["continental_army_mobilized"] =
      &U::continental_army_mobilized;
  u["gave_independence_war_hints"] =
      &U::gave_independence_war_hints;
  u["intervention_force_deployed"] =
      &U::intervention_force_deployed;
  u["ref_will_forfeit"]    = &U::ref_will_forfeit;
  u["expeditionary_force"] = &U::expeditionary_force;
};

} // namespace
