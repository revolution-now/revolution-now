/****************************************************************
**bridge.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-12.
*
* Description: Translates between the OG and NG save state
*              representation.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// C++ standard library
#include <string>

namespace rn {
struct RootState;
struct RealTerrain;
}

namespace sav {
struct ColonySAV;
struct MapFile;
}

// Choose a neutral namespace so that both the old and new game
// data types must be qualified for better readability.
namespace bridge {

base::valid_or<std::string> convert_to_rn(
    sav::ColonySAV const& in, rn::RootState& out );

base::valid_or<std::string> convert_to_og(
    rn::RootState const& in, sav::ColonySAV& out );

base::valid_or<std::string> convert_to_rn(
    sav::MapFile const& in, rn::RealTerrain& out );

base::valid_or<std::string> convert_to_og(
    rn::RealTerrain const& in, sav::ColonySAV& out );

} // namespace bridge
