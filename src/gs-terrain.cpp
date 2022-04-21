/****************************************************************
**gs-terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Save-game state for terrain data.
*
*****************************************************************/
#include "gs-terrain.hpp"

// Revolution Now
#include "game-state.hpp"
#include "map-square.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TerrainState
*****************************************************************/
valid_or<std::string> TerrainState::validate() const {
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
  TerrainState const& terrain_state = GameState::terrain();
  return terrain_state.world_map().size();
}

Rect TerrainState::world_rect_tiles() const {
  return { 0_x, 0_y, world_size_tiles().w,
           world_size_tiles().h };
}

bool TerrainState::square_exists( Coord coord ) const {
  if( coord.x < 0_x || coord.y < 0_y ) return false;
  return coord.is_inside(
      Rect::from( Coord{}, world_map().size() ) );
}

maybe<MapSquare&> TerrainState::mutable_maybe_square_at(
    Coord coord ) {
  if( !square_exists( coord ) ) return nothing;
  return mutable_world_map()[coord.y][coord.x];
}

maybe<MapSquare const&> TerrainState::maybe_square_at(
    Coord coord ) const {
  if( !square_exists( coord ) ) return nothing;
  return world_map()[coord.y][coord.x];
}

MapSquare const& TerrainState::square_at( Coord coord ) const {
  maybe<MapSquare const&> res = maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

MapSquare& TerrainState::mutable_square_at( Coord coord ) {
  maybe<MapSquare&> res = mutable_maybe_square_at( coord );
  CHECK( res, "square {} does not exist!", coord );
  return *res;
}

bool TerrainState::is_land( Coord coord ) const {
  return rn::is_land( square_at( coord ) );
}

} // namespace rn
