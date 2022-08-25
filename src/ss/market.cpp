/****************************************************************
**market.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Represents state related to european market
*              prices.
*
*****************************************************************/
#include "market.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

/****************************************************************
** MarketItem
*****************************************************************/
base::valid_or<string> MarketItem::validate() const {
  REFL_VALIDATE( starting_ask_price_in_hundreds >= 1,
                 "starting_ask_price_in_hundreds must be >= 1" );
  REFL_VALIDATE(
      starting_ask_price_in_hundreds <= 20,
      "starting_ask_price_in_hundreds must be <= 20" );
  REFL_VALIDATE( current_ask_price_in_hundreds >= 1,
                 "current_ask_price_in_hundreds must be >= 1" );
  REFL_VALIDATE( current_ask_price_in_hundreds <= 20,
                 "current_ask_price_in_hundreds must be <= 20" );
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

  u["starting_ask_price_in_hundreds"] =
      &U::starting_ask_price_in_hundreds;
  u["net_supply_in_europe"] = &U::net_supply_in_europe;
  u["current_ask_price_in_hundreds"] =
      &U::current_ask_price_in_hundreds;
  u["boycott"] = &U::boycott;
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

} // namespace

} // namespace rn
