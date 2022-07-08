/****************************************************************
**game.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-11.
*
* Description: Overall game flow of an individual game.
*
*****************************************************************/
#include "game.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "conductor.hpp"
#include "console.hpp"
#include "gui.hpp"
#include "interrupts.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-updater-lua.hpp"
#include "map-updater.hpp"
#include "menu.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "renderer.hpp" // FIXME: remove
#include "save-game.hpp"
#include "ts.hpp"
#include "turn.hpp"
#include "window.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/root.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

enum class e_game_module_tune_points {
  start_game //
};

// TODO: temporary until we have AI.
void ensure_human_player( PlayersState const& players_state ) {
  maybe<e_nation> human_nation;
  for( auto const& [nation, player] : players_state.players ) {
    if( player.has_value() && player->human ) {
      human_nation = nation;
      break;
    }
  }
  CHECK( human_nation.has_value(),
         "there must be at least one human player." );
}

void play( e_game_module_tune_points tune ) {
  switch( tune ) {
    case e_game_module_tune_points::start_game:
      conductor::play_request(
          conductor::e_request::fife_drum_happy,
          conductor::e_request_probability::always );
      break;
  }
}

wait<> run_game(
    Planes&                                            planes,
    base::function_ref<void( SS& ss, lua::state& st )> loader ) {
  // This is the entire (serializable) state representing a game.
  SS ss;

  MapUpdater map_updater(
      ss.mutable_terrain_use_with_care,
      global_renderer_use_only_when_needed() );

  lua::state& st = planes.console().lua_state();
  loader( ss, st );
  // FIXME: need to deal with frozen globals.
  st["ROOT"] = ss.root;

  st["TS"] = st.table.create();
  st["TS"]["map_updater"] =
      static_cast<IMapUpdater&>( map_updater );

  map_updater.redraw();
  ensure_human_player( ss.players );

  WindowPlane window_plane;
  RealGui     gui( window_plane );

  TS ts{
      .planes      = planes,
      .map_updater = map_updater,
      .lua         = st,
      .gui         = gui,
  };

  MenuPlane     menu_plane;
  LandViewPlane land_view_plane( planes, ss, ts );
  PanelPlane    panel_plane( planes, ss );

  auto        popper = planes.new_copied_group();
  PlaneGroup& group  = planes.back();
  group.land_view    = &land_view_plane;
  group.panel        = &panel_plane;
  group.menu         = &menu_plane;
  group.window       = &window_plane;

  // land_view_plane.zoom_out_full();

  // TODO: give lua access to the renderer and map_updater as
  // well. That should then allow getting rid of all global state
  // completely. Put the lua definitions for those types in sepa-
  // rate files.

  play( e_game_module_tune_points::start_game );
  return co::erase( co::try_<game_quit_interrupt>(
      [&] { return turn_loop( planes, ss, ts ); } ) );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_existing_game( Planes& planes ) {
  co_await run_game( planes, []( SS& ss, lua::state& ) {
    CHECK_HAS_VALUE( load_game( ss.root, 0 ) );
  } );
}

wait<> run_new_game( Planes& planes ) {
  co_await run_game( planes, []( SS&, lua::state& st ) {
    CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );
  } );
}

} // namespace rn
