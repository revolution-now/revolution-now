/****************************************************************
**key.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-08-08.
*
* Description: Game Exchange Key.
*
*****************************************************************/
#include "key.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** NationSetup
*****************************************************************/
valid_or<string> NationSetup::validate() const {
  REFL_VALIDATE( !name.empty(),
                 "nation name must not be empty" );
  return valid;
}

/****************************************************************
** RiverParameters
*****************************************************************/
valid_or<string> RiverParameters::validate() const {
  REFL_VALIDATE( min_length >= 2,
                 "minimum river length must be >= 2" );
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
** PerlinMountains.
*****************************************************************/
valid_or<string> PerlinMountains::validate() const {
  if( enabled ) {
    REFL_VALIDATE( density_divisor >= 1.0,
                   "perlin mountains density divisor must be at "
                   "least 1.0." );
  } else {
    REFL_VALIDATE( density_divisor == 0.0,
                   "perlin mountains density divisor must be "
                   "zero when disabled." );
  }
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
** GameSettingsSetup
*****************************************************************/
valid_or<string> GameSettingsSetup::validate() const {
  REFL_VALIDATE( starting_year >= 0,
                 "starting_year must be >= 0." );
  return valid;
}

/****************************************************************
** GeneratedMapSetup
*****************************************************************/
valid_or<string> GeneratedMapSetup::validate() const {
  REFL_VALIDATE( size.w >= kMapWidthMin,
                 "map width must be >= {}", kMapWidthMin );
  REFL_VALIDATE( size.h >= kMapHeightMin,
                 "map height must be >= {}", kMapHeightMin );
  REFL_VALIDATE( size.area() > 0, "map area must be > 0" );
  return valid;
}

} // namespace rn
