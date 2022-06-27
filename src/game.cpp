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
#include "gui.hpp"
#include "interrupts.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "map-updater.hpp"
#include "menu.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "renderer.hpp" // FIXME: remove
#include "save-game.hpp"
#include "ts.hpp"
#include "turn.hpp"
#include "window.hpp"

// gs
#include "ss/ref.hpp"
#include "ss/root.hpp"

// luapp
#include "luapp/state.hpp"

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

wait<> load_and_run_game(
    Planes&                                            planes,
    base::function_ref<void( SS& ss, lua::state& st )> loader ) {
  SS         ss;
  MapUpdater map_updater(
      ss.mutable_terrain_use_with_care,
      global_renderer_use_only_when_needed() );
  map_updater.redraw();
  ensure_human_player( ss.players );

  WindowPlane   window_plane;
  RealGui       gui( window_plane );
  MenuPlane     menu_plane;
  LandViewPlane land_view_plane( menu_plane, window_plane,
                                 ss.land_view, ss.terrain,
                                 map_updater, gui );
  PanelPlane    panel_plane( menu_plane );

  auto        popper = planes.new_group();
  PlaneGroup& group  = planes.back();
  group.push( land_view_plane );
  group.push( panel_plane );
  group.push( menu_plane );
  group.push( window_plane );

  // land_view_plane.zoom_out_full();

  lua::state st;
  lua_init( st );
  loader( ss, st );
  // FIXME: need to deal with frozen globals.
  st["ROOT"] = ss.root;

  // TODO: give lua access to the renderer and map_updater as
  // well. That should then allow getting rid of all global state
  // completely. Put the lua definitions for those types in sepa-
  // rate files.

  TS ts{
      .planes          = planes,
      .map_updater     = map_updater,
      .lua_state       = st,
      .gui             = gui,
      .window_plane    = window_plane,
      .menu_plane      = menu_plane,
      .panel_plane     = panel_plane,
      .land_view_plane = land_view_plane,
  };

  play( e_game_module_tune_points::start_game );
  return co::erase( co::try_<game_quit_interrupt>(
      [&] { return turn_loop( ss, ts ); } ) );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_existing_game( Planes& planes ) {
  co_await load_and_run_game( planes, []( SS& ss, lua::state& ) {
    CHECK_HAS_VALUE( load_game( ss.root, 0 ) );
  } );
}

wait<> run_new_game( Planes& planes ) {
  co_await load_and_run_game( planes, []( SS&, lua::state& st ) {
    CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );
  } );
}

} // namespace rn
