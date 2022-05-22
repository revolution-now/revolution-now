/****************************************************************
**save-game.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-15.
*
* Description: Unit tests for the src/save-game.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/save-game.hpp"

// Revolution Now
#include "src/game-state.hpp"
#include "src/gs-top.hpp"
#include "src/lua.hpp"
#include "src/rand.hpp"

// luapp
#include "luapp/state.hpp"

// base
#include "base/io.hpp"
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::testing::data_dir;

// Change this to 1 and then run the tests to regenerate the
// files, but then make sure to change it back to zero!
#define REGENERATE_FILES 0 // !!! DO NOT COMMIT

// Currently these are only implemented in release mode because
// they are slow, since the world is large. TODO: once we can
// generate a test world with smaller size we should enable this
// in debug mode.
#ifdef NDEBUG

void print_line( string_view what ) {
  (void)what;
#  if 0
  fmt::print( "---------- {} ----------\n", what );
#  endif
}

void reset_seeds() {
  rng::reseed( 0 );
  lua::state& st = lua_global_state();
  st["math"]["randomseed"]( 0 );
}

void generate_save_file( fs::path const&        dst,
                         SaveGameOptions const& options ) {
  default_construct_game_state();
  reset_seeds();
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  lua::state&            st = lua_global_state();
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );
  REQUIRE( save_game_to_rcl_file( dst, options ) );
}

TEST_CASE( "[save-game] no default values (compact)" ) {
  static fs::path const src =
      data_dir() / "saves/compact.sav.rcl";

  static SaveGameOptions const opts{
      .verbosity = e_savegame_verbosity::compact,
  };

  NonRenderingMapUpdater map_updater( GameState::terrain() );

#  if REGENERATE_FILES
  generate_save_file( src, opts );
#  else
  (void)generate_save_file;
#  endif

  CHECK( fs::exists( src ) );

  // FIXME: find a better way to get a random temp folder.
  static fs::path const dst = "/tmp/test-compact.sav.rcl";
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );

  // Make a round trip.
  print_line( "Load Compact" );
  REQUIRE( load_game_from_rcl_file( map_updater, src, opts ) );
  print_line( "Save Compact" );
  REQUIRE( save_game_to_rcl_file( dst, opts ) );

  UNWRAP_CHECK( src_text,
                base::read_text_file_as_string( src ) );
  UNWRAP_CHECK( dst_text,
                base::read_text_file_as_string( dst ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( src_text == dst_text ) );
}

TEST_CASE( "[save-game] default values (full)" ) {
  static fs::path const src = data_dir() / "saves/full.sav.rcl";

  static SaveGameOptions const opts{
      .verbosity = e_savegame_verbosity::full,
  };

  NonRenderingMapUpdater map_updater( GameState::terrain() );

#  if REGENERATE_FILES
  generate_save_file( src, opts );
#  else
  (void)generate_save_file;
#  endif

  CHECK( fs::exists( src ) );

  // FIXME: find a better way to get a random temp folder.
  static fs::path const dst = "/tmp/test-full.sav.rcl";
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );

  // Make a round trip.
  print_line( "Load Full" );
  REQUIRE( load_game_from_rcl_file( map_updater, src, opts ) );
  print_line( "Save Full" );
  REQUIRE( save_game_to_rcl_file( dst, opts ) );

  UNWRAP_CHECK( src_text,
                base::read_text_file_as_string( src ) );
  UNWRAP_CHECK( dst_text,
                base::read_text_file_as_string( dst ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( src_text == dst_text ) );
}

TEST_CASE( "[save-game] world gen with default values (full)" ) {
  lua::state& st = lua_global_state();
  default_construct_game_state();
  reset_seeds();
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );
  TopLevelState backup = std::move( GameState::top() );
  default_construct_game_state();
  reset_seeds();
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );

  // FIXME: find a better way to get a random temp folder.
  static fs::path const dst = "/tmp/test-world-gen-full.sav.rcl";
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );

  SaveGameOptions opts{
      .verbosity = e_savegame_verbosity::full,
  };

  // Make a round trip.
  print_line( "Save Gen" );
  REQUIRE( save_game_to_rcl_file( dst, opts ) );
  print_line( "Load Gen" );
  REQUIRE( load_game_from_rcl_file( map_updater, dst, opts ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( backup == GameState::top() ) );
}

TEST_CASE(
    "[save-game] world gen with no default values (compact)" ) {
  lua::state& st = lua_global_state();
  default_construct_game_state();
  reset_seeds();
  NonRenderingMapUpdater map_updater( GameState::terrain() );
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );
  TopLevelState backup = std::move( GameState::top() );
  default_construct_game_state();
  reset_seeds();
  CHECK_HAS_VALUE( st["new_game"]["create"].pcall() );

  // FIXME: find a better way to get a random temp folder.
  static fs::path const dst =
      "/tmp/test-world-gen-compact.sav.rcl";
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );

  SaveGameOptions opts{
      .verbosity = e_savegame_verbosity::compact,
  };

  // Make a round trip.
  print_line( "Save Gen" );
  REQUIRE( save_game_to_rcl_file( dst, opts ) );
  print_line( "Load Gen" );
  REQUIRE( load_game_from_rcl_file( map_updater, dst, opts ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( backup == GameState::top() ) );
}

#endif

TEST_CASE( "[save-game] no regen" ) {
  // This will flag if we forget to turn off file regeneration.
  // It may cause issues though if we turn on random test order-
  // ing.
#if REGENERATE_FILES
  REQUIRE( false );
#endif
}

} // namespace
} // namespace rn
