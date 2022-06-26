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

namespace lua {
struct state;
}

namespace rn {

struct OmniPlane;
struct ConsolePlane;
struct WindowPlane;
struct MenuPlane;
struct PanelPlane;
struct LandViewPlane;

struct Planes;
struct IMapUpdater;
struct IGui;

/****************************************************************
** TS
*****************************************************************/
struct TS {
  Planes&      planes;
  IMapUpdater& map_updater;
  lua::state&  lua_state;
  IGui&        gui;

  maybe<OmniPlane&>     omni_plane      = {};
  maybe<ConsolePlane&>  console_plane   = {};
  maybe<WindowPlane&>   window_plane    = {};
  maybe<MenuPlane&>     menu_plane      = {};
  maybe<PanelPlane&>    panel_plane     = {};
  maybe<LandViewPlane&> land_view_plane = {};

  TS with_new( ConsolePlane& p ) const;
  TS with_new( WindowPlane& p, IGui& gui ) const;
  TS with_new( MenuPlane& p ) const;
  TS with_new( PanelPlane& p ) const;
  TS with_new( LandViewPlane& p ) const;
};

} // namespace rn
