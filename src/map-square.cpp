/****************************************************************
**map-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#include "map-square.hpp"

// Revolution Now
#include "lua.hpp"
#include "nation.hpp"
#include "terrain.hpp"

// config
#include "config/terrain.rds.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

bool is_land( MapSquare const& square ) {
  return square.surface == e_surface::land;
}

bool is_water( MapSquare const& square ) {
  return square.surface == e_surface::water;
}

e_surface surface_type( MapSquare const& square ) {
  return square.surface;
}

e_terrain effective_terrain( MapSquare const& square ) {
  if( is_water( square ) ) return e_terrain::ocean;
  if( !square.overlay.has_value() )
    return from_ground_terrain( square.ground );
  switch( *square.overlay ) {
    case e_land_overlay::hills: return e_terrain::hills;
    case e_land_overlay::mountains: return e_terrain::mountains;
    case e_land_overlay::forest: {
      maybe<e_terrain> forested =
          config_terrain
              .types[from_ground_terrain( square.ground )]
              .with_forest;
      DCHECK( forested );
      return *forested;
    }
  }
}

MovementPoints movement_points_required(
    MapSquare const& src_square, MapSquare const& dst_square,
    e_direction d ) {
  if( is_water( src_square ) || is_water( dst_square ) )
    return MovementPoints( 1 );
  // Both squares are land.

  if( src_square.road && dst_square.road )
    return MovementPoints::_1_3();

  // In the original game rivers, unlike roads, cannot run diago-
  // nally, and this is reflected in the movement point logic. We
  // get a movement bonus from moving from a river tile to an-
  // other river tile only if it is along one of the cardinal di-
  // rections.
  if( src_square.river && dst_square.river &&
      direction_type( d ) == e_direction_type::cardinal )
    return MovementPoints::_1_3();

  // We're moving from land to land without a road/river on both
  // squares, so it comes down to the terrain. In the game, the
  // only terrain that is relevant in this situation is that of
  // the terrain in the target square.
  return MovementPoints(
      config_terrain.types[effective_terrain( dst_square )]
          .movement_cost );
}

bool has_forest( MapSquare const& square ) {
  return square.overlay == e_land_overlay::forest;
}

// Will check-fail if the square has no forest.
void clear_forest( MapSquare& square ) {
  DCHECK( has_forest( square ) );
  square.overlay = nothing;
}

bool can_irrigate( MapSquare const& square ) {
  return !square.irrigation &&
         config_terrain.types[effective_terrain( square )]
             .can_irrigate;
}

void irrigate( MapSquare& square ) {
  CHECK( can_irrigate( square ) );
  square.irrigation = true;
}

bool can_plow( MapSquare const& square ) {
  return can_irrigate( square ) || has_forest( square );
}

MapSquare map_square_for_terrain( e_terrain terrain ) {
  if( terrain == e_terrain::hills )
    return MapSquare{ .surface = e_surface::land,
                      .overlay = e_land_overlay::hills };
  if( terrain == e_terrain::mountains )
    return MapSquare{ .surface = e_surface::land,
                      .overlay = e_land_overlay::mountains };
  if( maybe<e_ground_terrain> cleared =
          cleared_forest( terrain );
      cleared )
    // Forest square.
    return MapSquare{ .surface = e_surface::land,
                      .ground  = *cleared,
                      .overlay = e_land_overlay::forest };
  if( terrain == e_terrain::ocean )
    return MapSquare{ .surface = e_surface::water };
  UNWRAP_CHECK( ground, to_ground_terrain( terrain ) );
  return MapSquare{ .surface = e_surface::land,
                    .ground  = ground };
}

maybe<e_natural_resource> effective_resource(
    MapSquare const& square ) {
  if( square.overlay == e_land_overlay::forest )
    return square.forest_resource;
  return square.ground_resource;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::MapSquare;

  auto u = st.usertype.create<rn::MapSquare>();

  u["surface"]         = &U::surface;
  u["ground"]          = &U::ground;
  u["overlay"]         = &U::overlay;
  u["river"]           = &U::river;
  u["ground_resource"] = &U::ground_resource;
  u["forest_resource"] = &U::forest_resource;
  u["irrigation"]      = &U::irrigation;
  u["road"]            = &U::road;
  u["sea_lane"]        = &U::sea_lane;
  u["lost_city_rumor"] = &U::lost_city_rumor;
};

} // namespace

} // namespace rn
