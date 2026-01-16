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
** Temperature
*****************************************************************/
valid_or<string> Temperature::validate() const {
  return valid;
}

/****************************************************************
** Climate
*****************************************************************/
valid_or<string> Climate::validate() const {
  return valid;
}

} // namespace config::map_gen

/****************************************************************
** config_map_gen_t
*****************************************************************/
valid_or<string> config_map_gen_t::validate() const {
  return valid;
}

} // namespace rn
