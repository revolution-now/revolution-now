/****************************************************************
**fog-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Represents a player's view of a map square.
*
*****************************************************************/
#include "fog-square.hpp" // IWYU pragma: keep

// refl
#include "refl/to-str.hpp" // IWYU pragma: keep

// base
#include "base/to-str-ext-std.hpp" // IWYU pragma: keep

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** FrozenSquare
*****************************************************************/
valid_or<string> FrozenSquare::validate() const {
  // Make sure that that there is no square that contains both a
  // colony and a dwelling.
  REFL_VALIDATE( !colony.has_value() || !dwelling.has_value(),
                 "Cannot have both a colony and dwelling on the "
                 "same square." );

  // Colonies in the FrozenSquare are not real colonies and thus
  // must have id=0 and must have frozen info in them.
  if( colony.has_value() ) {
    REFL_VALIDATE( colony->frozen.has_value(),
                   "Colonies in FrozenSquare objects must have "
                   "frozen info present." );
    REFL_VALIDATE(
        colony->id == 0,
        "Colonies in FrozenSquare objects must have id=0." );
  }

  // Dwellings in the FrozenSquare are not real dwellings and
  // thus must have id=0 and must have frozen info in them.
  if( dwelling.has_value() ) {
    REFL_VALIDATE( dwelling->frozen.has_value(),
                   "Dwellings in FrozenSquare objects must have "
                   "frozen info present." );
    REFL_VALIDATE(
        dwelling->id == 0,
        "Dwellings in FrozenSquare objects must have id=0." );
  }

  return valid;
}

} // namespace rn
