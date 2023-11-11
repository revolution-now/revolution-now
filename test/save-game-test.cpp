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
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/save-game.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/rand.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/root.hpp"

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

using ::mock::matchers::_;
using ::testing::data_dir;

// Change this to 1 and then run the tests to regenerate the
// files, but then make sure to change it back to zero!
#define REGENERATE_FILES 0 // !!! DO NOT COMMIT

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {};

/****************************************************************
** Helpers
*****************************************************************/
void print_line( string_view what ) {
  (void)what;
#if 0
  fmt::print( "---------- {} ----------\n", what );
#endif
}

void reset_seeds( lua::state& st ) {
  // rng::reseed( 0 );
  st["math"]["randomseed"]( 0 );
}

void expect_rands( World& W ) {
  // These are for choosing the immigrant pool.
  W.rand().EXPECT__between_doubles( _, _ ).times( 3 ).returns(
      0.0 );

  // These are for choosing the initial processed goods volumes.
  W.rand().EXPECT__between_ints( _, _, _ ).times( 4 ).returns(
      0x200 );

  // These are for choosing the expertise of each native
  // dwelling.
  W.rand().EXPECT__between_ints( _, _, _ ).times( 7 ).returns(
      0 );
}

void create_new_game_from_lua( World& world ) {
  lua::state& st       = world.lua();
  lua::table  new_game = st["new_game"].as<lua::table>();
  UNWRAP_CHECK(
      options, new_game["default_options"].pcall<lua::table>() );
  options["map"]["world_size"] = Delta{ .w = 8, .h = 8 };
  CHECK_HAS_VALUE(
      new_game["create"].pcall( world.root(), options ) );
}

void generate_save_file( World& world, fs::path const& dst,
                         SaveGameOptions const& options ) {
  reset_seeds( world.lua() );
  create_new_game_from_lua( world );
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );
  REQUIRE( save_game_to_rcl_file( world.ss(), dst, options ) );
}

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[save-game] no default values (compact)" ) {
  World W;
  W.expensive_run_lua_init();
  // FIXME
  W.initialize_ts();

  static fs::path const src =
      data_dir() / "saves/compact.sav.rcl";

  static SaveGameOptions const opts{
      .verbosity = e_savegame_verbosity::compact,
  };

#if REGENERATE_FILES
  expect_rands( W );
  generate_save_file( W, src, opts );
#else
  (void)generate_save_file;
#endif

  CHECK( fs::exists( src ) );
  INFO( fmt::format( "src file: {}", src ) );

  // FIXME: find a better way to get a random temp folder.
  static fs::path const dst = "/tmp/test-compact.sav.rcl";
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );

  // Make a round trip.
  print_line( "Load Compact" );
  REQUIRE( load_game_from_rcl_file( W.root(), src, opts ) );
  print_line( "Save Compact" );
  REQUIRE( save_game_to_rcl_file( W.ss(), dst, opts ) );

  UNWRAP_CHECK( src_text,
                base::read_text_file_as_string( src ) );
  UNWRAP_CHECK( dst_text,
                base::read_text_file_as_string( dst ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( src_text == dst_text ) );
}

// This test is disabled because it is not always supposed to
// pass; e.g. when we add a new field to the schema it fails, but
// that situation is normal and allowed as the schema evolves.
#if 0
TEST_CASE( "[save-game] default values (full)" ) {
  World W;
  W.expensive_run_lua_init();
  // FIXME
  W.initialize_ts();

  static fs::path const src = data_dir() / "saves/full.sav.rcl";

  static SaveGameOptions const opts{
      .verbosity = e_savegame_verbosity::full,
  };

#  if REGENERATE_FILES
  expect_rands( W );
  generate_save_file( W, src, opts );
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
  REQUIRE( load_game_from_rcl_file( W.root(), src, opts ) );
  print_line( "Save Full" );
  REQUIRE( save_game_to_rcl_file( W.ss(), dst, opts ) );

  UNWRAP_CHECK( src_text,
                base::read_text_file_as_string( src ) );
  UNWRAP_CHECK( dst_text,
                base::read_text_file_as_string( dst ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( src_text == dst_text ) );
}
#endif

TEST_CASE( "[save-game] world gen with default values (full)" ) {
  World W;
  W.expensive_run_lua_init();
  W.initialize_ts();
  reset_seeds( W.lua() );
  expect_rands( W );
  create_new_game_from_lua( W );
  RootState backup = W.root();

  // FIXME: find a better way to get a random temp folder.
  static fs::path const dst = "/tmp/test-world-gen-full.sav.rcl";
  if( fs::exists( dst ) ) fs::remove( dst );
  CHECK( !fs::exists( dst ) );

  SaveGameOptions opts{
      .verbosity = e_savegame_verbosity::full,
  };

  // Make a round trip.
  print_line( "Save Gen" );
  REQUIRE( save_game_to_rcl_file( W.ss(), dst, opts ) );
  W.root() = {};
  print_line( "Load Gen" );
  REQUIRE( load_game_from_rcl_file( W.root(), dst, opts ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( backup == W.root() ) );
}

TEST_CASE(
    "[save-game] world gen with no default values (compact)" ) {
  World W;
  W.expensive_run_lua_init();
  W.initialize_ts();
  reset_seeds( W.lua() );
  expect_rands( W );
  create_new_game_from_lua( W );
  RootState backup = W.root();

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
  REQUIRE( save_game_to_rcl_file( W.ss(), dst, opts ) );
  W.root() = {};
  print_line( "Load Gen" );
  REQUIRE( load_game_from_rcl_file( W.root(), dst, opts ) );

  // Use parenthesis here so that it doesn't dump the entire save
  // file to the console if they don't match.
  REQUIRE( ( backup == W.root() ) );
}

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
