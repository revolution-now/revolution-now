/****************************************************************
**lcr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-24.
*
* Description: Types used by the LCR config file.
*
*****************************************************************/
#include "lcr.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> config_lcr_t::validate() const {
  // Check that the rumor outcome weights sum to 100 for each ex-
  // plorer type.
  for( auto bucket :
       refl::enum_values<e_lcr_explorer_category> ) {
    int total = 0;
    for( auto outcome : refl::enum_values<e_rumor_type> )
      total += rumor_type_weights[bucket][outcome];
    REFL_VALIDATE(
        total == 100,
        "LCR rumor type weights for the '{}' category unit must "
        "sum to 100, but instead sum to {}.",
        bucket, total );

    total = 0;
    for( auto outcome : refl::enum_values<e_burial_mounds_type> )
      total += burial_mounds_type_weights[bucket][outcome];
    REFL_VALIDATE(
        total == 100,
        "LCR burial mounds type weights for the '{}' unit "
        "category must sum to 100, but instead sum to {}.",
        bucket, total );
  }
  return base::valid;
}

} // namespace rn
