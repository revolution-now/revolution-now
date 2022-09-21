/****************************************************************
**ts.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-25.
*
* Description: Non-serialized (transient) game state.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "maybe.hpp"

// luapp
#include "luapp/ext-userdata.hpp"

namespace lua {
struct state;
}

namespace rn {

struct Planes;
struct IMapUpdater;
struct IGui;
struct IRand;
struct RootState;

/****************************************************************
** TS
*****************************************************************/
struct TS {
  TS( IMapUpdater& map_updater_, lua::state& lua_, IGui& gui_,
      IRand& rand_, RootState& saved );

  ~TS();

  TS( TS&& );

  IMapUpdater& map_updater;
  lua::state&  lua;
  IGui&        gui;
  IRand&       rand;
  // This refers to a serialized state data structure that holds
  // the game state as it was when the game was most recently
  // saved or loaded. It is used to determine if the game needs
  // to be saved when the player tries to exit.
  RootState& saved;

 private:
  struct LuaRefSetAndRestore;
  std::unique_ptr<LuaRefSetAndRestore> pimpl_;
};

void to_str( TS const& o, std::string& out, base::ADL_t );

} // namespace rn

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::TS, owned_by_cpp ){};

}
