/****************************************************************
**gs-settings.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-28.
*
* Description: Save-game state for game-wide settings.
*
*****************************************************************/
#include "ss/settings.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

base::valid_or<string> SettingsState::validate() const {
  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // SettingsState.
  [&] {
    using U = ::rn::SettingsState;
    auto u  = st.usertype.create<U>();

    u["difficulty"] = &U::difficulty;

    // NOTE: Game options are not exposed here; they are exposed
    // via a higher level API in the game-options module because
    // they require wrappers that execute functions when their
    // values are changed.
  }();
};

} // namespace

} // namespace rn
