/****************************************************************
**classic-sav.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-11-13.
*
* Description: Interface to the sav library for doing the things
*              that the game does with classic binary save files.
*
*****************************************************************/
#include "classic-sav.hpp"

// Revolution Now
#include "imap-updater.hpp"

// sav
#include "sav/binary.hpp"
#include "sav/bridge.hpp"
#include "sav/map-file.hpp"

// ss
#include "ss/terrain.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> load_classic_binary_map_file(
    std::string const& path, RealTerrain& real_terrain ) {
  sav::MapFile map_file;
  CHECK_HAS_VALUE( sav::load_map_file( path, map_file ) );
  CHECK_HAS_VALUE(
      bridge::convert_map_to_ng( map_file, real_terrain ) );
  return valid;
}

} // namespace rn
