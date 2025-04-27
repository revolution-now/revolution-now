/****************************************************************
**old-world-state.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-27.
*
# Description: Per-player old world state.
*
*****************************************************************/
#include "old-world-state.hpp"

// gs
#include "ss/market.hpp"
#include "ss/unit-type.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

base::valid_or<string> ImmigrationState::validate() const {
  // Validate that all immigrants in the pool are colonists.
  for( e_unit_type type : immigrants_pool ) {
    REFL_VALIDATE( is_unit_a_colonist( type ),
                   "units in the immigrant pool must be a "
                   "colonist, but {} is not.",
                   type );
  }

  return base::valid;
}

base::valid_or<string> TaxationState::validate() const {
  REFL_VALIDATE( tax_rate >= 0, "The tax rate must be >= 0." );
  REFL_VALIDATE( tax_rate <= 100,
                 "The tax rate must be <= 100." );

  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// HarborState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::HarborState;
  auto u  = st.usertype.create<U>();

  u["selected_unit"] = &U::selected_unit;
};

// ImmigrantsPoolArray
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::ImmigrantsPoolArray;
  auto u  = st.usertype.create<U>();

  u[lua::metatable_key]["__index"] = [&]( U& obj, int idx ) {
    LUA_CHECK( st, idx >= 1 && idx <= 3,
               "immigrant pool index must be either 1, 2, 3." );
    return obj[idx - 1];
  };

  u[lua::metatable_key]["__newindex"] = [&]( U& obj, int idx,
                                             e_unit_type type ) {
    LUA_CHECK( st, idx >= 1 && idx <= 3,
               "immigrant pool index must be either 1, 2, 3." );
    obj[idx - 1] = type;
  };

  // !! NOTE: because we overwrote the __index metamethod on this
  // userdata we cannot add any further (non-metatable) members
  // on this object, since there will be no way to look them up
  // by name.
};

// ImmigrationState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::ImmigrationState;
  auto u  = st.usertype.create<U>();

  u["immigrants_pool"]     = &U::immigrants_pool;
  u["num_recruits_rushed"] = &U::num_recruits_rushed;
};

// TaxationState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::TaxationState;
  auto u  = st.usertype.create<U>();

  u["tax_rate"]              = &U::tax_rate;
  u["next_tax_event_turn"]   = &U::next_tax_event_turn;
  u["king_remarriage_count"] = &U::king_remarriage_count;
};

// OldWorldState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::OldWorldState;
  auto u  = st.usertype.create<U>();

  u["harbor_state"] = &U::harbor_state;
  u["immigration"]  = &U::immigration;
  u["taxes"]        = &U::taxes;
  u["market"]       = &U::market;
};

} // namespace

} // namespace rn
