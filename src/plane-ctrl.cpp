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
#include "fb.hpp"
#include "init.hpp"
#include "macros.hpp"
#include "menu.hpp"
#include "plane.hpp"

// Flatbuffers
#include "fb/sg-plane_generated.h"

using namespace std;

namespace rn {

namespace {

vector<e_plane> const default_in_game_config{
    e_plane::viewport, //
    e_plane::panel,    //
    e_plane::menu,     //
    e_plane::window    //
};

vector<e_plane> const g_main_menu_config{
    e_plane::main_menu, //
    e_plane::window,    //
};

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Plane ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( Plane,
  ( vector<e_plane>, planes         ),
  ( vector<e_plane>, in_game_planes ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

private:
  SAVEGAME_FRIENDS( Plane );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    if( planes.empty() ) planes = g_main_menu_config;

    if( in_game_planes.empty() )
      in_game_planes = default_in_game_config;

    return xp_success_t{};
  }
};
SAVEGAME_IMPL( Plane );

/****************************************************************
** Helpers
*****************************************************************/
void set_planes() {
  vector<e_plane> res = SG().planes;
  // Should be last.
  res.push_back( e_plane::console );
  // This should be the only place where this is called.
  set_plane_list( res );
}

/****************************************************************
** Init / Cleanup
*****************************************************************/
void init_plane_config() {
  set_plane_config( e_plane_config::main_menu );
}

void cleanup_plane_config() {}

//
REGISTER_INIT_ROUTINE( plane_config );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void set_plane_config( e_plane_config conf ) {
  switch( conf ) {
    case +e_plane_config::main_menu:
      SG().planes = g_main_menu_config;
      break;
    case +e_plane_config::play_game:
      SG().planes = SG().in_game_planes;
      break;
    case +e_plane_config::terrain_view:
      SG().in_game_planes = vector<e_plane>{
          e_plane::viewport, //
          e_plane::panel,    //
          e_plane::menu,     //
          e_plane::window    //
      };
      SG().planes = SG().in_game_planes;
      break;
    case +e_plane_config::europe:
      SG().in_game_planes = vector<e_plane>{
          e_plane::europe, //
          e_plane::menu,   //
          e_plane::window  //
      };
      SG().planes = SG().in_game_planes;
      break;
  }
  CHECK( !SG().planes.empty() );
  set_planes();
}

/****************************************************************
** Menu Handlers
*****************************************************************/
// FIXME: these should be conditioned on some global serialized
// state.
MENU_ITEM_HANDLER(
    e_menu_item::europort_view,
    [] { set_plane_config( e_plane_config::europe ); },
    [] { return !is_plane_enabled( e_plane::europe ); } )

MENU_ITEM_HANDLER(
    e_menu_item::europort_close,
    [] { set_plane_config( e_plane_config::terrain_view ); },
    [] { return is_plane_enabled( e_plane::europe ); } )

/****************************************************************
** Testing
*****************************************************************/
void test_plane_ctrl() {
  //
}

} // namespace rn
