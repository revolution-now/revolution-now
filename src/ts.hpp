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

struct IColonyViewer;
struct ICombat;
struct IGui;
struct IMapUpdater;
struct IRand;
struct Planes;
struct RootState;
struct TerrainConnectivity;

/****************************************************************
** TS
*****************************************************************/
struct TS {
  TS( Planes& planes, IMapUpdater& map_updater_,
      lua::state& lua_, IGui& gui_, IRand& rand_,
      ICombat& combat, IColonyViewer& colony_viewer,
      RootState& saved, TerrainConnectivity& connectivity );

  ~TS();

  TS( TS&& );

  Planes&        planes;
  IMapUpdater&   map_updater;
  lua::state&    lua;
  IGui&          gui;
  IRand&         rand;
  ICombat&       combat;
  IColonyViewer& colony_viewer;
  // This refers to a serialized state data structure that holds
  // the game state as it was when the game was most recently
  // saved or loaded. It is used to determine if the game needs
  // to be saved when the player tries to exit.
  RootState& saved;

  TerrainConnectivity& connectivity;

  // Builder style.
  TS with_gui( IGui& new_gui );

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
