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
#include "refl/ext.hpp"
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

void linker_dont_discard_module_ss_dwelling();
void linker_dont_discard_module_ss_dwelling() {}

/****************************************************************
** DwellingRelationship
*****************************************************************/
base::valid_or<string> DwellingRelationship::validate() const {
  REFL_VALIDATE(
      dwelling_only_alarm >= 0 && dwelling_only_alarm <= 99,
      "dwelling_only_alarm must be in [0, 99]." );
  return base::valid;
}

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

    u["dwelling_only_alarm"] = &U::dwelling_only_alarm;
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
    u["trading"]    = &U::trading;
    u["teaches"]    = &U::teaches;
    u["has_taught"] = &U::has_taught;

    u["relationship"] =
        []( U& o, e_nation nation ) -> DwellingRelationship& {
      return o.relationship[nation];
    };
  }();
};

} // namespace

} // namespace rn
