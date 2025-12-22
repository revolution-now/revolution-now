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
#include "agents.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "colony-view.hpp"
#include "color-cycle.hpp"
#include "combat.hpp"
#include "connectivity.hpp"
#include "console.hpp"
#include "create-game.hpp"
#include "difficulty-screen.hpp"
#include "frame-count.hpp" // FIXME
#include "game-setup.hpp"
#include "iagent.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "imenu-server.hpp"
#include "inative-agent.hpp"
#include "interrupts.hpp"
#include "irand.hpp"
#include "land-view.hpp"
#include "lua.hpp"
#include "map-updater.hpp"
#include "panel-plane.hpp"
#include "plane-stack.hpp"
#include "rcl-game-storage.hpp" // FIXME: temporary
#include "save-game.hpp"
#include "terminal.hpp" // FIXME
#include "ts.hpp"
#include "turn.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/root.hpp"

// render
#include "render/renderer.hpp"

// luapp
#include "luapp/enum.hpp" // FIXME
#include "luapp/ext-refl.hpp"
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/no-discard.hpp"
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

using namespace std;

namespace rn {

namespace {

// TODO: temporary
e_player ensure_human_player( PlayersState const& players ) {
  for( auto& [player_type, player] : players.players )
    if( player.has_value() )
      if( player->control == e_player_control::human )
        return player_type;
  FATAL( "there must be at least player under human control" );
}

wait<> persistent_msg_box( IGui& gui, string_view const msg ) {
  while( true ) co_await gui.message_box( string( msg ) );
}

wait_bool create_from_setup( SS& ss, IGui& gui, IRand& rand,
                             lua::state& lua,
                             GameSetup const& setup ) {
  wait<> const generating_msg = persistent_msg_box(
      gui, "Generating game... please wait." );
  co_await 1_frames;
  // TODO: we could consider putting the below function in a
  // sandbox of some kind and then running it in its own thread
  // so that we don't block the main thread.
  if( auto const ok =
          create_game_from_setup( ss, rand, lua, setup );
      !ok ) {
    co_await gui.message_box( "Failed to create game: {}",
                              ok.error() );
    co_return false;
  }
  // NOTE: this takes ~100-200ms for a normal map size on a
  // fast machine.
  CHECK_HAS_VALUE( ss.as_const.validate_full_game_state() );
  co_return true;
}

wait_bool new_game_new_world( IEngine& engine, Planes& planes,
                              SS& ss, IGui& gui, RootState&,
                              lua::state& lua ) {
  auto const setup = co_await create_default_game_setup(
      engine, planes, gui, engine.rand(), lua );
  if( !setup.has_value() ) co_return false;
  co_return co_await create_from_setup( ss, gui, engine.rand(),
                                        lua, *setup );
}

wait_bool new_game_america( IEngine& engine, Planes& planes,
                            SS& ss, IGui& gui, RootState&,
                            lua::state& lua ) {
  auto const setup = co_await create_america_game_setup(
      engine, planes, gui, engine.rand(), lua );
  if( !setup.has_value() ) co_return false;
  co_return co_await create_from_setup( ss, gui, engine.rand(),
                                        lua, *setup );
}

wait_bool customize_new_world( IEngine& engine, Planes& planes,
                               SS& ss, IGui& gui, RootState&,
                               lua::state& lua ) {
  EnumChoiceConfig const config{
    .msg = "Select Desired Customization Level:",
  };
  auto const mode =
      co_await gui.optional_enum_choice<e_customization_mode>(
          config );
  if( !mode.has_value() ) co_return false;
  auto const setup = co_await create_customized_game_setup(
      engine, planes, gui, *mode );
  if( !setup.has_value() ) co_return false;
  co_return co_await create_from_setup( ss, gui, engine.rand(),
                                        lua, *setup );
}

using LoaderFunc = base::function_ref<wait_bool(
    IEngine&, Planes&, SS& ss, IGui& gui, RootState& saved,
    lua::state& lua )>;

wait<> run_game( IEngine& engine, Planes& planes, IGui& gui,
                 LoaderFunc const loader ) {
  // This is the entire (serializable) state representing a game.
  SS ss;
  // This will hold the state of the game the last time it was
  // saved (not including auto-save) and/or loaded.
  RootState saved;

  RealCombat combat( ss, engine.rand() );
  ColonyViewer colony_viewer( engine, ss );
  TerrainConnectivity connectivity;

  NonRenderingMapUpdater non_rendering_map_updater(
      ss, connectivity );

  // This is the lua state that will be operative throughout the
  // duration of this particular game.
  lua::state st;

  st["ROOT"] = ss.root;
  st["SS"]   = ss;
  st["IMapUpdater"] =
      static_cast<IMapUpdater&>( non_rendering_map_updater );
  st["IRand"] = static_cast<IRand&>( engine.rand() );

  // Do this after we set globals so that they will be included
  // in the freezing.
  lua_init( st );

  if( !co_await loader( engine, planes, ss, gui, saved, st ) )
    // Didn't load a game for some reason. Could have failed or
    // maybe there are no games to load.
    co_return;

  TS ts( planes, gui, combat, colony_viewer, saved );

  NativeAgents native_agents =
      create_native_agents( ss, engine.rand() );
  auto _1 = ts.set_native_agents( native_agents );

  // This one needs to run after the loader because it needs to
  // know which nations are human.
  Agents agents =
      create_agents( engine, ss, non_rendering_map_updater,
                     planes, gui, engine.rand() );
  auto _2 = ts.set_agents( agents );

  rr::Renderer& renderer =
      engine.renderer_use_only_when_needed();

  RenderingMapUpdater map_updater(
      ss, connectivity, renderer,
      MapUpdaterOptions{
        .render_fog_of_war =
            ss.settings.in_game_options.game_menu_options
                [e_game_menu_option::show_fog_of_war] } );

  auto _3 = ts.set_map_updater( map_updater );

  // This has likely already been done as a part of game set up
  // (e.g. initial unit placement), but let's just make sure it
  // has up front to avoid any surprises later.
  map_updater.connectivity();
  CHECK( !connectivity.indices.empty() );

  // Start the background coroutine that runs the color-cycling
  // animations on the map. Note that the enabled flag can change
  // as the game progresses (it can be changed by the user in the
  // game options UI). Thus it 1) must be a reference, and 2)
  // must refer to a boolean that will be stationary in memory.
  bool const& cycling_enabled =
      ss.settings.in_game_options.game_menu_options
          [e_game_menu_option::water_color_cycling];
  wait<> const cycling_thread =
      cycle_map_colors_thread( renderer, gui, cycling_enabled );

  // TODO: temporary
  ensure_human_player( ss.players );

  // We could create a new terminal object here which would clear
  // the history, but there doesn't seem to be a good reason to
  // do that, so just provide continuity. Note that we are using
  // a new Lua state though for the duration of this particular
  // new or loaded game.
  Terminal& terminal = planes.get().console.typed().terminal();

  ConsolePlane console_plane( engine, terminal, st );
  LandViewPlane land_view_plane( engine, ss, ts,
                                 /*visibility=*/nothing );
  PanelPlane panel_plane( engine, ss, ts, land_view_plane );

  auto owner          = planes.push();
  PlaneGroup& group   = owner.group;
  group.menus_enabled = true;
  group.menu.typed().enable_cheat_menu(
      ss.settings.cheat_options.enabled );
  group.panel   = panel_plane;
  group.console = console_plane;
  group.set_bottom<ILandViewPlane>( land_view_plane );

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
  ts.map_updater().redraw();
  // This is so that when the game is exited the terrain buffers
  // won't continue to render in the background.
  SCOPE_EXIT { ts.map_updater().unrender(); };

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
  auto const loop = [&] { return turn_loop( engine, ss, ts ); };
  co_await co::erase( co::try_<game_quit_interrupt>( loop ) );
}

wait<> handle_mode( IEngine& engine, Planes& planes, IGui& gui,
                    StartMode::new_random const& ) {
  co_await run_game( engine, planes, gui, new_game_new_world );
}

wait<> handle_mode( IEngine& engine, Planes& planes, IGui& gui,
                    StartMode::new_america const& ) {
  co_await run_game( engine, planes, gui, new_game_america );
}

wait<> handle_mode( IEngine& engine, Planes& planes, IGui& gui,
                    StartMode::new_customize const& ) {
  co_await run_game( engine, planes, gui, customize_new_world );
}

wait<> handle_mode( IEngine& engine, Planes& planes, IGui& gui,
                    StartMode::load const& load ) {
  co_await run_game(
      engine, planes, gui,
      [&]( IEngine&, Planes&, SS& ss, IGui& gui,
           RootState& saved, lua::state& ) -> wait_bool {
        maybe<int> slot = load.slot;
        if( !slot.has_value() ) {
          // Pop open the load-game box to let the player choose
          // what they want to load.
          slot = co_await select_load_slot(
              gui, RclGameStorageQuery() );
          if( !slot.has_value() )
            // The player has cancelled, or the load did not
            // succeed for some other reason.
            co_return false;
        }
        CHECK( slot.has_value() );
        co_return co_await load_from_slot_interactive(
            ss, gui, RclGameStorageLoad( ss ), saved, *slot );
      } );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
wait<> run_game_with_mode( IEngine& engine, Planes& planes,
                           IGui& gui, StartMode const& mode ) {
  StartMode next_mode = mode;
  while( true ) {
    try {
      co_await visit(
          [&]( auto& m ) {
            return handle_mode( engine, planes, gui, m );
          },
          next_mode.as_base() );
      break;
    } catch( game_load_interrupt const& load ) {
      next_mode = StartMode::load{ .slot = load.slot };
    }
  }
}

} // namespace rn
