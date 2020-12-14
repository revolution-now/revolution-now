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
#include "logging.hpp"
#include "macros.hpp"
#include "menu.hpp"
#include "plane.hpp"
#include "sg-macros.hpp"

// Flatbuffers
#include "fb/sg-plane_generated.h"

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Plane );

namespace {

struct PlaneList {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  bool operator==( PlaneList const& rhs ) const {
    return plane_list == rhs.plane_list;
  }

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, PlaneList,
  ( vector<e_plane>, plane_list ));
  // clang-format on
};

PlaneList const g_main_menu_config{ {
    e_plane::main_menu, //
    e_plane::window,    //
} };

void set_planes();

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Plane ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( Plane,
  ( vector<PlaneList>, plane_list_stack ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

private:
  SAVEGAME_FRIENDS( Plane );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    if( plane_list_stack.empty() )
      plane_list_stack.push_back( g_main_menu_config );

    set_planes();
    return xp_success_t{};
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return xp_success_t{}; }
};
SAVEGAME_IMPL( Plane );

/****************************************************************
** Helpers
*****************************************************************/
void set_planes() {
  CHECK( !SG().plane_list_stack.empty() );
  vector<e_plane> res = SG().plane_list_stack.back().plane_list;
  // Should be last.
  res.push_back( e_plane::console );
  // This should be the only place where this is called.
  set_plane_list( res );
}

/****************************************************************
** Init / Cleanup
*****************************************************************/
void init_plane_config() {
  push_plane_config( e_plane_config::main_menu );
}

void cleanup_plane_config() {
  if( SG().plane_list_stack.size() > 5 )
    lg.warn(
        "cleaning up whilst there are {} plane lists still in "
        "the stack.",
        SG().plane_list_stack.size() );
}

REGISTER_INIT_ROUTINE( plane_config );

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void push_plane_config( e_plane_config conf ) {
  switch( conf ) {
    case e_plane_config::main_menu:
      SG().plane_list_stack.push_back( g_main_menu_config );
      break;
    case e_plane_config::terrain:
      SG().plane_list_stack.push_back( PlaneList{ {
          e_plane::land_view, //
          e_plane::panel,     //
          e_plane::menu,      //
          e_plane::window     //
      } } );
      break;
    case e_plane_config::colony:
      SG().plane_list_stack.push_back( PlaneList{ {
          e_plane::colony, //
          // e_plane::menu,   //
          e_plane::window //
      } } );
      break;
    case e_plane_config::europe:
      SG().plane_list_stack.push_back( PlaneList{ {
          e_plane::europe, //
          e_plane::menu,   //
          e_plane::window  //
      } } );
      break;
  }
  set_planes();
}

void pop_plane_config() {
  CHECK( SG().plane_list_stack.size() > 1 );
  SG().plane_list_stack.pop_back();
  set_planes();
}

void clear_plane_stack() { SG().plane_list_stack.clear(); }

} // namespace rn
