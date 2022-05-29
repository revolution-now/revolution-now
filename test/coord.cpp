/****************************************************************
**coord.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-23.
*
* Description: Unit tests for the coord module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "coord.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "catch-common.hpp"

namespace {

using namespace std;
using namespace rn;

TEST_CASE( "[e_direction] direction type*" ) {
  REQUIRE( direction_type( e_direction::nw ) ==
           e_direction_type::diagonal );
  REQUIRE( direction_type( e_direction::ne ) ==
           e_direction_type::diagonal );
  REQUIRE( direction_type( e_direction::sw ) ==
           e_direction_type::diagonal );
  REQUIRE( direction_type( e_direction::se ) ==
           e_direction_type::diagonal );
  REQUIRE( direction_type( e_direction::n ) ==
           e_direction_type::cardinal );
  REQUIRE( direction_type( e_direction::w ) ==
           e_direction_type::cardinal );
  REQUIRE( direction_type( e_direction::e ) ==
           e_direction_type::cardinal );
  REQUIRE( direction_type( e_direction::s ) ==
           e_direction_type::cardinal );
  REQUIRE( direction_type( e_direction::c ) ==
           e_direction_type::center );
}

TEST_CASE( "[coord] centered*" ) {
  Rect  rect;
  Delta delta;
  Coord expect;

  rect   = Rect{ 1_x, 1_y, 0_w, 0_h };
  delta  = Delta{ 4_w, 3_h };
  expect = Coord{ -1_x, 0_y };
  REQUIRE( centered( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 3_y };
  REQUIRE( centered( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 4_y };
  REQUIRE( centered_bottom( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 0_y };
  REQUIRE( centered_top( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 0_x, 3_y };
  REQUIRE( centered_left( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 4_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 3_y };
  REQUIRE( centered_right( delta, rect ) == expect );
}

TEST_CASE( "[coord] centered_on" ) {
  Coord coord;
  Rect  rect;
  Rect  expect;

  // Zero size.
  rect   = Rect{ 0_x, 0_y, 0_w, 0_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 10_x, 10_y, 0_w, 0_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ 0_x, 0_y, 0_w, 0_h };
  coord  = Coord{ -1_x, -2_y };
  expect = Rect{ -1_x, -2_y, 0_w, 0_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  // Small size.
  rect   = Rect{ 5_x, 5_y, 1_w, 2_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 10_x, 9_y, 1_w, 2_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ 5_x, 5_y, 2_w, 1_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 9_x, 10_y, 2_w, 1_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  // Large size.
  rect   = Rect{ 5_x, 5_y, 5_w, 5_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 8_x, 8_y, 5_w, 5_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ 100_x, 5_y, 300_w, 150_h };
  coord  = Coord{ -8_x, -2_y };
  expect = Rect{ -158_x, -77_y, 300_w, 150_h };
  REQUIRE( rect.centered_on( coord ) == expect );
}

TEST_CASE( "[coord] as_if_origin_were" ) {
  auto rect   = Rect{ 5_x, 5_y, 7_w, 9_h };
  auto coord  = Coord{ 10_x, 10_y };
  auto expect = Rect{ 15_x, 15_y, 7_w, 9_h };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );

  rect   = Rect{ 2_x, 3_y, 7_w, 9_h };
  coord  = Coord{ 1_x, 2_y };
  expect = Rect{ 3_x, 5_y, 7_w, 9_h };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );

  rect   = Rect{ 2_x, 3_y, 1_w, 1_h };
  coord  = Coord{ -1_x, -2_y };
  expect = Rect{ 1_x, 1_y, 1_w, 1_h };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_right_edge" ) {
  auto rect = Rect{ 5_x, 5_y, 7_w, 9_h };
  X    new_edge{};
  Rect expect{};

  new_edge = 7_x;
  expect   = Rect{ 5_x, 5_y, 2_w, 9_h };
  REQUIRE( rect.with_new_right_edge( new_edge ) == expect );

  new_edge = 50_x;
  expect   = Rect{ 5_x, 5_y, 45_w, 9_h };
  REQUIRE( rect.with_new_right_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_left_edge" ) {
  auto rect = Rect{ 5_x, 5_y, 7_w, 9_h };
  X    new_edge{};
  Rect expect{};

  new_edge = 7_x;
  expect   = Rect{ 7_x, 5_y, 5_w, 9_h };
  REQUIRE( rect.with_new_left_edge( new_edge ) == expect );

  new_edge = 3_x;
  expect   = Rect{ 3_x, 5_y, 9_w, 9_h };
  REQUIRE( rect.with_new_left_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_top_edge" ) {
  auto rect = Rect{ 5_x, 5_y, 7_w, 9_h };
  Y    new_edge{};
  Rect expect{};

  new_edge = 7_y;
  expect   = Rect{ 5_x, 7_y, 7_w, 7_h };
  REQUIRE( rect.with_new_top_edge( new_edge ) == expect );

  new_edge = 3_y;
  expect   = Rect{ 5_x, 3_y, 7_w, 11_h };
  REQUIRE( rect.with_new_top_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_bottom_edge" ) {
  auto rect = Rect{ 5_x, 5_y, 7_w, 9_h };
  Y    new_edge{};
  Rect expect{};

  new_edge = 7_y;
  expect   = Rect{ 5_x, 5_y, 7_w, 2_h };
  REQUIRE( rect.with_new_bottom_edge( new_edge ) == expect );

  new_edge = 50_y;
  expect   = Rect{ 5_x, 5_y, 7_w, 45_h };
  REQUIRE( rect.with_new_bottom_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::normalized" ) {
  Rect rect, expect;

  rect   = Rect{ 5_x, 5_y, 0_w, 0_h };
  expect = rect;
  REQUIRE( rect.normalized() == expect );

  rect   = Rect{ 5_x, 5_y, 7_w, 9_h };
  expect = rect;
  REQUIRE( rect.normalized() == expect );

  rect   = Rect{ 5_x, 5_y, -2_w, -1_h };
  expect = Rect{ 3_x, 4_y, 2_w, 1_h };
  REQUIRE( rect.normalized() == expect );
}

TEST_CASE( "[coord] rounded_to_multiple_to_minus_inf" ) {
  Delta delta;
  Coord coord;
  Coord expect;

  delta  = Delta{ 0_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 0_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 10_x, 8_y };
  expect = Coord{ 10_x, 8_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ -10_x, -8_y };
  expect = Coord{ -10_x, -8_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 3_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 15_y };
  expect = Coord{ 0_x, 10_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 20_x, 20_y };
  expect = Coord{ 20_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 23_x, 29_y };
  expect = Coord{ 20_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, 29_y };
  expect = Coord{ -10_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, -29_y };
  expect = Coord{ -10_x, -30_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );
}

TEST_CASE( "[coord] rounded_to_multiple_to_plus_inf" ) {
  Delta delta;
  Coord coord;
  Coord expect;

  delta  = Delta{ 0_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 0_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 10_x, 8_y };
  expect = Coord{ 10_x, 8_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ -10_x, -8_y };
  expect = Coord{ -10_x, -8_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 3_y };
  expect = Coord{ 10_x, 10_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 15_y };
  expect = Coord{ 10_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 20_x, 20_y };
  expect = Coord{ 20_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 23_x, 29_y };
  expect = Coord{ 30_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, 29_y };
  expect = Coord{ 0_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -7_x, 29_y };
  expect = Coord{ 0_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -2_x, 29_y };
  expect = Coord{ 0_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, -29_y };
  expect = Coord{ 0_x, -20_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );
}

TEST_CASE( "[coord] to gfx" ) {
  Delta d( W{ 5 }, H{ 6 } );
  Coord c( X{ 5 }, Y{ 6 } );
  Rect  r = Rect::from( Coord( X{ 1 }, Y{ 2 } ),
                        Delta( W{ 5 }, H{ 6 } ) );

  REQUIRE( d == gfx::size{ .w = 5, .h = 6 } );
  REQUIRE( c == gfx::point{ .x = 5, .y = 6 } );
  REQUIRE( r == gfx::rect{ .origin = { .x = 1, .y = 2 },
                           .size   = { .w = 5, .h = 6 } } );
}

TEST_CASE( "[coord] from gfx" ) {
  Delta d( W{ 5 }, H{ 6 } );
  Coord c( X{ 5 }, Y{ 6 } );
  Rect  r = Rect::from( Coord( X{ 1 }, Y{ 2 } ),
                        Delta( W{ 5 }, H{ 6 } ) );

  gfx::size  d_gfx{ .w = 5, .h = 6 };
  gfx::point c_gfx{ .x = 5, .y = 6 };
  gfx::rect  r_gfx = gfx::rect{ .origin = { .x = 1, .y = 2 },
                                .size   = { .w = 5, .h = 6 } };

  REQUIRE( Delta::from_gfx( d_gfx ) == d );
  REQUIRE( Coord::from_gfx( c_gfx ) == c );
  REQUIRE( Rect::from_gfx( r_gfx ) == r );
}

TEST_CASE( "[coord] Rect::with_new_origin" ) {
  Rect rect = Rect::from( Coord( 4_x, 5_y ), Delta( 2_w, 3_h ) );
  Coord new_origin( 1_x, 3_y );
  Rect  expected =
      Rect::from( Coord( 3_x, 2_y ), Delta( 2_w, 3_h ) );
  REQUIRE( rect.with_new_origin( new_origin ) == expected );
}

} // namespace
