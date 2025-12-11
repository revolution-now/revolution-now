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

// rds
#include "bridge.rds.hpp"

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

using ConvResult = base::valid_or<std::string>;

// Convert OG -> NG.  Note that this one reads the ID map.
ConvResult convert_to_ng( sav::ColonySAV const& in,
                          rn::RootState& out,
                          IdMap const& id_map );

// Convert NG -> OG.  Note that this one writes the ID map.
ConvResult convert_to_og( rn::RootState const& in,
                          sav::ColonySAV& out, IdMap& id_map );

ConvResult convert_map_to_ng( sav::MapFile const& in,
                              rn::RealTerrain& out );

ConvResult convert_map_to_og( rn::RealTerrain const& in,
                              sav::MapFile& out );

} // namespace bridge
