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
** ElevationParameters.
*****************************************************************/
valid_or<string> ElevationParameters::validate() const {
  int total = 0;
  for( auto const& [elevation, weight] : weights )
    total += weight.value;
  REFL_VALIDATE(
      total > 0,
      "the sum of the elevation weights must be > 0" );
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
