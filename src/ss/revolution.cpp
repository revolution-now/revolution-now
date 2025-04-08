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
  REFL_VALIDATE( rebel_sentiment >= 0 && rebel_sentiment <= 100,
                 "rebel sentiment must be in the range 0-100, "
                 "but is equal to {}.",
                 rebel_sentiment );

  return valid;
}

} // namespace rn
