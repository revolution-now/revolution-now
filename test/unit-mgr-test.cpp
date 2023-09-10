/****************************************************************
**unit-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-17.
*
* Description: Unit tests for the src/unit-mgr.* module.
*
*****************************************************************/
// Under test.
#include "src/unit-mgr.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/testing.hpp"

// Revolution Now
#include "src/visibility.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/tribe.rds.hpp"
#include "src/ss/units.hpp"
#include "src/ss/woodcut.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

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
    add_player( e_nation::dutch );
    add_player( e_nation::spanish );
    set_default_player( e_nation::dutch );
    create_default_map();
  }

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

  inline static Coord const kLand = Coord{ .x = 1, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unit-mgr] current_activity_for_unit" ) {
  World W;

  auto f = [&]( UnitId id ) {
    return current_activity_for_unit( W.units(), W.colonies(),
                                      id );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal farmer" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_outdoors( colony.id, e_direction::w,
                             e_outdoor_job::food,
                             e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::farming );
  }

  SECTION( "petty_criminal no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::petty_criminal, W.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "veteran_dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "pioneer" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::petty_criminal ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::pioneering );
  }

  SECTION( "missionary" ) {
    UnitComposition expected;
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id = W.add_missionary_in_dwelling(
                           e_unit_type::missionary, dwelling.id )
                          .id();
    REQUIRE( f( id ) == e_unit_activity::missioning );
  }
}

TEST_CASE( "[unit-mgr] tribe_type_for_unit" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_type_for_unit( W.ss(), unit ) ==
           e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] tribe_for_unit" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_for_unit( W.ss(), unit ).type ==
           e_tribe::arawak );
  SSConst const ss_const( W.ss() );
  REQUIRE( tribe_for_unit( ss_const, unit ).type ==
           e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] coord_for_unit_multi_ownership" ) {
  World W;

  SECTION( "colonist in colony" ) {
    W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    UnitId const id{ 1 };
    REQUIRE(
        !coord_for_unit_indirect( W.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( W.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "missionary in dwelling" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id = W.add_missionary_in_dwelling(
                           e_unit_type::missionary, dwelling.id )
                          .id();
    REQUIRE(
        !coord_for_unit_indirect( W.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( W.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }
}

TEST_CASE( "[unit-mgr] change_unit_type" ) {
  World            W;
  Visibility const viz( W.ss(), W.default_nation() );
  Unit& unit = W.add_unit_on_map( e_unit_type::free_colonist,
                                  { .x = 3, .y = 3 } );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );

  change_unit_type( W.ss(), W.ts(), unit,
                    e_unit_type::expert_farmer );

  REQUIRE( unit.type() == e_unit_type::expert_farmer );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );

  change_unit_type( W.ss(), W.ts(), unit, e_unit_type::scout );

  REQUIRE( unit.type() == e_unit_type::scout );
  REQUIRE( viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
}

TEST_CASE( "[unit-mgr] change_unit_nation" ) {
  World            W;
  Visibility const dutch_viz( W.ss(), e_nation::dutch );
  Visibility const spanish_viz( W.ss(), e_nation::spanish );
  Unit&            unit =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 3, .y = 3 }, e_nation::dutch );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::hidden );

  change_unit_nation( W.ss(), W.ts(), unit, e_nation::spanish );

  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );

  change_unit_type( W.ss(), W.ts(), unit, e_unit_type::scout );

  REQUIRE( unit.type() == e_unit_type::scout );
  REQUIRE( dutch_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( dutch_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( dutch_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 0, .y = 0 } ) ==
           e_tile_visibility::hidden );
  REQUIRE( spanish_viz.visible( { .x = 1, .y = 1 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 2, .y = 2 } ) ==
           e_tile_visibility::visible_and_clear );
  REQUIRE( spanish_viz.visible( { .x = 3, .y = 3 } ) ==
           e_tile_visibility::visible_and_clear );
}

TEST_CASE( "[unit-mgr] change_unit_nation_and_move" ) {
  World W;
}

TEST_CASE( "[unit-mgr] destroy_unit" ) {
  World        W;
  UnitId const caravel_id =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 0, .y = 0 } )
          .id();
  UnitId const free_colonist_id =
      W.add_unit_in_cargo( e_unit_type::free_colonist,
                           caravel_id )
          .id();
  UnitId const expert_farmer_id =
      W.add_unit_in_cargo( e_unit_type::expert_farmer,
                           caravel_id )
          .id();

  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( expert_farmer_id ) );

  destroy_unit( W.ss(), expert_farmer_id );

  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE_FALSE( W.units().exists( expert_farmer_id ) );

  destroy_unit( W.ss(), caravel_id );

  REQUIRE_FALSE( W.units().exists( caravel_id ) );
  REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  REQUIRE_FALSE( W.units().exists( expert_farmer_id ) );
}

