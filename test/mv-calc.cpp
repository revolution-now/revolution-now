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

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// Revolution Now
#include "src/unit-mgr.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::Approx;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_player( e_nation::dutch ); }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[src/mv-calc] can_unit_move_based_on_mv_points" ) {
  World W;

  Unit                   unit;
  MovementPointsAnalysis res, expected;
  MovementPoints         needed;

  auto f = [&] {
    return can_unit_move_based_on_mv_points(
        W.ts(), W.default_player(), unit, needed );
  };

  auto create = [&]( e_unit_type type ) {
    return create_unregistered_unit( W.default_player(), type );
  };

  SECTION( "has=0, needed=0" ) {
    unit = create( e_unit_type::free_colonist );
    unit.forfeight_mv_points();
    needed   = MovementPoints( 0 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 0 ) );
  }

  SECTION( "has=1, needed=0" ) {
    unit     = create( e_unit_type::free_colonist );
    needed   = MovementPoints( 0 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 0 ) );
  }

  SECTION( "has=1, needed=1" ) {
    unit     = create( e_unit_type::free_colonist );
    needed   = MovementPoints( 1 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 1 ) );
  }

  SECTION( "has=1/3, needed=1/3" ) {
    unit = create( e_unit_type::free_colonist );
    unit.consume_mv_points( MovementPoints::_2_3() );
    needed   = MovementPoints::_1_3();
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() ==
             MovementPoints::_1_3() );
  }

  SECTION( "has=1/3, needed=0" ) {
    unit = create( e_unit_type::free_colonist );
    unit.consume_mv_points( MovementPoints::_2_3() );
    needed   = MovementPoints( 0 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 0 ) );
  }

  SECTION( "has=1, needed=2, start of turn" ) {
    unit     = create( e_unit_type::free_colonist );
    needed   = MovementPoints( 2 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = true,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 1 ) );
  }

  SECTION( "has=2/3, needed=1, overdraw denied" ) {
    unit = create( e_unit_type::free_colonist );
    unit.consume_mv_points( MovementPoints::_1_3() );
    needed   = MovementPoints( 1 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    W.rand()
        .EXPECT__bernoulli( Approx( .666666, .00001 ) )
        .returns( false );
    res = f();
    REQUIRE( res == expected );
    REQUIRE( !res.allowed() );
    REQUIRE( res.points_to_subtract() ==
             MovementPoints::_2_3() );
  }

  SECTION( "has=2/3, needed=1, overdraw allowed" ) {
    unit = create( e_unit_type::free_colonist );
    unit.consume_mv_points( MovementPoints::_1_3() );
    needed   = MovementPoints( 1 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = true };
    W.rand()
        .EXPECT__bernoulli( Approx( .666666, .00001 ) )
        .returns( true );
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() ==
             MovementPoints::_2_3() );
  }

  SECTION( "has=4, needed=4" ) {
    unit     = create( e_unit_type::scout );
    needed   = MovementPoints( 4 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 4 ) );
  }

  SECTION( "has=4, needed=5, start of turn" ) {
    unit     = create( e_unit_type::scout );
    needed   = MovementPoints( 5 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = true,
        .using_overdraw_allowance      = false };
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 4 ) );
  }

  SECTION( "has=2, needed=5, overdraw denied" ) {
    unit = create( e_unit_type::scout );
    unit.consume_mv_points( MovementPoints( 2 ) );
    needed   = MovementPoints( 5 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = false };
    W.rand()
        .EXPECT__bernoulli( Approx( .4, .00001 ) )
        .returns( false );
    res = f();
    REQUIRE( res == expected );
    REQUIRE( !res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 2 ) );
  }

  SECTION( "has=2, needed=5, overdraw allowed" ) {
    unit = create( e_unit_type::scout );
    unit.consume_mv_points( MovementPoints( 2 ) );
    needed   = MovementPoints( 5 );
    expected = MovementPointsAnalysis{
        .has                           = unit.movement_points(),
        .needed                        = needed,
        .using_start_of_turn_exemption = false,
        .using_overdraw_allowance      = true };
    W.rand()
        .EXPECT__bernoulli( Approx( .4, .00001 ) )
        .returns( true );
    res = f();
    REQUIRE( res == expected );
    REQUIRE( res.allowed() );
    REQUIRE( res.points_to_subtract() == MovementPoints( 2 ) );
  }
}

} // namespace
} // namespace rn
