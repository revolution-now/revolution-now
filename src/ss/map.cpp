/****************************************************************
**map.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-30.
*
* Description: Save-game state for stuff that is associated with
*              the map but not used for rendering.
*
*****************************************************************/
#include "map.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

using ::base::valid;
using ::base::valid_or;

/****************************************************************
** MapState
*****************************************************************/
base::valid_or<string> MapState::validate() const {
  return valid;
}

} // namespace rn
