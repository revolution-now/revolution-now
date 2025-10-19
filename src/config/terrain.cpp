/****************************************************************
**config-terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-30.
*
* Description: Configuration info for terrain.
*
*****************************************************************/
#include "terrain.hpp"

// Rds
#include "ss/terrain-enums.rds.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/valid.hpp"

// C++ standard library
#include <string>

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;

}

base::valid_or<string> TerrainTypeInfo::validate() const {
  // If surface type is water than irrigation/forest related
  // fields must be false/null.
  if( surface == e_surface::water ) {
    REFL_VALIDATE(
        with_forest == nothing,
        "water tiles cannot have with_forest to be non-null." );
    REFL_VALIDATE( cleared_forest == nothing,
                   "water tiles cannot have cleared_forest to "
                   "be non-null." );
  }

  // All movement costs are larger than zero.
  REFL_VALIDATE( movement_cost > 0,
                 "all tile types must have a movement cost "
                 "larger than 0." );

  // If with_forest is set then it cannot be a ground terrain.
  if( with_forest.has_value() ) {
    REFL_VALIDATE(
        !to_ground_terrain( *with_forest ).has_value(),
        "the with_forest field must not refer to a ground type, "
        "but it refers to ground type {}.",
        *with_forest );
  }

  // At most one of with_forest or cleared_forest can be
  // non-null.
  REFL_VALIDATE(
      !( with_forest.has_value() && cleared_forest.has_value() ),
      "at most one of with_forest and cleared_forest can be "
      "set." );

  return base::valid;
}

base::valid_or<string> config_terrain_t::validate() const {
  // with_forest -> cleared_forest roundtrip.
  for( auto const& [terrain, info] : types ) {
    if( !info.with_forest.has_value() ) continue;
    auto const& with_forest_info = types[*info.with_forest];
    REFL_VALIDATE( with_forest_info.cleared_forest.has_value(),
                   "terrain type {} has with_forest={} but "
                   "terrain type {} has cleared_forest=null.",
                   terrain, *info.with_forest,
                   *info.with_forest );
    REFL_VALIDATE(
        with_forest_info.cleared_forest ==
            to_ground_terrain( terrain ),
        "terrain type {} has with_forest={} but terrain type {} "
        "has cleared_forest={}: inconsistent roundtrip.",
        terrain, *info.with_forest, *info.with_forest,
        with_forest_info.cleared_forest );
  }

  // cleared_forest -> with_forest roundtrip.
  for( auto const& [terrain, info] : types ) {
    if( !info.cleared_forest.has_value() ) continue;
    auto const& cleared_forest_info =
        types[from_ground_terrain( *info.cleared_forest )];
    REFL_VALIDATE( cleared_forest_info.with_forest.has_value(),
                   "terrain type {} has cleared_forest={} but "
                   "terrain type {} has with_forest=null.",
                   terrain, *info.cleared_forest,
                   *info.cleared_forest );
    REFL_VALIDATE(
        cleared_forest_info.with_forest == terrain,
        "terrain type {} has cleared_forest={} but terrain type "
        "{} has with_forest={}: inconsistent roundtrip.",
        terrain, *info.cleared_forest, *info.cleared_forest,
        cleared_forest_info.with_forest );
  }

  return base::valid;
}

maybe<e_ground_terrain> to_ground_terrain( e_terrain terrain ) {
  switch( terrain ) {
    case e_terrain::arctic:
      return e_ground_terrain::arctic;
    case e_terrain::boreal:
      return nothing;
    case e_terrain::broadleaf:
      return nothing;
    case e_terrain::conifer:
      return nothing;
    case e_terrain::desert:
      return e_ground_terrain::desert;
    case e_terrain::grassland:
      return e_ground_terrain::grassland;
    case e_terrain::hills:
      return nothing;
    case e_terrain::marsh:
      return e_ground_terrain::marsh;
    case e_terrain::mixed:
      return nothing;
    case e_terrain::mountains:
      return nothing;
    case e_terrain::ocean:
      return nothing;
    case e_terrain::plains:
      return e_ground_terrain::plains;
    case e_terrain::prairie:
      return e_ground_terrain::prairie;
    case e_terrain::rain:
      return nothing;
    case e_terrain::savannah:
      return e_ground_terrain::savannah;
    case e_terrain::scrub:
      return nothing;
    case e_terrain::swamp:
      return e_ground_terrain::swamp;
    case e_terrain::tropical:
      return nothing;
    case e_terrain::tundra:
      return e_ground_terrain::tundra;
    case e_terrain::wetland:
      return nothing;
  }
}

e_terrain from_ground_terrain( e_ground_terrain ground ) {
  switch( ground ) {
    case e_ground_terrain::arctic:
      return e_terrain::arctic;
    case e_ground_terrain::desert:
      return e_terrain::desert;
    case e_ground_terrain::grassland:
      return e_terrain::grassland;
    case e_ground_terrain::marsh:
      return e_terrain::marsh;
    case e_ground_terrain::plains:
      return e_terrain::plains;
    case e_ground_terrain::prairie:
      return e_terrain::prairie;
    case e_ground_terrain::savannah:
      return e_terrain::savannah;
    case e_ground_terrain::swamp:
      return e_terrain::swamp;
    case e_ground_terrain::tundra:
      return e_terrain::tundra;
  }
}

} // namespace rn
