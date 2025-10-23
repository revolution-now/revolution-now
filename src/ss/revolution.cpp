/****************************************************************
**revolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-04-06.
*
* Description: Holds state related to the war of independence.
*
*****************************************************************/
#include "revolution.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** RevolutionState
*****************************************************************/
valid_or<string> RevolutionState::validate() const {
  // Check that rebel sentiment is in the right range.
  REFL_VALIDATE( rebel_sentiment >= 0 && rebel_sentiment <= 100,
                 "rebel sentiment must be in the range 0-100, "
                 "but is equal to {}.",
                 rebel_sentiment );

  // Check that, if we haven't declared, that the
  // post-declaration event flags are not set.
  if( status < e_revolution_status::declared ) {
    REFL_VALIDATE( !continental_army_mobilized,
                   "the continental_army_mobilized flag cannot "
                   "be set prior to the declaration." );
    REFL_VALIDATE( !gave_independence_war_hints,
                   "the gave_independence_war_hints flag cannot "
                   "be set prior to the declaration." );
    REFL_VALIDATE( !intervention_force_deployed,
                   "the intervention_force_deployed flag cannot "
                   "be set prior to the declaration." );
  }

  return valid;
}

} // namespace rn
