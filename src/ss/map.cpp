/****************************************************************
**map.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-30.
*
* Description: Save-game state for stuff that is associated with
*              the map but not used for rendering.
*
*****************************************************************/
#include "map.hpp"

// luapp
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

using ::base::valid;
using ::base::valid_or;

/****************************************************************
** MapState
*****************************************************************/
base::valid_or<string> MapState::validate() const {
  return valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // MapState.
  [&] {
    using U = ::rn::MapState;

    auto u = st.usertype.create<U>();

    u["depletion"] = &U::depletion;
  }();

  // ResourceDepletion.
  [&] {
    using U = ::rn::ResourceDepletion;

    auto u = st.usertype.create<U>();

    u["reset_depletion_counters"] = []( U& o ) {
      o.counters.clear();
    };
  }();
};

} // namespace

} // namespace rn
