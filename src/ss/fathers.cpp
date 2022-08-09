/****************************************************************
**fathers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-25.
*
* Description: Game state for founding fathers.
*
*****************************************************************/
#include "fathers.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_ss_fathers();
void linker_dont_discard_module_ss_fathers() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::FoundingFathersMap;
  auto u  = st.usertype.create<U>();

  // TODO: make this generic for enum maps.
  u[lua::metatable_key]["__index"] =
      []( U& obj, e_founding_father father ) {
        return obj[father];
      };

  // TODO: make this generic for enum maps.
  u[lua::metatable_key]["__newindex"] =
      []( U& obj, e_founding_father father, bool b ) {
        obj[father] = b;
      };

  // !! NOTE: because we overwrote the __*index metamethods on
  // this userdata we cannot add any further (non-metatable) mem-
  // bers on this object, since there will be no way to look them
  // up by name.
};

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::FoundingFathersState;

  auto u = st.usertype.create<U>();

  u["bells"] = &U::bells;
  u["has"]   = &U::has;
};

} // namespace

} // namespace rn
