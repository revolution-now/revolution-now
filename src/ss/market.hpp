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

using PlayerMarketStateCommoditiesEnumMap =
    ::refl::enum_map<::rn::e_commodity, ::rn::PlayerMarketItem>;
using GlobalMarketStateCommoditiesEnumMap =
    ::refl::enum_map<::rn::e_commodity, ::rn::GlobalMarketItem>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::PlayerMarketItem, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::GlobalMarketItem, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::PlayerMarketState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::GlobalMarketState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::PlayerMarketStateCommoditiesEnumMap,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::GlobalMarketStateCommoditiesEnumMap,
                     owned_by_cpp ){};

} // namespace lua
