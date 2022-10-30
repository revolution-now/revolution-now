/****************************************************************
**dwelling.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Represents an indian dwelling.
*
*****************************************************************/
#include "dwelling.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"

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
  // DwellingTradingState.
  [&] {
    using U = ::rn::DwellingTradingState;

    auto u = st.usertype.create<U>();

    (void)u; // TODO
  }();

  // Dwelling.
  [&] {
    using U = ::rn::Dwelling;

    auto u = st.usertype.create<U>();

    u["tribe"]      = &U::tribe;
    u["population"] = &U::population;
    u["muskets"]    = &U::muskets;
    u["horses"]     = &U::horses;
    u["trading"]    = &U::trading;
  }();
};

} // namespace

} // namespace rn
