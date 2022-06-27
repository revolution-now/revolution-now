/****************************************************************
**unit-type.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-24.
*
* Description: Config info for unit types.
*
*****************************************************************/
#pragma once

// Rds
#include "config/unit-type.rds.hpp"

// gs
#include "ss/unit-type.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

UnitTypeAttributes const& unit_attr( e_unit_type type );

UnitTypeAttributes const& unit_attr( UnitType type );

base::maybe<e_unit_type_modifier> inventory_to_modifier(
    e_unit_inventory inv );

} // namespace rn

namespace lua {
LUA_USERDATA_TRAITS( ::rn::UnitTypeAttributes, owned_by_cpp ){};
}
