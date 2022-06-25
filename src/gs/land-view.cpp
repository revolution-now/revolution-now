/****************************************************************
**land-view.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-22.
*
* Description: Save-game state for the land view.
*
*****************************************************************/
#include "land-view.hpp"

// Revolution Now
#include "lua.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_gs_land_view() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// LandViewState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::LandViewState;
  auto u  = st.usertype.create<U>();

  u["viewport"] = &U::viewport;
};

} // namespace

} // namespace rn
