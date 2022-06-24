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
#include "lua.hpp"

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

bool can_plow( e_terrain terrain ) {
  auto const& info = config_terrain.types[terrain];
  return info.can_irrigate || info.cleared_forest;
}

maybe<e_ground_terrain> cleared_forest( e_terrain terrain ) {
  return config_terrain.types[terrain].cleared_forest;
}

bool can_irrigate( e_terrain terrain ) {
  return config_terrain.types[terrain].can_irrigate;
}

maybe<e_ground_terrain> to_ground_terrain( e_terrain terrain ) {
  switch( terrain ) {
    case e_terrain::arctic: return e_ground_terrain::arctic;
    case e_terrain::boreal: return nothing;
    case e_terrain::broadleaf: return nothing;
    case e_terrain::conifer: return nothing;
    case e_terrain::desert: return e_ground_terrain::desert;
    case e_terrain::grassland:
      return e_ground_terrain::grassland;
    case e_terrain::hills: return nothing;
    case e_terrain::marsh: return e_ground_terrain::marsh;
    case e_terrain::mixed: return nothing;
    case e_terrain::mountains: return nothing;
    case e_terrain::ocean: return nothing;
    case e_terrain::plains: return e_ground_terrain::plains;
    case e_terrain::prairie: return e_ground_terrain::prairie;
    case e_terrain::rain: return nothing;
    case e_terrain::savannah: return e_ground_terrain::savannah;
    case e_terrain::scrub: return nothing;
    case e_terrain::swamp: return e_ground_terrain::swamp;
    case e_terrain::tropical: return nothing;
    case e_terrain::tundra: return e_ground_terrain::tundra;
    case e_terrain::wetland: return nothing;
  }
}

bool has_forest( e_terrain terrain ) {
  return cleared_forest( terrain ).has_value();
}

e_terrain from_ground_terrain( e_ground_terrain ground ) {
  switch( ground ) {
    case e_ground_terrain::arctic: return e_terrain::arctic;
    case e_ground_terrain::desert: return e_terrain::desert;
    case e_ground_terrain::grassland:
      return e_terrain::grassland;
    case e_ground_terrain::marsh: return e_terrain::marsh;
    case e_ground_terrain::plains: return e_terrain::plains;
    case e_ground_terrain::prairie: return e_terrain::prairie;
    case e_ground_terrain::savannah: return e_terrain::savannah;
    case e_ground_terrain::swamp: return e_terrain::swamp;
    case e_ground_terrain::tundra: return e_terrain::tundra;
  }
}

} // namespace rn
