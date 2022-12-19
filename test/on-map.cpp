/****************************************************************
**on-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-17.
*
* Description: Unit tests for the src/on-map.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/on-map.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/igui.hpp"

// mock
#include "src/mock/matchers.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

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

  Coord const kColonySquare = Coord{ .x = 1, .y = 1 };

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _, L, L, L,
      L, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
      _, L, L, L, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 6 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[on-map] non-interactive: moves the unit" ) {
  World        W;
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 0 } )
          .id();
  unit_to_map_square_non_interactive( W.ss(), W.ts(), unit_id,
                                      { .x = 0, .y = 1 } );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 0, .y = 1 } );

  NativeUnitId const unit_id2 =
      W.add_unit_on_map( e_native_unit_type::armed_brave,
                         { .x = 1, .y = 0 }, e_tribe::apache )
          .id;
  unit_to_map_square_non_interactive( W.ss(), unit_id2,
                                      { .x = 1, .y = 1 } );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 0, .y = 1 } );
}

#ifndef COMPILER_GCC
TEST_CASE( "[on-map] interactive: discovers new world" ) {
  World        W;
  Player&      player = W.default_player();
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 0 } )
          .id();
  wait<maybe<UnitDeleted>> w = make_wait<maybe<UnitDeleted>>();

  REQUIRE( player.discovered_new_world == nothing );

  SECTION( "already discovered" ) {
    player.discovered_new_world = "my world";
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.discovered_new_world == "my world" );
  }

  SECTION( "not yet discovered" ) {
    EXPECT_CALL( W.gui(),
                 string_input( _, e_input_required::yes ) )
        .returns<maybe<string>>( "my world 2" );
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.discovered_new_world == "my world 2" );
  }
}

TEST_CASE( "[on-map] interactive: treasure in colony" ) {
  World   W;
  Player& player = W.default_player();
  W.add_colony_with_new_unit( W.kColonySquare );
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 1, .y = 0 } )
          .id();
  wait<maybe<UnitDeleted>> w = make_wait<maybe<UnitDeleted>>();

  // This is so that the user doesn't get prompted to name the
  // new world.
  player.discovered_new_world = "";

  SECTION( "not entering colony" ) {
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
  }

  SECTION( "entering colony answer no" ) {
    EXPECT_CALL( W.gui(), choice( _, e_input_required::no ) )
        .returns<maybe<string>>( "no" );
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "entering colony answer yes" ) {
    EXPECT_CALL( W.gui(), choice( _, e_input_required::no ) )
        .returns<maybe<string>>( "yes" );
    string const msg =
        "Treasure worth 1000 arrives in Amsterdam!  The crown "
        "has provided a reimbursement of @[H]500@[] after a "
        "@[H]50%@[] witholding.";
    EXPECT_CALL( W.gui(), message_box( msg ) )
        .returns( monostate{} );
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( !W.units().exists( unit_id ) );
  }
}
#endif

TEST_CASE( "[on-map] non-interactive: updates visibility" ) {
  World W;
  // TODO
}

TEST_CASE( "[on-map] interactive: discovers rumor" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
