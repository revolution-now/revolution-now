/****************************************************************
**land-production.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Computes what is produced on a land square.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// gs
#include "ss/colony.rds.hpp"
#include "ss/unit-type.rds.hpp"

namespace rn {

struct TerrainState;

int production_on_square( e_outdoor_job       job,
                          TerrainState const& terrain_state,
                          e_unit_type unit_type, Coord where );

// Given a unit activity this will convert it to an outdoor job,
// if the mapping exists. Basically this can be used to take a
// colonist expertise and find the outdoor job corresponding to
// the job that it is skilled at.
maybe<e_outdoor_job> outdoor_job_for_expertise(
    e_unit_activity activity );

e_unit_activity activity_for_outdoor_job( e_outdoor_job job );

} // namespace rn
