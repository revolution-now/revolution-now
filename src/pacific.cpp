/****************************************************************
**pacific.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-28.
*
* Description: Helpers for distinguishing pacific/atlantic.
*
*****************************************************************/
#include "pacific.hpp"

// ss
#include "ss/terrain.hpp"

using namespace std;

namespace rn {

using ::gfx::point;
using ::gfx::size;

/****************************************************************
** Public API.
*****************************************************************/
bool is_atlantic_side_of_map( TerrainState const& terrain,
                              point const tile ) {
  size const map_size = terrain.world_size_tiles();
  return tile.x >= map_size.w / 2;
}

bool is_pacific_side_of_map( TerrainState const& terrain,
                             point const tile ) {
  return !is_atlantic_side_of_map( terrain, tile );
}

} // namespace rn
