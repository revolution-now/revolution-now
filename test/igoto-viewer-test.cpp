/****************************************************************
**igoto-viewer-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Unit tests for the igoto-viewer module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/igoto-viewer.hpp"

// Testing.
#include "test/mocks/igoto-viewer.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[igoto-viewer] travel_cost" ) {
  MockIGotoMapViewer viewer;
  point src;
  e_direction d = {};

  auto const f = [&] [[clang::noinline]] {
    return viewer.travel_cost( src, d );
  };

  using enum e_direction;

  src = {};
  d   = e;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      MovementPoints( 2 ) );
  viewer.EXPECT__has_lcr( point{ .x = 1, .y = 0 } )
      .returns( false );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( false );
  REQUIRE( f() == 2 * 3 );

  src = { .x = 3, .y = 5 };
  d   = e;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      MovementPoints::_1_3() );
  viewer.EXPECT__has_lcr( point{ .x = 4, .y = 5 } )
      .returns( false );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( false );
  REQUIRE( f() == 1 );

  src = { .x = 7, .y = 1 };
  d   = nw;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      MovementPoints::_1_3() );
  viewer.EXPECT__has_lcr( point{ .x = 6, .y = 0 } )
      .returns( true );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( false );
  REQUIRE( f() == 1 + 4 * 3 );

  src = { .x = 1, .y = 1 };
  d   = s;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      nothing );
  viewer.EXPECT__has_lcr( point{ .x = 1, .y = 2 } )
      .returns( true );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( false );
  REQUIRE( f() == 1 * 3 + 4 * 3 );

  src = { .x = 40, .y = 55 };
  d   = se;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      nothing );
  viewer.EXPECT__has_lcr( point{ .x = 41, .y = 56 } )
      .returns( false );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( false );
  REQUIRE( f() == 1 * 3 );

  src = {};
  d   = e;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      MovementPoints( 2 ) );
  viewer.EXPECT__has_lcr( point{ .x = 1, .y = 0 } )
      .returns( false );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( true );
  viewer.EXPECT__ends_turn_in_colony().returns( false );
  REQUIRE( f() == 2 * 3 );

  src = {};
  d   = e;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      MovementPoints( 2 ) );
  viewer.EXPECT__has_lcr( point{ .x = 1, .y = 0 } )
      .returns( false );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( false );
  REQUIRE( f() == 2 * 3 );

  src = {};
  d   = e;
  viewer.EXPECT__movement_points_required( src, d ).returns(
      MovementPoints( 2 ) );
  viewer.EXPECT__has_lcr( point{ .x = 1, .y = 0 } )
      .returns( false );
  viewer.EXPECT__has_colony( src.moved( d ) ).returns( true );
  viewer.EXPECT__ends_turn_in_colony().returns( true );
  REQUIRE( f() == ( 2 + 1000 ) * 3 );
}

TEST_CASE( "[igoto-viewer] heuristic_cost" ) {
  MockIGotoMapViewer viewer;
  point src, dst;

  auto const f = [&] [[clang::noinline]] {
    return viewer.heuristic_cost( src, dst );
  };

  src = {};
  dst = {};
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 0 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 0 );

  src = { .x = -2, .y = 4 };
  dst = { .x = 5, .y = 8 };
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 7 * 3 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 7 * 1 );

  src = { .x = -2, .y = 4 };
  dst = { .x = 5, .y = 11 };
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 7 * 3 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 7 * 1 );

  src = { .x = -2, .y = 4 };
  dst = { .x = 5, .y = 12 };
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints( 1 ) );
  REQUIRE( f() == 8 * 3 );
  viewer.EXPECT__minimum_heuristic_tile_cost().returns(
      MovementPoints::_1_3() );
  REQUIRE( f() == 8 * 1 );
}

TEST_CASE( "[igoto-viewer] is_sea_lane_launch_point" ) {
  MockIGotoMapViewer viewer;
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return viewer.is_sea_lane_launch_point( tile );
  };

  using enum e_map_side_edge;
  using enum e_direction;

  tile = { .x = 3, .y = 5 };

  viewer.EXPECT__is_on_map_side_edge( tile ).returns( atlantic );
  REQUIRE( f() == e );
  viewer.EXPECT__is_on_map_side_edge( tile ).returns( pacific );
  REQUIRE( f() == w );

  // From here on we are not on the edge.
  viewer.EXPECT__is_on_map_side_edge( tile )
      .by_default()
      .returns( none );

  SECTION( "pacific succeeds" ) {
    viewer.EXPECT__map_side( tile ).by_default().returns(
        e_map_side::pacific );
    viewer.EXPECT__can_enter_tile( point{ .x = 2, .y = 5 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 2, .y = 4 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 2, .y = 6 } )
        .returns( true );
    viewer.EXPECT__is_sea_lane( point{ .x = 2, .y = 6 } )
        .returns( true );
    REQUIRE( f() == sw );
  }

  SECTION( "pacific fails" ) {
    viewer.EXPECT__map_side( tile ).by_default().returns(
        e_map_side::pacific );
    viewer.EXPECT__can_enter_tile( point{ .x = 2, .y = 5 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 2, .y = 4 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 2, .y = 6 } )
        .returns( false );
    REQUIRE( f() == nothing );
  }

  SECTION( "atlantic succeeds" ) {
    viewer.EXPECT__map_side( tile ).by_default().returns(
        e_map_side::atlantic );
    viewer.EXPECT__can_enter_tile( point{ .x = 3, .y = 5 } )
        .returns( true );
    viewer.EXPECT__is_sea_lane( point{ .x = 3, .y = 5 } )
        .returns( true );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 5 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 4 } )
        .returns( true );
    viewer.EXPECT__is_sea_lane( point{ .x = 4, .y = 4 } )
        .returns( true );
    REQUIRE( f() == ne );
  }

  SECTION( "atlantic fails here" ) {
    viewer.EXPECT__map_side( tile ).by_default().returns(
        e_map_side::atlantic );
    viewer.EXPECT__can_enter_tile( point{ .x = 3, .y = 5 } )
        .returns( false );
    REQUIRE( f() == nothing );
  }

  SECTION( "atlantic fails / no east hidden" ) {
    viewer.EXPECT__map_side( tile ).by_default().returns(
        e_map_side::atlantic );
    viewer.EXPECT__can_enter_tile( point{ .x = 3, .y = 5 } )
        .returns( true );
    viewer.EXPECT__is_sea_lane( point{ .x = 3, .y = 5 } )
        .returns( true );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 5 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 4 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 6 } )
        .returns( false );
    viewer.EXPECT__is_sea_lane( point{ .x = 4, .y = 5 } )
        .returns( false );
    REQUIRE( f() == nothing );
  }

  SECTION( "atlantic succeeds / east hidden" ) {
    viewer.EXPECT__map_side( tile ).by_default().returns(
        e_map_side::atlantic );
    viewer.EXPECT__can_enter_tile( point{ .x = 3, .y = 5 } )
        .returns( true );
    viewer.EXPECT__is_sea_lane( point{ .x = 3, .y = 5 } )
        .returns( true );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 5 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 4 } )
        .returns( false );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 6 } )
        .returns( false );
    viewer.EXPECT__is_sea_lane( point{ .x = 4, .y = 5 } )
        .returns( nothing );
    viewer.EXPECT__can_enter_tile( point{ .x = 4, .y = 5 } )
        .returns( true );
    REQUIRE( f() == e );
  }
}

} // namespace
} // namespace rn