TEST_CASE( "[unit-mgr] unit_ownership_change_non_interactive" ) {
  World       W;
  Unit const& free_colonist = W.add_unit_on_map(
      e_unit_type::free_colonist, { .x = 1, .y = 1 } );
  Unit const& missionary = W.add_unit_on_map(
      e_unit_type::missionary, { .x = 3, .y = 0 } );
  Unit const& caravel = W.add_unit_on_map( e_unit_type::caravel,
                                           { .x = 0, .y = 0 } );
  EuroUnitOwnershipChangeTo change_to;
  UnitOwnership             expected;

  auto ownership = [&]( UnitId unit_id ) {
    return as_const( W.units() ).ownership_of( unit_id );
  };

  auto f = [&]( UnitId unit_id ) {
    unit_ownership_change_non_interactive( W.ss(), unit_id,
                                           change_to );
    return ownership( unit_id );
  };

  expected = UnitOwnership::world{ .coord = { .x = 1, .y = 1 } };
  REQUIRE( ownership( free_colonist.id() ) == expected );

  // Free.
  change_to = EuroUnitOwnershipChangeTo::free{};
  expected  = UnitOwnership::free{};
  REQUIRE( f( free_colonist.id() ) == expected );

  // World.
  change_to = EuroUnitOwnershipChangeTo::world{
      .ts = &W.ts(), .target = { .x = 1, .y = 2 } };
  expected = UnitOwnership::world{ .coord = { .x = 1, .y = 2 } };
  REQUIRE( f( free_colonist.id() ) == expected );

  // Colony.
  Colony& colony = W.add_colony( { .x = 1, .y = 0 } );
  change_to      = EuroUnitOwnershipChangeTo::colony_low_level{
           .colony_id = colony.id };
  expected = UnitOwnership::colony{ .id = colony.id };
  REQUIRE( f( free_colonist.id() ) == expected );

  // Cargo.
  change_to = EuroUnitOwnershipChangeTo::cargo{
      .new_holder = caravel.id(), .starting_slot = 0 };
  expected = UnitOwnership::cargo{ .holder = caravel.id() };
  REQUIRE( f( free_colonist.id() ) == expected );

  // Dwelling.
  Dwelling& dwelling =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::iroquois );
  change_to = EuroUnitOwnershipChangeTo::dwelling{
      .dwelling_id = dwelling.id };
  expected = UnitOwnership::dwelling{ .id = dwelling.id };
  REQUIRE( f( missionary.id() ) == expected );

  // Sail to harbor.
  change_to = EuroUnitOwnershipChangeTo::sail_to_harbor{};
  expected  = UnitOwnership::harbor{
       .st = UnitHarborViewState{
           .port_status = PortStatus::inbound{ .turns = 0 },
           .sailed_from = Coord{ .x = 0, .y = 0 } } };
  REQUIRE( f( caravel.id() ) == expected );

  // Sail to new_world.
  change_to = EuroUnitOwnershipChangeTo::sail_to_new_world{};
  expected  = UnitOwnership::harbor{
       .st = UnitHarborViewState{
           .port_status = PortStatus::outbound{ .turns = 4 },
           .sailed_from = Coord{ .x = 0, .y = 0 } } };
  REQUIRE( f( caravel.id() ) == expected );

  // Move to port.
  change_to = EuroUnitOwnershipChangeTo::move_to_port{};
  expected  = UnitOwnership::harbor{
       .st = UnitHarborViewState{
           .port_status = PortStatus::in_port{},
           .sailed_from = Coord{ .x = 0, .y = 0 } } };
  REQUIRE( f( caravel.id() ) == expected );
}

#ifndef COMPILER_GCC
TEST_CASE( "[unit-mgr] unit_ownership_change" ) {
  World       W;
  Unit const& unit =
      W.add_free_unit( e_unit_type::free_colonist );
  EuroUnitOwnershipChangeTo change_to;
  UnitOwnership             expected;

  auto ownership = [&] {
    return as_const( W.units() ).ownership_of( unit.id() );
  };

  auto f = [&] {
    wait<maybe<UnitDeleted>> const w =
        unit_ownership_change( W.ss(), unit.id(), change_to );
    BASE_CHECK( !w.exception() );
    BASE_CHECK( w.ready() );
    BASE_CHECK( !w->has_value() ); // unit not deleted.
  };

  // World.
  change_to = EuroUnitOwnershipChangeTo::world{
      .ts = &W.ts(), .target = { .x = 0, .y = 1 } };
  expected = UnitOwnership::world{ .coord = { .x = 0, .y = 1 } };
  // We'll allow these async events to trigger so that we can
  // verify that we're calling the interactive version of the
  // function that moves a unit onto a map square.
  W.gui()
      .EXPECT__display_woodcut( e_woodcut::discovered_new_world )
      .returns();
  // Player asked to name the new world.
  W.gui().EXPECT__string_input( _, _ ).returns<maybe<string>>(
      "my land" );
  f();
  REQUIRE( ownership() == expected );
}
#endif

} // namespace
} // namespace rn
