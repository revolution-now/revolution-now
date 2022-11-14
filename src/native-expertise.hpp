/****************************************************************
**native-expertise.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-12.
*
* Description: Selects a teaching expertise for a native
*              dwelling.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/unit-type.rds.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct Dwelling;
struct SSConst;
struct TS;

// Looks at all of the production values (outdoor and some in-
// door) on all square in the vicinity of the dwelling and re-
// turns weights, where a given weight is larger if more of that
// thing is produced. Guarantees that the weights returned will
// always contain at least one item with a non-zero weight, and
// all weights will be >= 0.
refl::enum_map<e_unit_activity, int> dwelling_expertise_weights(
    SSConst const& ss, Dwelling const& dwelling );

// Randomly selects an expertise according to the weights.
e_unit_activity select_expertise_for_dwelling(
    TS& ts, refl::enum_map<e_unit_activity, int> weights );

} // namespace rn
