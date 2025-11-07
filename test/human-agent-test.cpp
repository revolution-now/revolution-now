/****************************************************************
**human-agent-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-02.
*
* Description: Unit tests for the human-agent module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/human-agent.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/iengine.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/land-view.rds.hpp"
#include "src/ss/turn.rds.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;
using ::mock::matchers::AllOf;
using ::mock::matchers::Field;
using ::mock::matchers::IterableElementsAre;
using ::mock::matchers::StrContains;
using ::refl::enum_map;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world()
    : agent_( default_player_type(), engine(), ss(),
              map_updater(), gui(), planes() ) {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }

  void create_isolation_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, _, L, L, L, //
      L, L, _, L, L, L, //
      _, _, _, L, L, L, //
      L, L, L, L, L, L, //
      L, L, L, L, L, L, //
      L, L, L, L, L, L, //
    };
    build_map( std::move( tiles ), 6 );
  }

  HumanAgent agent_;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[human-agent] pick_dump_cargo" ) {
  world w;
  map<int /*slot*/, Commodity> comms;

  auto const f = [&] [[clang::noinline]] {
    return co_await_test( w.agent_.pick_dump_cargo( comms ) );
  };

  using enum e_commodity;
  comms[0] = { .type = cotton, .quantity = 23 };
  comms[2] = { .type = tools, .quantity = 100 };
  comms[4] = { .type = horses, .quantity = 50 };
  comms[5] = { .type = muskets, .quantity = 1 };

  ChoiceConfig const config{
    .msg = "What cargo would you like to dump overboard?",
    .options =
        {
          ChoiceConfigOption{
            .key          = "0",
            .display_name = "23 cotton",
          },
          ChoiceConfigOption{
            .key          = "2",
            .display_name = "100 tools",
          },
          ChoiceConfigOption{
            .key          = "4",
            .display_name = "50 horses",
          },
          ChoiceConfigOption{
            .key          = "5",
            .display_name = "1 muskets",
          },
        },
  };

  SECTION( "selects nothing" ) {
    w.gui().EXPECT__choice( config ).returns( nothing );
    REQUIRE( f() == nothing );
  }

  SECTION( "selects slot 2" ) {
    w.gui().EXPECT__choice( config ).returns( "2" );
    REQUIRE( f() == 2 );
  }
}

TEST_CASE( "[human-agent] should_take_native_land" ) {
  world w;

  string msg;
  enum_map<e_native_land_grab_result, string> names;
  enum_map<e_native_land_grab_result, bool> disabled;

  auto const f = [&] [[clang::noinline]] {
    return co_await_test( w.agent_.should_take_native_land(
        msg, names, disabled ) );
  };

  msg = "You are trespassing on native land";
  names[e_native_land_grab_result::cancel]    = "cancel";
  disabled[e_native_land_grab_result::cancel] = false;
  names[e_native_land_grab_result::pay]       = "pay";
  disabled[e_native_land_grab_result::pay]    = true;
  names[e_native_land_grab_result::take]      = "take";
  disabled[e_native_land_grab_result::take]   = false;

  auto config_matcher = AllOf(
      Field( &ChoiceConfig::msg,
             StrContains( "You are trespassing" ) ),
      Field(
          &ChoiceConfig::options,
          IterableElementsAre(
              AllOf(
                  Field( &ChoiceConfigOption::key, "cancel"s ),
                  Field( &ChoiceConfigOption::disabled,
                         false ) ),
              AllOf(
                  Field( &ChoiceConfigOption::key, "pay"s ),
                  Field( &ChoiceConfigOption::disabled, true ) ),
              AllOf( Field( &ChoiceConfigOption::key, "take"s ),
                     Field( &ChoiceConfigOption::disabled,
                            false ) ) ) ) );
  w.gui()
      .EXPECT__choice( std::move( config_matcher ) )
      .returns( "take" );

  REQUIRE( f() == e_native_land_grab_result::take );
}

TEST_CASE( "[human-agent] confirm_build_inland_colony" ) {
  world w;

  auto const f = [&] [[clang::noinline]] {
    return co_await_test(
        w.agent_.confirm_build_inland_colony() );
  };

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "access it by ship" ) ) )
      .returns( "no" );
  REQUIRE( f() == ui::e_confirm::no );

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "access it by ship" ) ) )
      .returns( "yes" );
  REQUIRE( f() == ui::e_confirm::yes );

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "access it by ship" ) ) )
      .returns( nothing );
  REQUIRE( f() == ui::e_confirm::no );
}

