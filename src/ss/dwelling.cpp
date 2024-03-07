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

namespace {

using ::base::valid;
using ::base::valid_or;

} // namespace

void linker_dont_discard_module_ss_dwelling();
void linker_dont_discard_module_ss_dwelling() {}

/****************************************************************
** DwellingRelationship
*****************************************************************/
valid_or<string> DwellingRelationship::validate() const {
  REFL_VALIDATE(
      dwelling_only_alarm >= 0 && dwelling_only_alarm <= 99,
      "dwelling_only_alarm must be in [0, 99]." );
  return valid;
}

/****************************************************************
** Dwelling
*****************************************************************/
valid_or<string> Dwelling::validate() const {
  // A dwelling object must either have an id (meaning that it is
  // a real dwelling on the map) or it must have a `frozen` rep-
  // resentation (meaning that it is a snapshot of the dwelling
  // when last visited) but cannot have both.
  bool const real_dwelling = ( id != 0 );
  bool const has_frozen    = frozen.has_value();
  REFL_VALIDATE( real_dwelling != has_frozen,
                 "Dwelling must have either a non-zero ID or a "
                 "frozen state, and not both." );

  return valid;
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

    u["seeking_primary"]     = &U::seeking_primary;
    u["seeking_secondary_1"] = &U::seeking_secondary_1;
    u["seeking_secondary_2"] = &U::seeking_secondary_2;
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

    u["dwelling_only_alarm"]   = &U::dwelling_only_alarm;
    u["has_spoken_with_chief"] = &U::has_spoken_with_chief;
  }();

  // Dwelling.
  [&] {
    using U = ::rn::Dwelling;

    auto u = st.usertype.create<U>();

    u["id"]         = &U::id;
    u["is_capital"] = &U::is_capital;
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
