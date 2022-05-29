/****************************************************************
**old-world-state.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-27.
*
# Description: Per-player old world state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "old-world-state.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

using MarketStateCommoditiesEnumMap =
    ::refl::enum_map<::rn::e_commodity, ::rn::MarketItem>;

using ImmigrantsPoolArray = std::array<e_unit_type, 3>;

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::MarketItem, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::MarketState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::ExpeditionaryForce, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::HarborState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::ImmigrationState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::TaxationState, owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::OldWorldState, owned_by_cpp ){};

LUA_USERDATA_TRAITS( ::rn::MarketStateCommoditiesEnumMap,
                     owned_by_cpp ){};
LUA_USERDATA_TRAITS( ::rn::ImmigrantsPoolArray, owned_by_cpp ){};

} // namespace lua
