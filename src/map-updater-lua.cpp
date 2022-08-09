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
    bool grid                = o.options().grid;
    o.mutable_options().grid = !grid;
    o.redraw();
  };
};

} // namespace

} // namespace rn
