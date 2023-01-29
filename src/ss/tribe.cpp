/****************************************************************
**tribe.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-30.
*
* Description: Represents one indian tribe.
*
*****************************************************************/
#include "tribe.hpp"

// config
#include "config/natives.rds.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** TribeRelationship
*****************************************************************/
base::valid_or<string> TribeRelationship::validate() const {
  REFL_VALIDATE( tribal_alarm >= 0 && tribal_alarm <= 99,
                 "tribal_alarm must be in [0, 99]." );
  return base::valid;
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  // TribeRelationshipMap.
  // TODO: make this generic.
  [&] {
    using U = ::rn::TribeRelationshipMap;
    auto u  = st.usertype.create<U>();

    u[lua::metatable_key]["__index"] =
        [&]( U& obj, e_nation nation ) -> TribeRelationship& {
      LUA_CHECK( st, obj[nation].has_value(),
                 "this tribe has not yet encountered the {}.",
                 nation );
      return *obj[nation];
    };
  }();

  // TribeRelationship.
  [&] {
    using U = ::rn::TribeRelationship;

    auto u = st.usertype.create<U>();

    u["at_war"]       = &U::at_war;
    u["tribal_alarm"] = &U::tribal_alarm;
  }();

  // Tribe.
  [&] {
    using U = ::rn::Tribe;

    auto u = st.usertype.create<U>();

    u["type"]         = &U::type;
    u["muskets"]      = &U::muskets;
    u["horses"]       = &U::horses;
    u["relationship"] = &U::relationship;
  }();
};

} // namespace

} // namespace rn
