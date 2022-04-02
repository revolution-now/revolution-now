/****************************************************************
**mv-calc.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-28.
*
* Description: Unit tests for the src/mv-calc.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/mv-calc.hpp"

// Revolution Now
#include "src/ustate.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[src/mv-calc] expense" ) {
  MapSquare ocean = map_square_for_terrain( e_terrain::ocean );
  MapSquare grassland =
      map_square_for_terrain( e_terrain::grassland );
  MapSquare mountains =
      map_square_for_terrain( e_terrain::mountains );
  MapSquare grassland_with_road =
      map_square_for_terrain( e_terrain::grassland );
  grassland_with_road.road = true;
  MapSquare mountains_with_road =
      map_square_for_terrain( e_terrain::mountains );
  mountains_with_road.road = true;

  MapSquare grassland_with_road_and_river =
      map_square_for_terrain( e_terrain::grassland );
  grassland_with_road_and_river.road = true;
  MapSquare mountains_with_river =
      map_square_for_terrain( e_terrain::mountains );
  mountains_with_river.river = e_river::major;

  Unit privateer = create_free_unit(
      e_nation::english,
      UnitComposition::create( e_unit_type::privateer ) );
  Unit free_colonist = create_free_unit(
      e_nation::english,
      UnitComposition::create( e_unit_type::free_colonist ) );
  Unit dragoon = create_free_unit(
      e_nation::english,
      UnitComposition::create( e_unit_type::dragoon ) );

  reference_wrapper<MapSquare const> src  = ocean;
  reference_wrapper<MapSquare const> dst  = ocean;
  reference_wrapper<Unit>            unit = privateer;

  MovementPointsAnalysis expected, result;

  // Privateer, ocean to ocean.
  src      = ocean;
  dst      = ocean;
  unit     = privateer;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints( 8 ),
      .needs                       = MovementPoints( 1 ),
      .has_start_of_turn_exemption = true,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() == MovementPoints( 7 ) );

  // For the colonist at 1 movement points.
  Unit forked_colonist_1 = free_colonist;

  // Free colonist, road to road.
  src      = grassland_with_road_and_river;
  dst      = grassland_with_road_and_river;
  unit     = free_colonist;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints( 1 ),
      .needs                       = MovementPoints::_1_3(),
      .has_start_of_turn_exemption = true,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() ==
           MovementPoints::_2_3() );

  // For the colonist at 2/3 movement points.
  Unit forked_colonist_2_3 = free_colonist;

  // Continue with original colonist.
  src      = grassland_with_road_and_river;
  dst      = grassland_with_road_and_river;
  unit     = free_colonist;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints::_2_3(),
      .needs                       = MovementPoints::_1_3(),
      .has_start_of_turn_exemption = false,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() ==
           MovementPoints::_1_3() );

  // For the colonist at 1/3 movement points.
  Unit forked_colonist_1_3 = free_colonist;

  // Continue with original colonist.
  src      = grassland_with_road_and_river;
  dst      = grassland_with_road_and_river;
  unit     = free_colonist;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints::_1_3(),
      .needs                       = MovementPoints::_1_3(),
      .has_start_of_turn_exemption = false,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() == 0 );

  // Continue with the 1/3 colonist.
  src      = grassland_with_road_and_river;
  dst      = grassland;
  unit     = forked_colonist_1_3;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints::_1_3(),
      .needs                       = MovementPoints( 1 ),
      .has_start_of_turn_exemption = false,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( !result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  REQUIRE( unit.get().movement_points() ==
           MovementPoints::_1_3() );

  // Continue with the 2/3 colonist.
  src      = grassland_with_road_and_river;
  dst      = grassland;
  unit     = forked_colonist_2_3;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints::_2_3(),
      .needs                       = MovementPoints( 1 ),
      .has_start_of_turn_exemption = false,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == true );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() == MovementPoints( 0 ) );

  // Continue with the 1-point colonist.
  src      = grassland;
  dst      = mountains;
  unit     = forked_colonist_1;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints( 1 ),
      .needs                       = MovementPoints( 3 ),
      .has_start_of_turn_exemption = true,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == true );
  REQUIRE( result.using_overdraw_allowance() == false );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() == MovementPoints( 0 ) );

  // Dragoon with two movement points.
  src      = grassland;
  dst      = mountains;
  unit     = dragoon;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints( 4 ),
      .needs                       = MovementPoints( 3 ),
      .has_start_of_turn_exemption = true,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  unit.get().consume_mv_points( result.points_to_subtract() );
  REQUIRE( unit.get().movement_points() == MovementPoints( 1 ) );

  // Continue with dragoon.
  src      = grassland;
  dst      = mountains;
  unit     = dragoon;
  expected = MovementPointsAnalysis{
      .has                         = MovementPoints( 1 ),
      .needs                       = MovementPoints( 3 ),
      .has_start_of_turn_exemption = false,
  };
  result = expense_movement_points( unit, src, dst );
  REQUIRE( result == expected );
  REQUIRE( !result.allowed() );
  REQUIRE( result.using_start_of_turn_exemption() == false );
  REQUIRE( result.using_overdraw_allowance() == false );
  REQUIRE( unit.get().movement_points() == MovementPoints( 1 ) );
}

} // namespace
} // namespace rn
