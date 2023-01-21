/****************************************************************
**player.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#include "player.hpp"

// gs
#include "ss/fathers.hpp"
#include "ss/old-world-state.hpp"

// luapp
#include "luapp/enum.hpp"
#include "luapp/ext-base.hpp"
#include "luapp/register.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

void linker_dont_discard_module_player();
void linker_dont_discard_module_player() {}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_STARTUP( lua::state& st ) {
  {
    using U = ::rn::WoodcutMap;
    auto u  = st.usertype.create<U>();

    // TODO: make this generic for enum maps.
    u[lua::metatable_key]["__index"] =
        []( U& o, e_woodcut type ) { return o[type]; };

    // TODO: make this generic for enum maps.
    u[lua::metatable_key]["__newindex"] =
        []( U& o, e_woodcut type, bool b ) { o[type] = b; };

    // !! NOTE: because we overwrote the __*index metamethods on
    // this userdata we cannot add any further (non-metatable)
    // members on this object, since there will be no way to look
    // them up by name.
  };

  {
    using U = ::rn::Player;
    auto u  = st.usertype.create<U>();

    u["nation"]              = &U::nation;
    u["human"]               = &U::human;
    u["money"]               = &U::money;
    u["crosses"]             = &U::crosses;
    u["old_world"]           = &U::old_world;
    u["new_world_name"]      = &U::new_world_name;
    u["woodcuts"]            = &U::woodcuts;
    u["revolution_status"]   = &U::revolution_status;
    u["fathers"]             = &U::fathers;
    u["starting_position"]   = &U::starting_position;
    u["last_high_seas"]      = &U::last_high_seas;
    u["artillery_purchases"] = &U::artillery_purchases;
  };
};

} // namespace

} // namespace rn
