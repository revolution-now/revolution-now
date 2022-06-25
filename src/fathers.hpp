/****************************************************************
**fathers.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-26.
*
* Description: Api for querying properties of founding fathers.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// gs
#include "gs/fathers.rds.hpp"

// C++ standard library
#include <string_view>
#include <vector>

namespace rn {

/****************************************************************
** e_founding_father
*****************************************************************/
std::string_view founding_father_name(
    e_founding_father father );

/****************************************************************
** e_founding_father_type
*****************************************************************/
e_founding_father_type founding_father_type(
    e_founding_father father );

std::vector<e_founding_father> founding_fathers_for_type(
    e_founding_father_type type );

std::string_view founding_father_type_name(
    e_founding_father_type type );

void linker_dont_discard_module_fathers();

} // namespace rn
