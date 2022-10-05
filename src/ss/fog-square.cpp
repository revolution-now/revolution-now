/****************************************************************
**fog-square.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-05.
*
* Description: Represents a player's view of a map square.
*
*****************************************************************/
#include "fog-square.hpp"

// ss
#include "ss/map-square.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // FogSquare.
  [&] {
    using U = ::rn::FogSquare;

    auto u = st.usertype.create<U>();

    u["square"] = &U::square;
  }();
};

} // namespace

} // namespace rn
