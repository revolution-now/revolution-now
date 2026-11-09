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

// ss
#include "ss/key.hpp"

// refl
#include "refl/ext.hpp"

namespace rn {

namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;
using ::refl::enum_values;

} // namespace

namespace config::map_gen {

/****************************************************************
** LandMass
*****************************************************************/
valid_or<string> LandMass::validate() const {
  REFL_VALIDATE(
      !fully_random.empty(),
      "land mass fully_random list must be non-empty." );

  for( e_land_mass const land_mass : enum_values<e_land_mass> ) {
    REFL_VALIDATE( customized[land_mass].mean >= 0.0,
                   "land mass mean must be in [0,1] for {}.",
                   land_mass );
    REFL_VALIDATE( customized[land_mass].mean <= 1.0,
                   "land mass mean must be in [0,1] for {}.",
                   land_mass );
    REFL_VALIDATE( customized[land_mass].min >= 0.0,
                   "land mass min must be in [0,1] for {}.",
                   land_mass );
    REFL_VALIDATE( customized[land_mass].min <= 1.0,
                   "land mass min must be in [0,1] for {}.",
                   land_mass );
    REFL_VALIDATE( customized[land_mass].max >= 0.0,
                   "land mass max must be in [0,1] for {}.",
                   land_mass );
    REFL_VALIDATE( customized[land_mass].max <= 1.0,
                   "land mass max must be in [0,1] for {}.",
                   land_mass );
  }
  return valid;
}

/****************************************************************
** LandForm
*****************************************************************/
valid_or<string> LandForm::validate() const {
  REFL_VALIDATE(
      !fully_random.empty(),
      "land form fully_random list must be non-empty." );
  return valid;
}

/****************************************************************
** Temperature.
*****************************************************************/
valid_or<string> Temperature::validate() const {
  REFL_VALIDATE(
      !fully_random.empty(),
      "temperature fully_random list must be non-empty." );
  return valid;
}

/****************************************************************
** Climate.
*****************************************************************/
valid_or<string> Climate::validate() const {
  REFL_VALIDATE(
      !fully_random.empty(),
      "climate fully_random list must be non-empty." );
  return valid;
}

/****************************************************************
** PerlinLandFormRange
*****************************************************************/
valid_or<string> PerlinLandFormRange::validate() const {
  REFL_VALIDATE( scale.mean >= 1.0,
                 "Perlin land form scale must be >= 1.0" );
  REFL_VALIDATE( scale.min >= 1.0,
                 "Perlin land form min must be >= 1.0" );
  REFL_VALIDATE( scale.max >= 1.0,
                 "Perlin land form max must be >= 1.0" );
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
** Rivers
*****************************************************************/
valid_or<string> Rivers::validate() const {
  REFL_VALIDATE( min_length >= 2, "min_length must be >= 2" );
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
  REFL_VALIDATE( default_map_size.area() > 0,
                 "map area must be > 0" );
  return valid;
}

} // namespace rn
