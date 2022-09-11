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
#include "co-wait.hpp"
#include "conductor.hpp"
#include "console.hpp"
#include "gui.hpp"
#include "interrupts.hpp"
#include "irand.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "map-updater.hpp"
#include "menu.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "rand.hpp"
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

void play( IRand& rand, e_game_module_tune_points tune ) {
  switch( tune ) {
    case e_game_module_tune_points::start_game:
      conductor::play_request(
          rand, conductor::e_request::fife_drum_happy,
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

  auto        popper = planes.new_copied_group();
  PlaneGroup& group  = planes.back();

  WindowPlane window_plane;
  group.window = &window_plane;

  RealGui gui( window_plane );

  Rand rand; // random seed.

  TS ts( map_updater, st, gui, rand );

  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  loader( ss, st );

  map_updater.redraw();
  ensure_human_player( ss.players );

  MenuPlane menu_plane;
  group.menu = &menu_plane;

  PanelPlane panel_plane( planes, ss, ts );
  group.panel = &panel_plane;

  LandViewPlane land_view_plane( planes, ss, ts );
  group.land_view = &land_view_plane;

  // land_view_plane.zoom_out_full();

  play( rand, e_game_module_tune_points::start_game );
  // All of the above needs to stay alive, so we must wait.
  co_await co::erase( co::try_<game_quit_interrupt>(
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
