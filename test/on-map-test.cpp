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
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/mocks/irand.hpp"

// mock
#include "src/mock/matchers.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {

struct TestingOnlyUnitOnMapMover : UnitOnMapMover {
  using Base = UnitOnMapMover;
  using Base::to_map_interactive;
  using Base::to_map_non_interactive;
};

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
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
    set_default_player( e_nation::dutch );
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
    TestingOnlyUnitOnMapMover::to_map_non_interactive(
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
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
    UnitOnMapMover::native_unit_to_map_non_interactive(
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
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
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
        .returns();
    W.gui()
        .EXPECT__string_input( _, e_input_required::yes )
        .returns<maybe<string>>( "my world 2" );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
    REQUIRE( !w.exception() );
    REQUIRE( w.ready() );
    REQUIRE( *w == nothing );
    REQUIRE( W.units().coord_for( unit_id ) ==
             Coord{ .x = 0, .y = 1 } );
    REQUIRE( player.new_world_name == "my world 2" );
  }
}

TEST_CASE( "[on-map] interactive: discovers pacific ocean" ) {
  World   W;
  Player& player                           = W.default_player();
  player.new_world_name                    = "";
  W.terrain().pacific_ocean_endpoints()[0] = 1;
  W.terrain().pacific_ocean_endpoints()[1] = 2;
  W.terrain().pacific_ocean_endpoints()[2] = 1;
  UnitId const unit_id =
      W.add_unit_on_map( e_unit_type::treasure,
                         { .x = 2, .y = 4 } )
          .id();
  wait<maybe<UnitDeleted>> w = make_wait<maybe<UnitDeleted>>();

  REQUIRE(
      player.woodcuts[e_woodcut::discovered_pacific_ocean] ==
      false );

  W.gui()
      .EXPECT__display_woodcut(
          e_woodcut::discovered_pacific_ocean )
      .returns();
  w = TestingOnlyUnitOnMapMover::to_map_interactive(
      W.ss(), W.ts(), unit_id, { .x = 1, .y = 3 } );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( *w == nothing );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 1, .y = 3 } );
  REQUIRE(
      player.woodcuts[e_woodcut::discovered_pacific_ocean] ==
      true );

  // Make sure it doesn't happen again.
  REQUIRE( W.terrain().is_pacific_ocean( { .x = 0, .y = 2 } ) );
  w = TestingOnlyUnitOnMapMover::to_map_interactive(
      W.ss(), W.ts(), unit_id, { .x = 0, .y = 3 } );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
  REQUIRE( *w == nothing );
  REQUIRE( W.units().coord_for( unit_id ) ==
           Coord{ .x = 0, .y = 3 } );
  REQUIRE(
      player.woodcuts[e_woodcut::discovered_pacific_ocean] ==
      true );
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
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 0, .y = 1 } );
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
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 1, .y = 1 } );
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
        "Treasure worth 1000\x7f arrives in Amsterdam!  The "
        "crown has provided a reimbursement of [500\x7f] after "
        "a [50%] witholding.";
    W.gui().EXPECT__message_box( msg ).returns( monostate{} );
    w = TestingOnlyUnitOnMapMover::to_map_interactive(
        W.ss(), W.ts(), unit_id, { .x = 1, .y = 1 } );
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
        TestingOnlyUnitOnMapMover::to_map_interactive(
            W.ss(), W.ts(), unit.id(), to );
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
      .returns();
  W.gui()
      .EXPECT__message_box( StrContains( "Youth" ) )
      .returns();

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
  UNWRAP_CHECK( player_terrain, W.terrain().player_terrain(
                                    W.default_nation() ) );
  gfx::Matrix<maybe<FogSquare>> const& map = player_terrain.map;

  REQUIRE( !map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 2, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 3, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 4, .y = 1 }].has_value() );

  Unit const& unit =
      W.add_free_unit( e_unit_type::free_colonist );
  REQUIRE( !map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 2, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 3, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 4, .y = 1 }].has_value() );

  TestingOnlyUnitOnMapMover::to_map_non_interactive(
      W.ss(), W.ts(), unit.id(), { .x = 0, .y = 1 } );
  REQUIRE( map[{ .x = 0, .y = 1 }].has_value() );
  REQUIRE( map[{ .x = 1, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 2, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 3, .y = 1 }].has_value() );
  REQUIRE( !map[{ .x = 4, .y = 1 }].has_value() );
  REQUIRE( map[{ .x = 0, .y = 1 }]->fog_of_war_removed );
  REQUIRE( map[{ .x = 1, .y = 1 }]->fog_of_war_removed );
}

TEST_CASE(
    "[on-map] non-interactive: to_map_non_interactive "
    "unsentries surrounding units" ) {
  World W;
  Unit& unit1 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_nation::dutch );
  unit1.sentry();
  REQUIRE( unit1.orders().holds<unit_orders::sentry>() );
  Unit& unit2 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 2, .y = 1 }, e_nation::spanish );
  REQUIRE( unit1.orders().holds<unit_orders::none>() );
  REQUIRE( unit2.orders().holds<unit_orders::none>() );
}

TEST_CASE(
    "[on-map] non-interactive: "
    "native_unit_to_map_non_interactive unsentries surrounding "
    "units" ) {
  World W;
  Unit& unit1 =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 }, e_nation::dutch );
  unit1.sentry();
  REQUIRE( unit1.orders().holds<unit_orders::sentry>() );
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 2, .y = 1 }, e_tribe::cherokee );
  W.add_native_unit_on_map( e_native_unit_type::brave,
                            { .x = 2, .y = 0 }, dwelling.id );
  REQUIRE( unit1.orders().holds<unit_orders::none>() );
}

TEST_CASE(
    "[on-map] non-interactive: native_unit_to_map_interactive "
    "meets europeans" ) {
  World W;
  W.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 1, .y = 1 }, e_nation::dutch );
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::cherokee );
  NativeUnit const& native_unit = W.add_native_unit_on_map(
      e_native_unit_type::brave, { .x = 3, .y = 1 },
      dwelling.id );

  W.euro_mind( e_nation::dutch )
      .EXPECT__meet_tribe_ui_sequence(
          MeetTribe{ .nation        = e_nation::dutch,
                     .tribe         = e_tribe::cherokee,
                     .num_dwellings = 1 } )
      .returns<wait<e_declare_war_on_natives>>(
          e_declare_war_on_natives::no );
  wait<> const w =
      UnitOnMapMover::native_unit_to_map_interactive(
          W.ss(), W.ts(), native_unit.id, { .x = 2, .y = 1 },
          dwelling.id );
  REQUIRE( !w.exception() );
  REQUIRE( w.ready() );
}

} // namespace
} // namespace rn
