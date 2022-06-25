/****************************************************************
**terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Save-game state for terrain data.
*
*****************************************************************/
#include "terrain.hpp"

// gs
#include "gs/map-square.rds.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TerrainState
*****************************************************************/
base::valid_or<std::string> TerrainState::validate() const {
  return base::valid;
}

void TerrainState::validate_or_die() const {
  CHECK_HAS_VALUE( validate() );
}

TerrainState::TerrainState( wrapped::TerrainState&& o )
  : o_( std::move( o ) ) {
  // Populate transient state.
  // none.
}

TerrainState::TerrainState()
  : TerrainState( wrapped::TerrainState{ .world_map = {} } ) {
  validate_or_die();
}

Matrix<MapSquare> const& TerrainState::world_map() const {
  return o_.world_map;
}

Matrix<MapSquare>& TerrainState::mutable_world_map() {
  return o_.world_map;
}

Delta TerrainState::world_size_tiles() const {
  return o_.world_map.size();
}

Rect TerrainState::world_rect_tiles() const {
  return { .x = 0,
           .y = 0,
           .w = world_size_tiles().w,
           .h = world_size_tiles().h };
}

bool TerrainState::square_exists( Coord coord ) const {
  if( coord.x < 0 || coord.y < 0 ) return false;
  return coord.is_inside(
      Rect::from( Coord{}, world_map().size() ) );
}

base::maybe<MapSquare&> TerrainState::mutable_maybe_square_at(
    Coord coord ) {
  if( !square_exists( coord ) ) return base::nothing;
  return mutable_world_map()[coord.y][coord.x];
}

base::maybe<MapSquare const&> TerrainState::maybe_square_at(
    Coord coord ) const {
  if( !square_exists( coord ) ) return base::nothing;
  return world_map()[coord.y][coord.x];
}

MapSquare const& TerrainState::total_square_at(
    Coord coord ) const {
  static MapSquare const kArctic =
      MapSquare{ .surface = e_surface::land,
                 .ground  = e_ground_terrain::arctic };
  static MapSquare const kSeaLane =
      MapSquare{ .surface  = e_surface::water,
                 .ground   = {},
                 .sea_lane = true };
  Rect rect = world_rect_tiles();
  if( coord.x < rect.left_edge() ||
      coord.x >= rect.right_edge() )
    return kSeaLane;
  if( coord.y < rect.top_edge() ||
      coord.y >= rect.bottom_edge() )
    return kArctic;
  // This should never fail since coord should now be on the map.
  return square_at( coord );
}

MapSquare const& TerrainState::square_at( Coord coord ) const {
  base::maybe<MapSquare const&> res = maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

MapSquare& TerrainState::mutable_square_at( Coord coord ) {
  base::maybe<MapSquare&> res = mutable_maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

bool TerrainState::is_land( Coord coord ) const {
  return square_at( coord ).surface == e_surface::land;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::TerrainState;

  auto u = st.usertype.create<U>();

  u["placement_seed"]     = &U::placement_seed;
  u["set_placement_seed"] = &U::set_placement_seed;
  u["world_size_tiles"]   = &U::world_size_tiles;
  u["square_exists"]      = &U::square_exists;
};

} // namespace

} // namespace rn
