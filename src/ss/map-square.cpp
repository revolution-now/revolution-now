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
  if( surface == e_surface::land ) {
    // The OG does not have a way of representing this. The NG
    // can represent it in MapSquare but not in other cases (e.g.
    // e_terrain cannot represent it) so we must prohibit it.
    bool const has_forest_on_arctic =
        overlay == e_land_overlay::forest &&
        ground == e_biome::arctic;
    REFL_VALIDATE( !has_forest_on_arctic,
                   "cannot have forest on an arctic tile." );

    REFL_VALIDATE( !sea_lane,
                   "cannot have Sea Lane on land tiles." );

    REFL_VALIDATE(
        ground_resource != e_natural_resource::fish,
        "prime fishing resource not allowed on land tiles." );
    REFL_VALIDATE( forest_resource != e_natural_resource::fish,
                   "prime fishing resource not allowed on "
                   "forest/land tiles." );

    if( river == e_river::minor ) {
      // NOTE: Although the OG never seems to produce maps with
      // minor rivers and hills on the same tile, its data model
      // does support it and our bridge does support it, so we
      // will allow it here.
      REFL_VALIDATE( true || // disabled
                     overlay != e_land_overlay::hills );

      // We do not support minor rivers on mountains tiles for
      // compatibility with the original game which does not have
      // a way to represent this.
      REFL_VALIDATE( overlay != e_land_overlay::mountains,
                     "cannot have a minor river and a mountain "
                     "on the same tile." );
    }

    if( river == e_river::major ) {
      // NOTE: Although the OG never seems to produce maps with
      // major rivers and mountains on the same tile, its data
      // model does support it and our bridge does support it, so
      // we will allow it here.
      REFL_VALIDATE( true || // disabled
                     overlay != e_land_overlay::mountains );

      // We do not support major rivers on hills tiles for com-
      // patibility with the original game which does not have a
      // way to represent this.
      REFL_VALIDATE( overlay != e_land_overlay::hills,
                     "cannot have a major river and hills on "
                     "the same tile." );
    }
  }
  return valid;
}

} // namespace rn
