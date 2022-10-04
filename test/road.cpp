/****************************************************************
**road.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-26.
*
* Description: Unit tests for the src/test/road.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/road.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "src/imap-updater.hpp"
#include "src/map-square.hpp"
#include "src/on-map.hpp"
#include "src/ustate.hpp"

// ss
#include "src/ss/terrain.hpp"
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using Catch::Contains;

Coord const kSquare{};

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {}

  void initialize( e_unit_type unit_type ) {
    add_default_player();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );

    add_unit_on_map( unit_type, Coord{} );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[road] perform_road_work 100 tools" ) {
  World W;
  W.initialize( e_unit_type::pioneer );

  UnitId id       = 1;
  Unit&  unit     = W.units().unit_for( id );
  Coord  location = W.units().coord_for( id );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Before starting road work.
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 3;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( W.units(), W.terrain(), W.map_updater(),
                       unit );
    REQUIRE( has_road( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( W.units(), W.terrain(), W.map_updater(),
                     unit );
  REQUIRE( has_road( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[road] perform_road_work hardy_pioneer" ) {
  World W;
  W.initialize( e_unit_type::hardy_pioneer );

  UnitId id       = 1;
  Unit&  unit     = W.units().unit_for( id );
  Coord  location = W.units().coord_for( id );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( location == kSquare );

  // Before starting road work.
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 1;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( W.units(), W.terrain(), W.map_updater(),
                       unit );
    REQUIRE( has_road( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( W.units(), W.terrain(), W.map_updater(),
                     unit );
  REQUIRE( has_road( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 80 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[road] perform_road_work 20 tools" ) {
  World W;
  W.initialize( e_unit_type::pioneer );

  UnitId id       = 1;
  Unit&  unit     = W.units().unit_for( id );
  Coord  location = W.units().coord_for( id );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();

  // Before starting road work.
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 3;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( W.units(), W.terrain(), W.map_updater(),
                       unit );
    REQUIRE( has_road( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( W.units(), W.terrain(), W.map_updater(),
                     unit );
  REQUIRE( has_road( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::free_colonist );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[road] perform_road_work hardy_pioneer 20 tools" ) {
  World W;
  W.initialize( e_unit_type::hardy_pioneer );

  UnitId id       = 1;
  Unit&  unit     = W.units().unit_for( id );
  Coord  location = W.units().coord_for( id );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( location == kSquare );

  // Take away most of the units tools.
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();
  unit.consume_20_tools();

  // Before starting road work.
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 1;

  // Do the work.
  for( int i = 0; i < kTurnsRequired; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( W.units(), W.terrain(), W.map_updater(),
                       unit );
    REQUIRE( has_road( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] == 20 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Finished.
  unit.new_turn();
  perform_road_work( W.units(), W.terrain(), W.map_updater(),
                     unit );
  REQUIRE( has_road( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::hardy_colonist );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 0 );
  REQUIRE( unit.movement_points() == 1 );
}

TEST_CASE( "[road] perform_road_work with cancel" ) {
  World W;
  W.initialize( e_unit_type::pioneer );

  UnitId id       = 1;
  Unit&  unit     = W.units().unit_for( id );
  Coord  location = W.units().coord_for( id );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( location == kSquare );

  // Before starting road work.
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  // Tell unit to start road work.
  unit.build_road();
  unit.set_turns_worked( 0 );
  REQUIRE( has_road( W.terrain(), kSquare ) == false );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::road );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );

  int const kTurnsRequired = 4;

  // Do part of the work.
  for( int i = 0; i < kTurnsRequired - 2; ++i ) {
    INFO( fmt::format( "i={}", i ) );
    unit.new_turn();
    perform_road_work( W.units(), W.terrain(), W.map_updater(),
                       unit );
    REQUIRE( has_road( W.terrain(), kSquare ) == false );
    REQUIRE( unit.type() == e_unit_type::pioneer );
    REQUIRE( unit.turns_worked() == i + 1 );
    REQUIRE( unit.orders() == e_unit_orders::road );
    REQUIRE( unit.composition()[e_unit_inventory::tools] ==
             100 );
    REQUIRE( unit.movement_points() == 0 );
  }

  // Effectively cancel it by putting a road on the tile.
  set_road( W.map_updater(), kSquare );

  // Cancelled.
  unit.new_turn();
  perform_road_work( W.units(), W.terrain(), W.map_updater(),
                     unit );
  REQUIRE( has_road( W.terrain(), kSquare ) == true );
  REQUIRE( unit.type() == e_unit_type::pioneer );
  REQUIRE( unit.turns_worked() == 0 );
  REQUIRE( unit.orders() == e_unit_orders::none );
  REQUIRE( unit.composition()[e_unit_inventory::tools] == 100 );
  REQUIRE( unit.movement_points() == 1 );
}

} // namespace
} // namespace rn
