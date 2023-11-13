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
#include "ts.hpp"

// sav
#include "sav/binary.hpp"
#include "sav/bridge.hpp"
#include "sav/map-file.hpp"

// ss
#include "ss/terrain.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
valid_or<string> load_classic_map_file(
    std::string const& path, RealTerrain& real_terrain ) {
  sav::MapFile map_file;
  CHECK_HAS_VALUE( sav::load_map_file( path, map_file ) );
  CHECK_HAS_VALUE(
      bridge::convert_to_rn( map_file, real_terrain ) );
  return valid;
}

/****************************************************************
** Lua.
*****************************************************************/
namespace {

LUA_FN( import_map_file, void, std::string const& path ) {
  TS& ts = st["TS"].as<TS&>();

  ts.map_updater.modify_entire_map_no_redraw(
      [&]( RealTerrain& real_terrain ) {
        valid_or<string> const success =
            load_classic_map_file( path, real_terrain );
        LUA_CHECK( st, success.valid(),
                   "failed to load map file: {}",
                   success.error() );
      } );
}

} // namespace

void linker_dont_discard_module_classic_sav();
void linker_dont_discard_module_classic_sav() {}

} // namespace rn
