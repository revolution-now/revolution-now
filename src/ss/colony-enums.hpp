/****************************************************************
**colony-enums.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-13.
*
* Description: Colony-related enums.
*
*****************************************************************/
#pragma once

// Rds
#include "colony-enums.rds.hpp"

namespace rn {

e_outdoor_job to_outdoor_job(
    e_outdoor_commons_secondary_job job );

e_colony_building to_colony_building(
    e_colony_barricade_type barricade );

} // namespace rn
