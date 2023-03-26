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

// C++ standard library
#include <unordered_map>

namespace lua {
struct state;
}

namespace rn {

struct IColonyViewer;
struct ICombat;
struct IGui;
struct IMapUpdater;
struct INativeMind;
struct IRand;
struct Planes;
struct RootState;
struct TerrainConnectivity;

enum class e_tribe;

/****************************************************************
** NativeMinds
*****************************************************************/
struct NativeMinds {
  NativeMinds() = default;

  // Have this defined in the cpp allows us to use the
  // forward-declared INativeMInd in a unique_ptr.
  ~NativeMinds();

  NativeMinds(
      std::unordered_map<e_tribe, std::unique_ptr<INativeMind>>
          minds );

  INativeMind& operator[]( e_tribe tribe ) const;

 private:
  // We don't use enum map here because it has some constraints
  // that don't work with forward-declared enums.
  std::unordered_map<e_tribe, std::unique_ptr<INativeMind>>
      minds_;
};

/****************************************************************
** TS
*****************************************************************/
struct TS {
  TS( Planes& planes, IMapUpdater& map_updater_,
      lua::state& lua_, IGui& gui_, IRand& rand_,
      ICombat& combat, IColonyViewer& colony_viewer,
      RootState& saved, TerrainConnectivity& connectivity,
      NativeMinds& native_minds );

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

  NativeMinds& native_minds;

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
