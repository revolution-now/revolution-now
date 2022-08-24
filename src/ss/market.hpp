/****************************************************************
**market.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-21.
*
* Description: Represents state related to european market
*              prices.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "ss/market.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using MarketStateCommoditiesEnumMap =
    ::refl::enum_map<::rn::e_commodity, ::rn::MarketItem>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::MarketItem, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::MarketState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::MarketStateCommoditiesEnumMap,
                     owned_by_cpp ){};

} // namespace lua
