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

    u["at_war"] = &U::at_war;
  }();

  // Tribe.
  [&] {
    using U = ::rn::Tribe;

    auto u = st.usertype.create<U>();

    u["type"]         = &U::type;
    u["relationship"] = &U::relationship;
  }();
};

} // namespace

} // namespace rn
