/****************************************************************
**map-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-26.
*
* Description: Serializable state representing a map square.
*
*****************************************************************/
#include "map-square.hpp"

// ss
#include "terrain-enums.rds.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

}

/****************************************************************
** MapSquare
*****************************************************************/
valid_or<string> MapSquare::validate() const {
  if( surface == e_surface::water ) {
    REFL_VALIDATE(
        !lost_city_rumor,
        "water tiles cannot have a lost city rumor." );
    REFL_VALIDATE( !road, "water tiles cannot have a road." );
    REFL_VALIDATE( !irrigation,
                   "water tiles cannot have irrigation." );
    REFL_VALIDATE( !overlay.has_value(),
                   "water tiles cannot have an overlay (forest, "
                   "mountains, or hills)." );
    REFL_VALIDATE(
        !ground_resource.has_value() ||
            ground_resource == e_natural_resource::fish,
        "the only prime resource available on water tiles is "
        "fish." );
    REFL_VALIDATE(
        !forest_resource.has_value(),
        "water tiles cannot have a prime forest resource." );
  }
  return valid;
}

} // namespace rn
