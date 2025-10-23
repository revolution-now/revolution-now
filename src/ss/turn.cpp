/****************************************************************
**turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
# Description: Turn-related save-game state.
*
*****************************************************************/
#include "turn.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TurnTimePoint
*****************************************************************/
base::valid_or<string> TurnTimePoint::validate() const {
  REFL_VALIDATE( year >= 0, "game year must be >= 0." );
  return base::valid;
}

} // namespace rn
