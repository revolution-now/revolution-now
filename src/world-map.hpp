/****************************************************************
**world-map.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-23.
*
* Description: Handles interaction with the world map.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "game-state.hpp"
#include "map-square.hpp"
#include "matrix.hpp"

namespace rn {

inline constexpr auto world_size = Delta{ 200_w, 100_h };

// FIXME: temporary.
void generate_terrain();

using WorldMap = Matrix<MapSquare>;

Delta world_size_tiles();
Delta world_size_pixels();
Rect  world_rect_tiles();
Rect  world_rect_pixels();

// FIXME: deprecated, prefer those that take TerrainState.
bool square_exists( Y y, X x );
bool square_exists( Coord coord );

bool square_exists( TerrainState const& terrain_state,
                    Coord               coord );

// FIXME: deprecated, prefer those that take TerrainState.
MapSquare const& square_at( Coord coord );
MapSquare&       mutable_square_at( Coord coord );

MapSquare const& square_at( TerrainState const& terrain_state,
                            Coord               coord );
MapSquare& square_at( TerrainState& terrain_state, Coord coord );

// Throws if coord is not on map.
// FIXME: deprecated, prefer those that take TerrainState.
bool is_land( Coord coord );

bool is_land( TerrainState const& terrain_state, Coord coord );

/****************************************************************
** Testing
*****************************************************************/
// FIXME: remove
void generate_unittest_terrain();

} // namespace rn
