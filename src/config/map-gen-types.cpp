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
  // This value was found empirically... for some reason any
  // value <= 1.0 gives non-sensical results. That's ok because a
  // scal of one is ~ one tile which is the smallest unit anyway.
  double constexpr kMinScale = 1.0;
  // NOTE: this is > and not >= since 1.0 yields bad results.
  REFL_VALIDATE( scale > kMinScale, "perlin scale must be > {}",
                 kMinScale );
  return valid;
}

/****************************************************************
** PerlinEdgeSuppression
*****************************************************************/
valid_or<string> PerlinEdgeSuppression::validate() const {
  REFL_VALIDATE(
      strength.w >= 0.0,
      "perlin edge suppression strengths must be >= 0.0" );
  REFL_VALIDATE(
      strength.h >= 0.0,
      "perlin edge suppression strengths must be >= 0.0" );
  return valid;
}

/****************************************************************
** WetnessRowModulationConfig
*****************************************************************/
valid_or<string> WetnessRowModulationConfig::validate() const {
  // NOTE: amplitude is allowed to be negative.
  REFL_VALIDATE( width > 0, "width must be > 0." );
  return valid;
}

/****************************************************************
** Wetness
*****************************************************************/
valid_or<string> WetnessConfig::validate() const {
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
** FormationGrowth
*****************************************************************/
valid_or<string> FormationGrowth::validate() const {
  REFL_VALIDATE( rate >= 0.0,
                 "formation growth rate must be >= 0" );
  REFL_VALIDATE(
      edge_decay.w >= 0 && edge_decay.w <= 1.0,
      "formation edge decay parameters must be in [0,1]" );
  REFL_VALIDATE(
      edge_decay.h >= 0 && edge_decay.h <= 1.0,
      "formation edge decay parameters must be in [0,1]" );
  REFL_VALIDATE( max_size >= 0,
                 "formation max size must be >= 0" );
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
