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
#include "luapp/ext-base.hpp"
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
  // GameOptionsMap.
  [&] {
    // TODO: make this generic.
    using U = ::rn::GameOptionsMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_game_flag_option c ) { return obj[c]; };

    u[lua::metatable_key]["__newindex"] =
        [&]( U& obj, e_game_flag_option c, bool on_off ) {
          obj[c] = on_off;
        };
  }();

  // GameOptions.
  [&] {
    using U = ::rn::GameOptions;
    auto u  = st.usertype.create<U>();

    u["flags"] = &U::flags;
  }();

  // SettingsState.
  [&] {
    using U = ::rn::SettingsState;
    auto u  = st.usertype.create<U>();

    u["difficulty"]   = &U::difficulty;
    u["game_options"] = &U::game_options;
  }();
};

} // namespace

} // namespace rn
