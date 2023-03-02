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
#include "colony-view.hpp"
#include "combat.hpp"
#include "conductor.hpp"
#include "connectivity.hpp"
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

e_nation ensure_human_player( PlayersState const& players ) {
  CHECK( players.human.has_value(),
         "there must be a human player." );
  return *players.human;
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

using LoaderFunc =
    base::function_ref<wait<base::NoDiscard<bool>>( SS& ss,
                                                    TS& ts )>;

wait<> run_game( Planes& planes, LoaderFunc loader ) {
  // This is the entire (serializable) state representing a game.
  SS ss;
  // This will hold the state of the game the last time it was
  // saved (not including auto-save) and/or loaded.
  RootState saved;

  lua::state& st = planes.console().lua_state();
  st["ROOT"]     = ss.root;
  st["SS"]       = ss;

  auto        popper = planes.new_copied_group();
  PlaneGroup& group  = planes.back();

  WindowPlane window_plane;
  group.window = &window_plane;

  RealGui gui( window_plane );

  Rand         rand; // random seed.
  RealCombat   combat( ss, rand );
  ColonyViewer colony_viewer( ss );

  TerrainConnectivity connectivity;

  {
    // The real map updater needs to know the map size during
    // construction, so use the non-rendering one, which is fine
    // because we don't need to render yet anyway.
    NonRenderingMapUpdater map_updater( ss );
    TS ts( planes, map_updater, st, gui, rand, combat,
           colony_viewer, saved, connectivity );
    if( !co_await loader( ss, ts ) )
      // Didn't load a game for some reason. Could have failed or
      // maybe there are no games to load.
      co_return;
  }

  // After this, any changes to the map that change land to water
  // or vice versa (or change map size) need to be followed up by
  // a call to re-compute the terrain connectivity. In practice,
  // this could only be done by opening the map editor mid-game.
  connectivity = compute_terrain_connectivity( ss );
  CHECK( !connectivity.indices.empty() );

  RenderingMapUpdater map_updater(
      ss, global_renderer_use_only_when_needed() );
  TS ts( planes, map_updater, st, gui, rand, combat,
         colony_viewer, saved, connectivity );

  ensure_human_player( ss.players );

  MenuPlane menu_plane;
  group.menu = &menu_plane;

  PanelPlane panel_plane( ss, ts );
  group.panel = &panel_plane;

  LandViewPlane land_view_plane( ss, ts,
                                 /*visibility=*/nothing );
  group.land_view = &land_view_plane;

  // Perform the initial rendering of the map. Even though it
  // will be wasteful in a sense, we will render the entire map
  // (with all tiles visible), in order to catch any rendering
  // issues up front, including just making sure there is enough
  // memory and GPU memory to whole the fully rendered map. This
  // is wasteful in the sense that, when the first player takes
  // there turn, this rendering will be thrown out and the play-
  // er's view will be drawn; however, it is prudent to do this
  // up front as opposed to waiting for the problematic tile (or
  // exposed map size) to be encountered by the player and have
  // the game crash mid-play. This will render the entire map be-
  // cause that is the default setting of the map updater.
  lg.info( "performing initial full map render." );
  ts.map_updater.redraw();

  play( rand, e_game_module_tune_points::start_game );
  // All of the above needs to stay alive, so we must wait.
  //
  // TODO: if this is a new game then Lua may have placed some
  // units on the map.  Just before we start the turn loop we
  // need to call the interactive on-map function for each of
  // those units.  This will ensure that the game invariants
  // are upheld, e.g.:
  //
  //   * A ship starting off next to land will have discovered
  //     the new world.
  //   * A unit next to a native entity will have met the tribe
  //     and created a relationship.
  //   * A unit on an LCR will have investigated it.
  //   * ...
  //
  // But this should not be done when loading an existing game.
  co_await co::erase( co::try_<game_quit_interrupt>(
      [&] { return turn_loop( ss, ts ); } ) );
}

wait<> handle_mode( Planes& planes, StartMode::new_ const& ) {
  auto factory = []( SS&,
                     TS& ts ) -> wait<base::NoDiscard<bool>> {
    CHECK_HAS_VALUE( ts.lua["new_game"]["create"].pcall() );
    co_return true;
  };
  co_await run_game( planes, factory );
}

wait<> handle_mode( Planes&                planes,
                    StartMode::load const& load ) {
  auto factory = [&]( SS& ss,
                      TS& ts ) -> wait<base::NoDiscard<bool>> {
    maybe<int> slot = load.slot;
    if( !slot.has_value() ) {
      // Pop open the load-game box to let the player choose what
      // they want to load.
      slot = co_await choose_load_slot( ts );
      if( !slot.has_value() )
        // The player has cancelled, or the load did not succeed
        // for some other reason.
        co_return false;
    }
    CHECK( slot.has_value() );
    co_return co_await load_slot( ss, ts, *slot );
  };
  co_await run_game( planes, factory );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_game_with_mode( Planes&          planes,
                           StartMode const& mode ) {
  StartMode next_mode = mode;
  while( true ) {
    try {
      co_await base::visit(
          [&]( auto& m ) { return handle_mode( planes, m ); },
          next_mode.as_base() );
      break;
    } catch( game_load_interrupt const& load ) {
      next_mode = StartMode::load{ .slot = load.slot };
    }
  }
}

} // namespace rn
