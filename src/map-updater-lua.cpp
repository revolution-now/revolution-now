/****************************************************************
**map-updater-lua.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-07.
*
* Description: Lua extension for IMapUpdater.
*
*****************************************************************/
#include "map-updater-lua.hpp"

// Revolution Now
#include "map-updater.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_map_updater_lua();
void linker_dont_discard_module_map_updater_lua() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::IMapUpdater;

  auto u = st.usertype.create<U>();

  u["redraw"]      = &U::redraw;
  u["toggle_grid"] = []( U& o ) {
    o.mutate_options_and_redraw(
        []( MapUpdaterOptions& options ) {
          options.grid = !options.grid;
        } );
  };
};

} // namespace

} // namespace rn
