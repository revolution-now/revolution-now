/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
# Description: Describes possible terrain types.
*
*****************************************************************/
#include "terrain.hpp"

// config
#include "config/terrain.rds.hpp"

// luapp
#include "luapp/state.hpp"

using namespace std;

namespace rn {

bool is_land( e_terrain terrain ) {
  return terrain != e_terrain::ocean;
}

bool is_water( e_terrain terrain ) {
  return terrain == e_terrain::ocean;
}

e_surface surface_type( e_terrain terrain ) {
  return is_land( terrain ) ? e_surface::land : e_surface::water;
}

maybe<e_ground_terrain> cleared_forest( e_terrain terrain ) {
  return config_terrain.types[terrain].cleared_forest;
}

bool has_forest( e_terrain terrain ) {
  return cleared_forest( terrain ).has_value();
}

} // namespace rn
