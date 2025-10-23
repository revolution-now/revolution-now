/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-22.
*
* Description: Save-game state for the land view.
*
*****************************************************************/
#include "land-view.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> Viewport::validate() const {
  REFL_VALIDATE( zoom >= 0.0, "zoom must be larger than zero" );
  REFL_VALIDATE( zoom >= 0.0, "zoom must be less than one" );
  REFL_VALIDATE( center_x >= 0.0,
                 "x center must be larger than 0" );
  REFL_VALIDATE( center_y >= 0.0,
                 "y center must be larger than 0" );
  return base::valid;
}

} // namespace rn
