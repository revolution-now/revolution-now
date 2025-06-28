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

// luapp
#include "luapp/ext-userdata.hpp"

namespace lua {
struct state;
}

namespace rn {

struct EuroAgents;
struct IColonyViewer;
struct ICombat;
struct IGui;
struct IMapUpdater;
struct IRand;
struct NativeAgents;
struct Planes;
struct RootState;
struct TerrainConnectivity;

template<typename T>
struct [[nodiscard]] set_and_restore_pointer {
  set_and_restore_pointer( T** pp, T* p )
    : pp_( pp ), old_( std::exchange( *pp, p ) ) {}

  ~set_and_restore_pointer() noexcept { *pp_ = old_; }

 private:
  T** pp_;
  T* old_;
};

#define TS_FIELD( type, var )                      \
                                                   \
 public:                                           \
  type& var() const {                              \
    CHECK( var##_ );                               \
    return *var##_;                                \
  }                                                \
  auto set_##var( type& p ) {                      \
    return set_and_restore_pointer( &var##_, &p ); \
  }                                                \
  auto set_##var##_no_restore( type& p ) {         \
    return var##_ = &p;                            \
  }                                                \
                                                   \
 private:                                          \
  type* var##_ = nullptr

/****************************************************************
** TS
*****************************************************************/
struct TS {
  TS( Planes& planes, lua::state& lua_, IGui& gui_, IRand& rand_,
      ICombat& combat, IColonyViewer& colony_viewer,
      RootState& saved, TerrainConnectivity& connectivity );

  ~TS();

  TS( TS&& ) = delete;

  Planes& planes;
  lua::state& lua;
  IGui& gui;
  IRand& rand;
  ICombat& combat;
  IColonyViewer& colony_viewer;

  TS_FIELD( IMapUpdater, map_updater );
  TS_FIELD( NativeAgents, native_agents );
  TS_FIELD( EuroAgents, euro_agents );

 public:
  // This refers to a serialized state data structure that holds
  // the game state as it was when the game was most recently
  // saved or loaded. It is used to determine if the game needs
  // to be saved when the player tries to exit.
  RootState& saved;

  TerrainConnectivity& connectivity;

 private:
  struct LuaRefSetAndRestore;
  std::unique_ptr<LuaRefSetAndRestore> pimpl_;
};

void to_str( TS const& o, std::string& out, base::tag<TS> );

} // namespace rn

#undef TS_FIELD

/****************************************************************
** Lua
*****************************************************************/
namespace lua {

LUA_USERDATA_TRAITS( ::rn::TS, owned_by_cpp ){};

}
