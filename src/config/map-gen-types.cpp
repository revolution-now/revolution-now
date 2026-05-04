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

// refl
#include "refl/ext.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** PerlinLandForm
*****************************************************************/
valid_or<string> PerlinLandForm::validate() const {
  return valid;
}

/****************************************************************
** PerlinEdgeSuppression
*****************************************************************/
valid_or<string> PerlinEdgeSuppression::validate() const {
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
** BiomeWetDryModulation
*****************************************************************/
valid_or<string> BiomeWetDryModulation::validate() const {
  REFL_VALIDATE( accumulation >= 0,
                 "accumulation must be >= 0." );
  return valid;
}

} // namespace rn
