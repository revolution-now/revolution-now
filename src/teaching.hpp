/****************************************************************
**teaching.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-19.
*
* Description: All things related to schools/colleges/university.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/colony-enums.rds.hpp"

// Rds
#include "teaching.rds.hpp"

namespace rn {

struct Colony;
struct SS;
struct TS;

// Maximum number of teachers allowed in this colony given the
// relevant buildings. If the colony has no relevant buildings
// then the result will be zero.
int max_teachers_allowed( Colony const& colony );

// This will take the list of teaching in the indoor_jobs list
// and will rewrite the list in the teachers map in order to make
// it consistent with the former. Any teachers that remain in the
// map after the update will not have their turns affected.
//
// This must be called anytime the list of colony indoor_jobs are
// updated.
void sync_colony_teachers( Colony& colony );

// This will actually evolve the teachers and promote any units.
ColonyTeachingEvolution evolve_teachers( SS& ss, TS& ts,
                                         Colony& colony );

// In the original game, this is any expert colonist, or unit
// type that derives from one. All unit types can teach in uni-
// versities, most in colleges, and some in schoolhouses. Upon
// failure it will produce a message suitable for displaying to
// the user.
base::valid_or<std::string> can_unit_teach_in_building(
    e_unit_type type, e_school_type school_type );

} // namespace rn
