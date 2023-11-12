/****************************************************************
**binary.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-01.
*
* Description: API for loading and saving games in the classic
*              format of the OG.
*
*****************************************************************/
#pragma once

// base
#include "base/valid.hpp"

// C++ standard library.
#include <string>

namespace sav {

struct ColonySAV;
struct MapFile;

// These will load/save the OG's binary save files (*.SAV) into a
// data structure that precisely reflects its contents so that a
// load followed by a save will produce an identical file.
base::valid_or<std::string> load_binary( std::string const& path,
                                         ColonySAV& out );

base::valid_or<std::string> save_binary( std::string const& path,
                                         ColonySAV const&   in );

// These will load/save the OG's map files, i.e. those files gen-
// erated by the map editor. They tend to have the extension MP.
// For example, the "America" map that ships with the OG is named
// "AMER2.MP".
base::valid_or<std::string> load_map_file(
    std::string const& path, MapFile& out );

base::valid_or<std::string> save_map_file(
    std::string const& path, MapFile const& in );

} // namespace sav