TEST_CASE( "[human-agent] name_colony" ) {
  world w;
  Colony& colony = w.add_colony( { .x = 1, .y = 1 } );
  colony.name    = "used";

  auto const f = [&] [[clang::noinline]] {
    return co_await_test( w.agent_.name_colony() );
  };

  SECTION( "good" ) {
    w.gui()
        .EXPECT__string_input(
            Field( &StringInputConfig::msg,
                   StrContains( "colony be named" ) ) )
        .returns( "good" );
    REQUIRE( f() == "good" );
  }

  SECTION( "spaces" ) {
    w.gui()
        .EXPECT__string_input(
            Field( &StringInputConfig::msg,
                   StrContains( "colony be named" ) ) )
        .returns( " bad " );
    w.gui().EXPECT__message_box(
        StrContains( "start or end with spaces" ) );
    w.gui()
        .EXPECT__string_input(
            Field( &StringInputConfig::msg,
                   StrContains( "colony be named" ) ) )
        .returns( "good" );
    REQUIRE( f() == "good" );
  }

  SECTION( "already exists" ) {
    w.gui()
        .EXPECT__string_input(
            Field( &StringInputConfig::msg,
                   StrContains( "colony be named" ) ) )
        .returns( "used" );
    w.gui().EXPECT__message_box(
        StrContains( "already a colony" ) );
    w.gui()
        .EXPECT__string_input(
            Field( &StringInputConfig::msg,
                   StrContains( "colony be named" ) ) )
        .returns( "good" );
    REQUIRE( f() == "good" );
  }
}

TEST_CASE( "[human-agent] should_make_landfall" ) {
  world w;
  bool already_moved = false;

  auto const f = [&] [[clang::noinline]] {
    return co_await_test(
        w.agent_.should_make_landfall( already_moved ) );
  };

  already_moved = false;
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "like to make landfall?" ) ) )
      .returns( "no" );
  REQUIRE( f() == ui::e_confirm::no );

  already_moved = true;
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "have already moved" ) ) )
      .returns( "no" );
  REQUIRE( f() == ui::e_confirm::no );

  already_moved = false;
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "like to make landfall?" ) ) )
      .returns( "yes" );
  REQUIRE( f() == ui::e_confirm::yes );

  already_moved = true;
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "have already moved" ) ) )
      .returns( "yes" );
  REQUIRE( f() == ui::e_confirm::yes );

  already_moved = true;
  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "have already moved" ) ) )
      .returns( nothing );
  REQUIRE( f() == ui::e_confirm::no );
}

TEST_CASE( "[human-agent] should_sail_high_seas" ) {
  world w;

  UnitId const dummy_unit_id{ 1 };

  auto const f = [&] [[clang::noinline]] {
    return co_await_test(
        w.agent_.should_sail_high_seas( dummy_unit_id ) );
  };

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "sail the high seas" ) ) )
      .returns( "no" );
  REQUIRE( f() == ui::e_confirm::no );

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "sail the high seas" ) ) )
      .returns( "yes" );
  REQUIRE( f() == ui::e_confirm::yes );

  w.gui()
      .EXPECT__choice(
          Field( &ChoiceConfig::msg,
                 StrContains( "sail the high seas" ) ) )
      .returns( nothing );
  REQUIRE( f() == ui::e_confirm::no );
}

// Just do something basic here since the bulk of it is already
// tested in other modules. One thing that this provides is that
// it will break if omniscient path finding is enabled.
TEST_CASE( "[human-agent] evolve_goto" ) {
  world w;
  w.create_isolation_map();
  EvolveGoto expected;

  using enum e_surface;
  using enum e_unit_type;

  point const kStartingTile{ .x = 0, .y = 0 };
  point const kTargetTile{ .x = 4, .y = 4 };

  Unit& unit = w.add_unit_on_map( free_colonist, kStartingTile );

  auto const f = [&] [[clang::noinline]] {
    return w.agent_.evolve_goto( unit.id() );
  };

  BASE_CHECK( w.square( kStartingTile ).surface == land );
  BASE_CHECK( w.square( kTargetTile ).surface == land );

  // This will cause the viz to be from the player's standpoint,
  // wherein all tiles are hidden, so the path should succeed. On
  // the other hand, if omniscient path finding is enabled then
  // the path should fail here.
  w.turn().cycle =
      TurnCycle::player{ .type = w.default_player_type() };
  w.land_view().map_revealed = MapRevealed::no_special_view{};
  unit.orders()              = unit_orders::go_to{
                 .target = goto_target::map{ .tile = kTargetTile } };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );

  // This viz will see the full map regardless of the omniscient
  // path finding config, thus the path should always fail.
  w.land_view().map_revealed = MapRevealed::entire{};
  unit.orders()              = unit_orders::go_to{
                 .target = goto_target::map{ .tile = kTargetTile } };
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );
}

TEST_CASE( "[human-agent] evolve_trade_route" ) {
  world w;
}

} // namespace
} // namespace rn
