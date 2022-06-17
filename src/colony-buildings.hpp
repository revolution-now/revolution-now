/****************************************************************
**colony-buildings.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: All things related to colony buildings.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// Rds
#include "colony-buildings.rds.hpp"
#include "colony.rds.hpp"
#include "config/colony-enums.rds.hpp"
#include "tile-enum.rds.hpp"
#include "utype.rds.hpp"

namespace rn {

struct Colony;

/****************************************************************
** Public API
*****************************************************************/
e_colony_building_slot slot_for_building(
    e_colony_building building );

maybe<e_indoor_job> indoor_job_for_slot(
    e_colony_building_slot slot );

e_colony_building_slot slot_for_indoor_job( e_indoor_job job );

// Returns all possible buildings in a slot, in descending order
// of calibur.
std::vector<e_colony_building> const& buildings_for_slot(
    e_colony_building_slot slot );

maybe<e_colony_building> building_for_slot(
    Colony const& colony, e_colony_building_slot slot );

// This will return true if the colony has the given building or
// if it has a higher level building in the same slot. This is
// the function that should always be used to test for that kind
// of thing, since it is not really specified whether e.g. a
// colony that has a shipyard needs to also have docks in its
// representation.
bool colony_has_building_level( Colony const&     colony,
                                e_colony_building building );

// Note that this does not apply to food.
int colony_warehouse_capacity( Colony const& colony );

e_unit_activity activity_for_indoor_job( e_indoor_job job );

} // namespace rn
