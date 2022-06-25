/****************************************************************
**fathers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-26.
*
* Description: Api for querying properties of founding fathers.
*
*****************************************************************/
#include "fathers.hpp"

// Revoulution Now
#include "lua.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/register.hpp"
#include "luapp/rtable.hpp"
#include "luapp/state.hpp"
#include "luapp/types.hpp"

// config
#include "config/fathers.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** e_founding_father
*****************************************************************/
string_view founding_father_name( e_founding_father father ) {
  return config_fathers.fathers[father].name;
}

/****************************************************************
** e_founding_father_type
*****************************************************************/
e_founding_father_type founding_father_type(
    e_founding_father father ) {
  return config_fathers.fathers[father].type;
}

vector<e_founding_father> founding_fathers_for_type(
    e_founding_father_type type ) {
  vector<e_founding_father> res;
  for( e_founding_father father :
       refl::enum_values<e_founding_father> )
    if( config_fathers.fathers[father].type == type )
      res.push_back( father );
  return res;
}

string_view founding_father_type_name(
    e_founding_father_type type ) {
  return config_fathers.types[type].name;
}

void linker_dont_discard_module_fathers() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  using U = ::rn::FoundingFathersMap;
  auto u  = st.usertype.create<U>();

  u[lua::metatable_key]["__index"] =
      []( U& obj, e_founding_father father ) {
        return obj[father];
      };

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
