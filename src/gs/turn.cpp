/****************************************************************
**turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-02.
*
# Description: Turn-related save-game state.
*
*****************************************************************/
#include "turn.hpp"

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

/****************************************************************
** TurnTimePoint
*****************************************************************/
// NOTE: this was originally put here just to force the linker to
// include this module for the lua setup.
base::valid_or<string> TurnTimePoint::validate() const {
  REFL_VALIDATE( year >= 0, "game year must be >= 0." );
  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

// TurnTimePoint
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::TurnTimePoint;
  auto u  = st.usertype.create<U>();

  u["year"]   = &U::year;
  u["season"] = &U::season;
};

// TurnState
LUA_STARTUP( lua::state& st ) {
  using U = ::rn::TurnState;
  auto u  = st.usertype.create<U>();

  u["time_point"] = &U::time_point;
};

} // namespace

} // namespace rn
