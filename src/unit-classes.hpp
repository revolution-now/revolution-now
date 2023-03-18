/****************************************************************
**unit-classes.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-18.
*
# Description: Types of units within a given unit class.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// config
#include "config/unit-classes.rds.hpp" // not a real config.

// ss
#include "ss/unit-type.hpp"

namespace rn {

maybe<e_scout_type> scout_type( e_unit_type type );

maybe<e_pioneer_type> pioneer_type( e_unit_type type );

maybe<e_missionary_type> missionary_type( UnitType type );

} // namespace rn
