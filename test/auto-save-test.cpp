/****************************************************************
**auto-save-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-06.
*
* Description: Unit tests for the auto-save module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/auto-save.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"

// Revolution Now
#include "src/isave-game.rds.hpp"

// config
#include "src/config/savegame.rds.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/settings.rds.hpp"
#include "src/ss/turn.rds.hpp"

// base
#include "src/base/scope-exit.hpp"
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

RDS_DEFINE_MOCK( IGameWriter );

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  World() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      _, L, L, //
      L, L, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[auto-save] should_autosave" ) {
  World w;
  set<int> expected;

  auto f = [&] { return should_autosave( w.ss().as_const ); };

  // Sanity check.
  expected = {};
  REQUIRE( f() == expected );

  // The strategy here is to enable everything that we need to
  // get multiple auto-saves to trigger, then slowly remove each
  // condition to test them.

  w.settings()
      .in_game_options
      .game_menu_options[e_game_menu_option::autosave] = true;
  w.turn().time_point.turns                            = 10;
  w.turn().autosave.last_save                          = 0;
  expected = { 0, 1 };
  REQUIRE( f() == expected );

  SECTION( "disabled in config" ) {
    auto& conf = ::rn::detail::__config_savegame;
    SCOPED_SET_AND_RESTORE( conf.autosave.enabled, false );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "disabled in game options" ) {
    w.settings()
        .in_game_options
        .game_menu_options[e_game_menu_option::autosave] = false;
    expected                                             = {};
    REQUIRE( f() == expected );
  }

  SECTION( "last turn same as current" ) {
    w.turn().autosave.last_save = w.turn().time_point.turns;
    expected                    = {};
    REQUIRE( f() == expected );
  }

  SECTION( "last turn larger than current" ) {
    w.turn().autosave.last_save = w.turn().time_point.turns + 1;
    expected                    = {};
    REQUIRE( f() == expected );
  }

  SECTION( "one slot disabled" ) {
    auto& conf = ::rn::detail::__config_savegame;
    BASE_CHECK( conf.autosave.slots.size() == 2 );
    SCOPED_SET_AND_RESTORE( conf.autosave.slots[1].enabled,
                            false );
    expected = { 0 };
    REQUIRE( f() == expected );
  }

  SECTION( "both slots disabled" ) {
    auto& conf = ::rn::detail::__config_savegame;
    BASE_CHECK( conf.autosave.slots.size() == 2 );
    SCOPED_SET_AND_RESTORE( conf.autosave.slots[0].enabled,
                            false );
    SCOPED_SET_AND_RESTORE( conf.autosave.slots[1].enabled,
                            false );
    expected = {};
    REQUIRE( f() == expected );
  }

  SECTION( "slot frequency is off current turn" ) {
    w.turn().time_point.turns = 11;
    expected                  = { 1 };
    REQUIRE( f() == expected );
  }

  SECTION( "turn zero, saved at zero" ) {
    w.turn().autosave.last_save = 0;
    w.turn().time_point.turns   = 0;
    expected                    = {};
    REQUIRE( f() == expected );
  }

  SECTION( "turn zero, never saved" ) {
    w.turn().autosave.last_save = nothing;
    w.turn().time_point.turns   = 0;
    expected                    = { 1 };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[auto-save] autosave" ) {
  World w;
  set<int> slots;
  expect<std::vector<fs::path>> expected = "";

  MockIGameWriter mock_game_writer;

  auto f = [&] {
    return autosave( w.ss(), mock_game_writer, w.turn().autosave,
                     slots );
  };

  BASE_CHECK( w.turn().autosave.last_save == nothing );
  w.turn().time_point.turns = 3;

  SECTION( "no slots" ) {
    slots    = {};
    expected = vector<fs::path>{};
    REQUIRE( f() == expected );
  }

  SECTION( "slot 0 only" ) {
    slots    = { 0 };
    expected = vector<fs::path>{ "xyz" };
    mock_game_writer.EXPECT__save_to_slot_no_checkpoint( 8 )
        .returns<fs::path>( "xyz" )
        .invokes( [&] {
          // This checks that the last_save field is set before
          // doing the actual save.
          BASE_CHECK( w.turn().autosave.last_save == 3 );
        } );
    REQUIRE( f() == expected );
  }

  SECTION( "slot 1 only" ) {
    slots    = { 1 };
    expected = vector<fs::path>{ "xyz" };
    mock_game_writer.EXPECT__save_to_slot_no_checkpoint( 9 )
        .returns<fs::path>( "xyz" )
        .invokes( [&] {
          // This checks that the last_save field is set before
          // doing the actual save.
          BASE_CHECK( w.turn().autosave.last_save == 3 );
        } );
    REQUIRE( f() == expected );
  }

  SECTION( "slot 0, 1" ) {
    slots    = { 0, 1 };
    expected = vector<fs::path>{ "abc", "xyz" };
    mock_game_writer.EXPECT__save_to_slot_no_checkpoint( 8 )
        .returns<fs::path>( "abc" )
        .invokes( [&] {
          // This checks that the last_save field is set before
          // doing the actual save.
          BASE_CHECK( w.turn().autosave.last_save == 3 );
        } );
    mock_game_writer.EXPECT__copy_slot_to_slot( 8, 9 ).returns(
        SlotCopiedPaths{ .src = "abc", .dst = "xyz" } );
    REQUIRE( f() == expected );
  }
}

} // namespace
} // namespace rn
