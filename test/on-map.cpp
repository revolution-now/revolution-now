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
#include "test/mocks/irand.hpp"

// mock
#include "src/mock/matchers.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;
using ::mock::matchers::StrContains;

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
  World W;

  SECTION( "euro unit" ) {
    UnitId const unit_id =
        W.add_unit_on_map( e_unit_type::treasure,
                           { .x = 1, .y = 0 } )
            .id();
    unit_to_map_square_non_interactive( W.ss(), W.ts(), unit_id,
                                        { .x = 0, .y = 1 } );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
  }

  SECTION( "native unit" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 0, .y = 0 }, e_tribe::apache );
    NativeUnitId const unit_id =
        W.add_native_unit_on_map(
             e_native_unit_type::armed_brave, { .x = 1, .y = 0 },
             dwelling.id )
            .id;
    unit_to_map_square_non_interactive(
        W.ss(), unit_id, { .x = 1, .y = 1 }, dwelling.id );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 1, .y = 1 } );
  }
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

  REQUIRE( player.new_world_name == nothing );

  SECTION( "already discovered" ) {
    player.new_world_name = "my world";
    player.woodcuts[e_woodcut::discovered_new_world] = true;
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world" );
  }

  SECTION( "not yet discovered" ) {
    W.gui()
        .EXPECT__display_woodcut(
            e_woodcut::discovered_new_world )
        .returns<monostate>();
    W.gui()
        .EXPECT__string_input( _, e_input_required::yes )
        .returns<maybe<string>>( "my world 2" );
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world 2" );
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
  player.new_world_name = "";

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
    W.gui()
        .EXPECT__choice( _, e_input_required::no )
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
    W.gui()
        .EXPECT__choice( _, e_input_required::no )
        .returns<maybe<string>>( "yes" );
    string const msg =
        "Treasure worth 1000 arrives in Amsterdam!  The crown "
        "has provided a reimbursement of [500] after a "
        "[50%] witholding.";
    W.gui().EXPECT__message_box( msg ).returns( monostate{} );
    w = unit_to_map_square( W.ss(), W.ts(), unit_id,
                            { .x = 1, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( w->has_value() );
    REQUIRE( !W.units().exists( unit_id ) );
  }
}
#endif

TEST_CASE(
    "[on-map] interactive: [LCR] shows fountain of youth "
    "woodcut" ) {
  World       W;
  Coord       to   = { .x = 1, .y = 1 };
  Unit const& unit = W.add_unit_on_map(
      e_unit_type::free_colonist, { .x = 1, .y = 0 } );
  MapSquare& square      = W.square( { .x = 1, .y = 1 } );
  square.lost_city_rumor = true;
  Player& player         = W.default_player();
  player.new_world_name  = "my world";
  player.woodcuts[e_woodcut::discovered_new_world] = true;

  auto f = [&] {
    wait<maybe<UnitDeleted>> const w =
        unit_to_map_square( W.ss(), W.ts(), unit.id(), to );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
  };

  // Selects rumor result = fountain of youth.
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 58 );
  // Selects burial mounds type (not relevant).
  W.rand()
      .EXPECT__between_ints( 0, 100, e_interval::half_open )
      .returns( 0 );
  W.gui()
      .EXPECT__display_woodcut(
          e_woodcut::discovered_fountain_of_youth )
      .returns<monostate>();
  W.gui()
      .EXPECT__message_box( StrContains( "Youth" ) )
      .returns<monostate>();

  for( int i = 0; i < 8; ++i ) {
    // Pick immigrant.
    W.gui()
        .EXPECT__choice( _, e_input_required::no )
        .returns<maybe<string>>( "0" );
    // Replace with next immigrant.
    W.rand().EXPECT__between_doubles( _, _ ).returns( 0 );
    // Wait a bit.
    W.gui()
        .EXPECT__wait_for( _ )
        .returns<chrono::microseconds>();
  }

  f();
}

TEST_CASE( "[on-map] non-interactive: updates visibility" ) {
  World W;
  // TODO
}

} // namespace
} // namespace rn
