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
#include "ss/fog-square.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rn {

struct Colony;
struct SSConst;

// When a player views a square and their copy of it must be up-
// dated, this will properly update all of the information in the
// FrozenSquare such as and colonies or dwellings on the square
// and their properties, as well as the terrain itself. But note
// that this will not alter the fog-of-war state.
void copy_real_square_to_frozen_square(
    SSConst const& ss, Coord tile, FrozenSquare& frozen_square );

// For when you want to just convert a single dwelling to the fog
// version. Will use the builtin one if available.
FrozenDwelling dwelling_to_frozen_dwelling(
    SSConst const& ss, Dwelling const& dwelling );

// For when you want to just convert a single colony to the
// frozen version. Will use the builtin one if available.
FrozenColony colony_to_frozen_colony( SSConst const& ss,
                                      Colony const& colony );

} // namespace rn
