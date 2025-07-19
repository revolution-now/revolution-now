/****************************************************************
**human-euro-agent-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-02.
*
* Description: Unit tests for the human-euro-agent module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/human-euro-agent.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

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
    : agent_( default_player_type(), ss(), gui(), planes() ) {
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

  HumanEuroAgent agent_;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[human-euro-agent] pick_dump_cargo" ) {
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

TEST_CASE( "[human-euro-agent] should_take_native_land" ) {
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

TEST_CASE( "[human-euro-agent] handle/ChooseImmigrant" ) {
  world w;
  // TODO
}

} // namespace
} // namespace rn
