/****************************************************************
**classic-sav.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-13.
*
* Description: Interface to the sav library for doing the things
*              that the game does with classic binary save files.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// C++ standard library
#include <string>

namespace rn {

struct RealTerrain;

base::valid_or<std::string> load_classic_map_file(
    std::string const& path, RealTerrain& real_terrain );

} // namespace rn
