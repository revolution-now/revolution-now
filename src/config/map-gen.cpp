/****************************************************************
**map-gen.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-03.
*
* Description: Config data for the map-gen module.
*
*****************************************************************/
#include "map-gen.hpp"

// refl
#include "refl/ext.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;

} // namespace

namespace config::map_gen {

/****************************************************************
** LandMass
*****************************************************************/
valid_or<string> LandMass::validate() const {
  return valid;
}

/****************************************************************
** LandForm
*****************************************************************/
valid_or<string> LandForm::validate() const {
  return valid;
}

/****************************************************************
** InlandLakes
*****************************************************************/
valid_or<string> InlandLakes::validate() const {
  REFL_VALIDATE(
      bias.min >= -1.0 && bias.max <= 1.0,
      "Inland lake bias range must be within [-1, 1]." );
  REFL_VALIDATE( bias.min <= bias.max,
                 "Inland lake bias min must be <= max." );
  return valid;
}

/****************************************************************
** BiomeCurve
*****************************************************************/
valid_or<string> BiomeCurve::validate() const {
  REFL_VALIDATE( exp >= 2, "exp must be >= 2" );
  REFL_VALIDATE( exp % 2 == 0, "exp must be even" );
  // NOTE: we don't do these validations in the BiomeCurveParams
  // type itself because they don't need to apply in all cases
  // where that type is used, namely they don't need to apply
  // when it is used as a gradient.
  REFL_VALIDATE( params.weight >= 0.0, "weight must be >= 0.0" );
  REFL_VALIDATE( params.center >= 0.0, "center must be >= 0.0" );
  REFL_VALIDATE( params.stddev > 0.0, "stddev must be > 0.0" );
  REFL_VALIDATE( params.sub >= 0.0, "sub must be >= 0.0" );
  return valid;
}

} // namespace config::map_gen

/****************************************************************
** config_map_gen_t
*****************************************************************/
valid_or<string> config_map_gen_t::validate() const {
  REFL_VALIDATE( default_map_size.w >= kMapWidthMin,
                 "map width must be >= {}", kMapWidthMin );
  REFL_VALIDATE( default_map_size.h >= kMapHeightMin,
                 "map height must be >= {}", kMapHeightMin );
  // The biome distribution algo assumes even map height.
  REFL_VALIDATE( default_map_size.h % 2 == 0,
                 "map height must be even" );
  return valid;
}

} // namespace rn
