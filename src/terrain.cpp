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

// Revolution Now
#include "config-files.hpp"
#include "config-terrain.hpp"

using namespace std;

namespace rn {

bool is_land( e_terrain terrain ) {
  return config_terrain.terrain.types[terrain].surface ==
         e_surface::land;
}

bool is_water( e_terrain terrain ) {
  return terrain == e_terrain::ocean;
}

e_surface surface_type( e_terrain terrain ) {
  return is_land( terrain ) ? e_surface::land : e_surface::water;
}

bool can_plow( e_terrain terrain ) {
  return !config_terrain.terrain.types[terrain]
              .plowed.is<PlowAction::none>();
}

maybe<e_terrain> cleared_forest( e_terrain terrain ) {
  UNWRAP_RETURN( cleared,
                 config_terrain.terrain.types[terrain]
                     .plowed.get_if<PlowAction::cleared>() );
  return cleared.to;
}

bool can_irrigate( e_terrain terrain ) {
  return config_terrain.terrain.types[terrain]
      .plowed.holds<PlowAction::irrigates>();
}

} // namespace rn
