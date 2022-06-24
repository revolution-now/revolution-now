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

// Revolution Now
#include "lua.hpp"

// gs
#include "gs/unit-type.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

base::valid_or<string> MarketItem::validate() const {
  // Keep in mind that this sell price is hundreds.
  REFL_VALIDATE( sell_price_in_hundreds >= 0,
                 "the sell price of a commodity must be >= 1" );
  REFL_VALIDATE( sell_price_in_hundreds < 100,
                 "the sell price of a commodity must be < 100" );

  REFL_VALIDATE(
      fabs( price_movement ) <= 1.0,
      "the price_movement field must be in [-1.0, 1.0]." );

  return base::valid;
}

base::valid_or<string> ExpeditionaryForce::validate() const {
  REFL_VALIDATE( regulars >= 0,
                 "the number of regulars in the expeditionary "
                 "force must be >= 0." );
  REFL_VALIDATE( cavalry >= 0,
                 "the number of cavalry in the expeditionary "
                 "force must be >= 0." );
  REFL_VALIDATE( artillery >= 0,
                 "the number of artillery in the expeditionary "
                 "force must be >= 0." );
  REFL_VALIDATE( men_of_war >= 0,
                 "the number of men_of_war in the expeditionary "
                 "force must be >= 0." );
  return base::valid;
}

base::valid_or<string> ImmigrationState::validate() const {
  // Validate that next_recruit_cost_base is in the right range.
  REFL_VALIDATE(
      next_recruit_cost_base >= 0,
      "next_recruit_cost_base must be >= 0, instead found {}.",
      next_recruit_cost_base );

  // Validate that all immigrants in the pool are human.
  for( e_unit_type type : immigrants_pool ) {
    REFL_VALIDATE( is_unit_human( UnitType::create( type ) ),
                   "units in the immigrant pool must be human, "
                   "but {} is not.",
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

// MarketItem
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::MarketItem;
  auto u  = st.usertype.create<U>();

  u["sell_price_in_hundreds"] = &U::sell_price_in_hundreds;
  u["boycott"]                = &U::boycott;
  u["price_movement"]         = &U::price_movement;
};

// MarketState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::MarketState;
  auto u  = st.usertype.create<U>();

  u["commodities"] = &U::commodities;
};

// MarketStateCommoditiesEnumMap
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::MarketStateCommoditiesEnumMap;
  auto u  = st.usertype.create<U>();

  u[lua::metatable_key]["__index"] =
      []( U& obj, e_commodity comm ) -> decltype( auto ) {
    return obj[comm];
  };

  // !! NOTE: because we overwrote the __index metamethod on this
  // userdata we cannot add any further (non-metatable) members
  // on this object, since there will be no way to look them up
  // by name.
};

// ExpeditionaryForce
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::ExpeditionaryForce;
  auto u  = st.usertype.create<U>();

  u["regulars"]   = &U::regulars;
  u["cavalry"]    = &U::cavalry;
  u["artillery"]  = &U::artillery;
  u["men_of_war"] = &U::men_of_war;
};

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

  u["immigrants_pool"]        = &U::immigrants_pool;
  u["next_recruit_cost_base"] = &U::next_recruit_cost_base;
};

// TaxationState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::TaxationState;
  auto u  = st.usertype.create<U>();

  u["tax_rate"] = &U::tax_rate;
};

// OldWorldState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::OldWorldState;
  auto u  = st.usertype.create<U>();

  u["harbor_state"]        = &U::harbor_state;
  u["immigration"]         = &U::immigration;
  u["taxes"]               = &U::taxes;
  u["market"]              = &U::market;
  u["expeditionary_force"] = &U::expeditionary_force;
};

} // namespace

} // namespace rn
