/****************************************************************
**plane-ctrl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-26.
*
* Description: Manages ordering and enablement of planes.
*
*****************************************************************/
#include "plane-ctrl.hpp"

// Revolution Now
#include "plane.hpp"

// C++ standard library
#include <stack>

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Globals
*****************************************************************/
stack<e_plane_config> g_plane_config_stack;

/****************************************************************
** Helpers
*****************************************************************/
vector<e_plane> const& planes_from_config(
    e_plane_config config ) {
  switch( config ) {
    case e_plane_config::main_menu: {
      static vector<e_plane> const planes{
          e_plane::main_menu, //
          e_plane::window,    //
      };
      return planes;
    }
    case e_plane_config::land_view: {
      static vector<e_plane> const planes{
          e_plane::land_view, //
          e_plane::panel,     //
          e_plane::menu,      //
          e_plane::window,    //
      };
      return planes;
    }
    case e_plane_config::colony: {
      static vector<e_plane> const planes{
          e_plane::colony, //
          e_plane::window, //
      };
      return planes;
    }
    case e_plane_config::harbor: {
      static vector<e_plane> const planes{
          e_plane::harbor, //
          e_plane::menu,   //
          e_plane::window, //
      };
      return planes;
    }
    case e_plane_config::black: {
      static vector<e_plane> const planes{
          e_plane::window, //
      };
      return planes;
    }
    case e_plane_config::map_editor: {
      static vector<e_plane> const planes{
          e_plane::map_edit, //
          e_plane::menu,     //
          e_plane::window,   //
      };
      return planes;
    }
  }
}

void update_planes() {
  if( g_plane_config_stack.empty() ) {
    set_plane_list( {} );
    return;
  }
  set_plane_list(
      planes_from_config( g_plane_config_stack.top() ) );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
ScopedPlanePush::ScopedPlanePush( e_plane_config config ) {
  g_plane_config_stack.push( config );
  update_planes();
}

ScopedPlanePush::~ScopedPlanePush() noexcept {
  CHECK( g_plane_config_stack.size() > 0 );
  g_plane_config_stack.pop();
  update_planes();
}

} // namespace rn
