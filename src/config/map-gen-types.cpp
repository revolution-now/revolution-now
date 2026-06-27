/****************************************************************
**map-gen-types.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-03.
*
* Description: Config data for the map-gen-types module.
*
*****************************************************************/
#include "map-gen-types.hpp"

// ss
#include "ss/map-square.rds.hpp"

// refl
#include "refl/ext.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** PerlinLandForm
*****************************************************************/
valid_or<string> PerlinLandForm::validate() const {
  // TODO
  return valid;
}

/****************************************************************
** PerlinEdgeSuppression
*****************************************************************/
valid_or<string> PerlinEdgeSuppression::validate() const {
  // TODO
  return valid;
}

/****************************************************************
** WetnessRowModulation
*****************************************************************/
valid_or<string> WetnessRowModulation::validate() const {
  // NOTE: amplitude is allowed to be negative.
  REFL_VALIDATE( width > 0, "width must be > 0." );
  return valid;
}

/****************************************************************
** Wetness
*****************************************************************/
valid_or<string> Wetness::validate() const {
  REFL_VALIDATE( amplitude >= 0, "amplitude must be >= 0." );
  REFL_VALIDATE( accumulation >= 0,
                 "accumulation must be >= 0." );
  return valid;
}

/****************************************************************
** WeatherValue
*****************************************************************/
valid_or<string> WeatherValue::validate() const {
  REFL_VALIDATE( abs( value ) <= kWeatherValueMaxMagnitude,
                 "A valid temperature or climate value must be "
                 "an integer in the range [-{},{}].",
                 kWeatherValueMaxMagnitude,
                 kWeatherValueMaxMagnitude );
  return valid;
}

/****************************************************************
** Public API.
*****************************************************************/
maybe<e_terrain_formation> terrain_formation_for(
    MapSquare const& square ) {
  CHECK( square.surface == e_surface::land );
  if( !square.overlay.has_value() )
    return e_terrain_formation::clearing;
  switch( *square.overlay ) {
    case rn::e_land_overlay::forest:
      return nothing;
    case rn::e_land_overlay::hills:
      return e_terrain_formation::hills;
    case rn::e_land_overlay::mountains:
      return e_terrain_formation::mountains;
  }
}

} // namespace rn
