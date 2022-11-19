/****************************************************************
**native-enums.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-19.
*
* Description: Enums associated with the natives.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "ss/native-enums.rds.hpp"

// ss
#include "unit-type.rds.hpp"

namespace rn {

e_unit_activity activity_for_native_skill(
    e_native_skill skill );

base::maybe<e_native_skill> native_skill_for_activity(
    e_unit_activity activity );

} // namespace rn
