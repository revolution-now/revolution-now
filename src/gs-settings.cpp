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
#include "gs-settings.hpp"

// Revolution Now
#include "lua.hpp"

// luapp
#include "luapp/ext-base.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

base::valid_or<string> SettingsState::validate() const {
  REFL_VALIDATE( difficulty >= 0,
                 "the difficulty level must be >= 0" );
  REFL_VALIDATE( difficulty <= 4,
                 "the difficulty level must be <= 4" );

  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// MarketItem
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::SettingsState;
  auto u  = st.usertype.create<U>();

  u["difficulty"] = &U::difficulty;
};

} // namespace

} // namespace rn
