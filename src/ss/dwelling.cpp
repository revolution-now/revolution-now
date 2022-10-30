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

// base
#include "base/to-str-ext-std.hpp"

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

  // DwellingRelationshipMap.
  // TODO: make this generic.
  [&] {
    using U = ::rn::DwellingRelationshipMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_nation nation ) -> DwellingRelationship& {
      return obj[nation];
    };
  }();

  // DwellingRelationship.
  [&] {
    using U = ::rn::DwellingRelationship;

    auto u = st.usertype.create<U>();

    u["alarm"]      = &U::alarm;
    u["has_taught"] = &U::has_taught;
  }();

  // Dwelling.
  [&] {
    using U = ::rn::Dwelling;

    auto u = st.usertype.create<U>();

    u["id"]         = &U::id;
    u["tribe"]      = &U::tribe;
    u["is_capital"] = &U::is_capital;
    u["location"]   = &U::location;
    u["population"] = &U::population;
    u["muskets"]    = &U::muskets;
    u["horses"]     = &U::horses;
    u["trading"]    = &U::trading;
    // u["relationship"]    = &U::relationship;
    u["teaches"] = &U::teaches;
  }();
};

} // namespace

} // namespace rn
