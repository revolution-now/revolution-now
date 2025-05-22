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
        [&]( U& obj, e_player player ) -> TribeRelationship& {
      return obj[player];
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

    u["type"]           = &U::type;
    u["muskets"]        = &U::muskets;
    u["horse_herds"]    = &U::horse_herds;
    u["horse_breeding"] = &U::horse_breeding;
    u["relationship"]   = &U::relationship;
  }();
};

} // namespace

} // namespace rn
