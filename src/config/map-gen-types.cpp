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

double constexpr kBiomeSelfAffinityMin = -3.0;
double constexpr kBiomeSelfAffinityMax = 3.0;

} // namespace

/****************************************************************
** BiomeAffinity
*****************************************************************/
valid_or<string> BiomeAffinity::validate() const {
  REFL_VALIDATE( for_self >= kBiomeSelfAffinityMin &&
                     for_self <= kBiomeSelfAffinityMax,
                 "biome affinity for_self property must be in "
                 "the range [{},{}], instead found {}.",
                 kBiomeSelfAffinityMin, kBiomeSelfAffinityMax,
                 for_self );
  REFL_VALIDATE( !for_water.has_value() ||
                     ( *for_water >= 0.0 && *for_water <= 1.0 ),
                 "biome affinity for_water, if non-null, must "
                 "be in the range [0,1]." );
  return valid;
}

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

} // namespace rn
