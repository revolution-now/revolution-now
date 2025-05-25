/****************************************************************
**siege.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-25.
*
* Description: Handles the colony under-siege status.
*
*****************************************************************/
#pragma once

namespace rn {

struct Colony;
struct SSConst;

// Checks if the colony is under seige, which is defined as a
// state where there are more foreign units of nations at war
// with us than there are friendly units in the colony square to-
// gether with the eight surrounding squares. In that case,
// colonists are not allowed to move from the fields to the
// gates. NOTE: This mechanic works the same way during the war
// of independence.
bool is_colony_under_siege( SSConst const& ss,
                            Colony const& colony );

} // namespace rn
