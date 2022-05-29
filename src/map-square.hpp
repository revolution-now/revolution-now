/****************************************************************
**map-square.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-02-01.
*
* Description: Represents a single square of land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "mv-points.hpp"
#include "terrain.hpp"

// Rds
#include "map-square.rds.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace rn {

bool is_land( MapSquare const& square );
bool is_water( MapSquare const& square );

e_surface surface_type( MapSquare const& square );

// The direction is from the source to the dest square.
MovementPoints movement_points_required(
    MapSquare const& src_square, MapSquare const& dst_square,
    e_direction d );

e_terrain effective_terrain( MapSquare const& square );

// This includes either clearing a forest or irrigating.
bool can_plow( MapSquare const& square );

bool has_forest( MapSquare const& square );

// Will check-fail if the square has no forest.
void clear_forest( MapSquare& square );

// This includes *only* irrigation and not forest-clearing.
// Will return false if the square already has irrigation.
bool can_irrigate( MapSquare const& square );

// This will apply irrigation (not forest-clearing) to the square
// and will check-fail if it cannot be irrigated or if there is
// already irrigation there.
void irrigate( MapSquare& square );

// This is mainly for testing. It will create a minimal valid
// MapSquare object that has the given effective terrain type.
MapSquare map_square_for_terrain( e_terrain terrain );

// This function contains the logic that determines what resource
// is present and active on the square given the ground_resource
// and forest_resource together with the forest status of the
// square.
maybe<e_natural_resource> effective_resource(
    MapSquare const& square );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {
LUA_USERDATA_TRAITS( ::rn::MapSquare, owned_by_cpp ){};
}
