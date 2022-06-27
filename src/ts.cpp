/****************************************************************
**ts.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-25.
*
* Description: Non-serialized (transient) game state.
*
*****************************************************************/
#include "ts.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace rn {

/****************************************************************
** TS
*****************************************************************/
OmniPlane& TS::omni_plane_or_die() const {
  UNWRAP_CHECK( p, omni_plane );
  return p;
}

ConsolePlane& TS::console_plane_or_die() const {
  UNWRAP_CHECK( p, console_plane );
  return p;
}

WindowPlane& TS::window_plane_or_die() const {
  UNWRAP_CHECK( p, window_plane );
  return p;
}

MenuPlane& TS::menu_plane_or_die() const {
  UNWRAP_CHECK( p, menu_plane );
  return p;
}

PanelPlane& TS::panel_plane_or_die() const {
  UNWRAP_CHECK( p, panel_plane );
  return p;
}

LandViewPlane& TS::land_view_plane_or_die() const {
  UNWRAP_CHECK( p, land_view_plane );
  return p;
}

TS TS::with_new( ConsolePlane& p ) const {
  return TS{
      .planes          = planes,          //
      .map_updater     = map_updater,     //
      .lua_state       = lua_state,       //
      .gui             = gui,             //
      .omni_plane      = omni_plane,      //
      .console_plane   = p,               //
      .window_plane    = window_plane,    //
      .menu_plane      = menu_plane,      //
      .panel_plane     = panel_plane,     //
      .land_view_plane = land_view_plane, //
  };
}

TS TS::with_new( WindowPlane& p, IGui& g ) const {
  return TS{
      .planes          = planes,          //
      .map_updater     = map_updater,     //
      .lua_state       = lua_state,       //
      .gui             = g,               //
      .omni_plane      = omni_plane,      //
      .console_plane   = console_plane,   //
      .window_plane    = p,               //
      .menu_plane      = menu_plane,      //
      .panel_plane     = panel_plane,     //
      .land_view_plane = land_view_plane, //
  };
}

TS TS::with_new( MenuPlane& p ) const {
  return TS{
      .planes          = planes,          //
      .map_updater     = map_updater,     //
      .lua_state       = lua_state,       //
      .gui             = gui,             //
      .omni_plane      = omni_plane,      //
      .console_plane   = console_plane,   //
      .window_plane    = window_plane,    //
      .menu_plane      = p,               //
      .panel_plane     = panel_plane,     //
      .land_view_plane = land_view_plane, //
  };
}

TS TS::with_new( PanelPlane& p ) const {
  return TS{
      .planes          = planes,          //
      .map_updater     = map_updater,     //
      .lua_state       = lua_state,       //
      .gui             = gui,             //
      .omni_plane      = omni_plane,      //
      .console_plane   = console_plane,   //
      .window_plane    = window_plane,    //
      .menu_plane      = menu_plane,      //
      .panel_plane     = p,               //
      .land_view_plane = land_view_plane, //
  };
}

TS TS::with_new( LandViewPlane& p ) const {
  return TS{
      .planes          = planes,        //
      .map_updater     = map_updater,   //
      .lua_state       = lua_state,     //
      .gui             = gui,           //
      .omni_plane      = omni_plane,    //
      .console_plane   = console_plane, //
      .window_plane    = window_plane,  //
      .menu_plane      = menu_plane,    //
      .panel_plane     = panel_plane,   //
      .land_view_plane = p,             //
  };
}

} // namespace rn
