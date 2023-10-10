/****************************************************************
**command-disband-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-10-08.
*
* Description: Unit tests for the command-disband module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/command-disband.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/unit-composer.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, //
        L, L, L, //
        _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[command-disband] confirm+perform" ) {
  World W;

  UnitId unit_id = {};

  auto confirm = [&] {
    auto handler =
        handle_command( W.ss(), W.ts(), W.default_player(),
                        unit_id, command::disband{} );
    return co_await_test( handler->confirm() );
  };

  auto perform = [&] {
    auto handler =
        handle_command( W.ss(), W.ts(), W.default_player(),
                        unit_id, command::disband{} );
    return co_await_test( handler->perform() );
  };

  auto require_on_land = [&]( Unit const& unit ) {
    REQUIRE( as_const( W.units() )
                 .ownership_of( unit.id() )
                 .holds<UnitOwnership::world>() );
  };

  auto require_sentried = [&]( Unit const& unit ) {
    REQUIRE( unit.orders().holds<unit_orders::sentry>() );
  };

  auto require_not_sentried = [&]( Unit const& unit ) {
    REQUIRE( !unit.orders().holds<unit_orders::sentry>() );
  };

  SECTION( "cancelled" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        nothing );
    REQUIRE( confirm() == false );
    REQUIRE( W.units().exists( ship_id ) );
  }

  SECTION( "selected no" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "no" );
    REQUIRE( confirm() == false );
    REQUIRE( W.units().exists( ship_id ) );
  }

  SECTION( "selected yes" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE_FALSE( W.units().exists( ship_id ) );
  }

  SECTION( "selected yes / land" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    UnitId const free_colonist_id =
        W.add_unit_on_map( e_unit_type::free_colonist,
                           { .x = 1, .y = 1 } )
            .id();
    unit_id = free_colonist_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE( W.units().exists( ship_id ) );
    REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  }

  SECTION( "unit on ship" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    UnitId const cargo_unit_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    unit_id = cargo_unit_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE( W.units().exists( ship_id ) );
    REQUIRE_FALSE( W.units().exists( cargo_unit_id ) );
    require_not_sentried( W.units().unit_for( ship_id ) );
  }

  SECTION( "ship with units on ocean" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    UnitId const cargo_unit_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE_FALSE( W.units().exists( ship_id ) );
    REQUIRE_FALSE( W.units().exists( cargo_unit_id ) );
  }

  SECTION( "ship with units on land" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 1, .y = 1 } )
            .id();
    UnitId const cargo_unit_id =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE_FALSE( W.units().exists( ship_id ) );
    REQUIRE( W.units().exists( cargo_unit_id ) );
    require_on_land( W.units().unit_for( cargo_unit_id ) );
    require_sentried( W.units().unit_for( cargo_unit_id ) );
  }

  SECTION( "ship with more units on water" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 0, .y = 0 } )
            .id();
    UnitId const cargo_unit_id1 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    UnitId const cargo_unit_id2 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    UnitId const cargo_unit_id3 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE_FALSE( W.units().exists( ship_id ) );
    REQUIRE_FALSE( W.units().exists( cargo_unit_id1 ) );
    REQUIRE_FALSE( W.units().exists( cargo_unit_id2 ) );
    REQUIRE_FALSE( W.units().exists( cargo_unit_id3 ) );
  }

  SECTION( "ship with more units on land" ) {
    UnitId const ship_id =
        W.add_unit_on_map( e_unit_type::galleon,
                           { .x = 1, .y = 1 } )
            .id();
    UnitId const cargo_unit_id1 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    UnitId const cargo_unit_id2 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    UnitId const cargo_unit_id3 =
        W.add_unit_in_cargo( e_unit_type::free_colonist,
                             ship_id )
            .id();
    unit_id = ship_id;
    W.gui().EXPECT__choice( _, _ ).returns<maybe<string>>(
        "yes" );
    REQUIRE( confirm() == true );
    perform();
    REQUIRE_FALSE( W.units().exists( ship_id ) );
    REQUIRE( W.units().exists( cargo_unit_id1 ) );
    REQUIRE( W.units().exists( cargo_unit_id2 ) );
    REQUIRE( W.units().exists( cargo_unit_id3 ) );
    require_on_land( W.units().unit_for( cargo_unit_id1 ) );
    require_on_land( W.units().unit_for( cargo_unit_id2 ) );
    require_on_land( W.units().unit_for( cargo_unit_id3 ) );
    require_sentried( W.units().unit_for( cargo_unit_id1 ) );
    require_sentried( W.units().unit_for( cargo_unit_id2 ) );
    require_sentried( W.units().unit_for( cargo_unit_id3 ) );
  }
}

// Make sure that the "no" option is first and highlighted by de-
// fault, so if you just hit enter accidentally nothing bad will
// happen.
TEST_CASE(
    "[command-disband] confirmation box has 'no' first" ) {
  World W;

  UnitId const unit_id = W.add_unit_on_map( e_unit_type::galleon,
                                            { .x = 0, .y = 0 } )
                             .id();

  auto confirm = [&] {
    auto handler =
        handle_command( W.ss(), W.ts(), W.default_player(),
                        unit_id, command::disband{} );
    return co_await_test( handler->confirm() );
  };

  ChoiceConfig const expected_config{
      .msg     = "Really disband [Galleon]?",
      .options = { ChoiceConfigOption{ .key          = "no",
                                       .display_name = "No" },
                   ChoiceConfigOption{ .key          = "yes",
                                       .display_name = "Yes" } },
      .sort    = false,
      // This should cause the first enabled item to be selected
      // by default.
      .initial_selection = nothing };

  W.gui()
      .EXPECT__choice( expected_config, e_input_required::no )
      .returns<maybe<string>>( nothing );
  REQUIRE( confirm() == false );
}

} // namespace
} // namespace rn
