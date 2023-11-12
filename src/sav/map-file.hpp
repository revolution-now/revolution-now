/****************************************************************
**map-file.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-12.
*
* Description: Describes the format of the *.MP files.
*
*****************************************************************/
#pragma once

// sav
#include "bytes.hpp"
#include "sav-struct.hpp"

// C++ standard library
#include <vector>

namespace sav {

struct MapFile {
  // Although the map file contains the size of the map here,
  // neither the game nor the map editor support anything other
  // than the standard size, for which these values are 58 and
  // 72, respectively. Note that the visible map size in the game
  // is only 56x70 since the outter border of tiles is not shown
  // on the map by the game.
  //
  // So we always expect the same values from MP files generated
  // by the OG, but we will allow other dimensions here just in
  // case.
  uint16_t map_size_x = {};
  uint16_t map_size_y = {};

  // These always seem to be 0x0004, but not sure what they mean.
  bytes<2> unknown = {};

  // This is really the only valuable part of the map file.
  std::vector<TILE> tile = {};

  // This appears unused by map files. Probably it is only in-
  // cluded because they wanted to write the "path" matrix, and
  // in the OG's data structure the "mask" is between the "tile"
  // and "path" matrices.
  std::vector<MASK> mask = {};

  // This may or may not be populated in a given map file; that
  // is because it only seems to get populated when the user se-
  // lects Map->Find Continents. So it should not be relied upon.
  // It is not clear if the game will compute/recompute this when
  // loading a map into the game.
  std::vector<PATH> path = {};
};

} // namespace sav
