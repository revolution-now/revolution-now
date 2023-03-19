/****************************************************************
**fog-conv.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-18.
*
* Description: Converts real entities into fogged entities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/fog-square.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Colony;
struct SSConst;

// When a player views a square and their copy of it must be up-
// dated, this will properly update all of the information in the
// FogSquare such as and colonies or dwellings on the square and
// their properties, as well as the terrain itself. But note that
// this will not alter the fog-of-war state.
void copy_real_square_to_fog_square( SSConst const& ss,
                                     Coord          tile,
                                     FogSquare&     fog_square );

// For when you want to just convert a single dwelling to the fog
// version.
FogDwelling dwelling_to_fog_dwelling( SSConst const& ss,
                                      DwellingId dwelling_id );

// For when you want to just convert a single colony to the fog
// version.
FogColony colony_to_fog_colony( Colony const& colony );

} // namespace rn
