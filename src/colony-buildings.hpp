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

// gs
#include "ss/colony-enums.rds.hpp"
#include "ss/unit-type.rds.hpp"

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

// If the building is a school / college / university then this
// will the relevant type.
maybe<e_school_type> school_type_from_building(
    e_colony_building building );

e_colony_building building_for_school_type(
    e_school_type school_type );

// Maximum number of colonists that can be dragged onto work in a
// building. In the original game this is always three except for
// schoolhouse (1) and college (2).
int max_workers_for_building( e_colony_building building );

// Should be called when a colony building is added to the colony
// as it will ensure that any one-time changes that need to be
// made will be made. It also ensures that the buildings remain
// in a state consistent with the OG's rules, in that e.g. having
// a Cathedral implies that you also have a Church.
void add_colony_building( Colony&           colony,
                          e_colony_building building );

e_colony_building barricade_type_to_colony_building(
    e_colony_barricade_type barricade );

maybe<e_colony_barricade_type> barricade_for_colony(
    Colony const& colony );

} // namespace rn
