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
#include "test/testing.hpp"

// gfx
#include "src/gfx/coord.hpp"

// refl
#include "src/refl/to-str.hpp"

// C++ standard library
#include <unordered_set>

// Must be last.
#include "test/catch-common.hpp"

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
}

TEST_CASE( "[e_direction] to_diagonal*" ) {
  REQUIRE( to_diagonal( e_direction::nw ) ==
           e_diagonal_direction::nw );
  REQUIRE( to_diagonal( e_direction::ne ) ==
           e_diagonal_direction::ne );
  REQUIRE( to_diagonal( e_direction::sw ) ==
           e_diagonal_direction::sw );
  REQUIRE( to_diagonal( e_direction::se ) ==
           e_diagonal_direction::se );
  REQUIRE( to_diagonal( e_direction::n ) == base::nothing );
  REQUIRE( to_diagonal( e_direction::w ) == base::nothing );
  REQUIRE( to_diagonal( e_direction::e ) == base::nothing );
  REQUIRE( to_diagonal( e_direction::s ) == base::nothing );
}

TEST_CASE( "[e_direction] to_direction*" ) {
  REQUIRE( to_direction( e_diagonal_direction::nw ) ==
           e_direction::nw );
  REQUIRE( to_direction( e_diagonal_direction::ne ) ==
           e_direction::ne );
  REQUIRE( to_direction( e_diagonal_direction::sw ) ==
           e_direction::sw );
  REQUIRE( to_direction( e_diagonal_direction::se ) ==
           e_direction::se );
  REQUIRE( to_direction( e_cardinal_direction::n ) ==
           e_direction::n );
  REQUIRE( to_direction( e_cardinal_direction::w ) ==
           e_direction::w );
  REQUIRE( to_direction( e_cardinal_direction::e ) ==
           e_direction::e );
  REQUIRE( to_direction( e_cardinal_direction::s ) ==
           e_direction::s );
}

TEST_CASE( "[coord] lexical ordering*" ) {
  REQUIRE( Coord{ .x = 10, .y = 1 } < Coord{ .x = 20, .y = 2 } );
  REQUIRE( Coord{ .x = 10, .y = 2 } < Coord{ .x = 20, .y = 2 } );
  REQUIRE(
      !( Coord{ .x = 20, .y = 2 } < Coord{ .x = 10, .y = 2 } ) );
  REQUIRE(
      !( Coord{ .x = 10, .y = 2 } < Coord{ .x = 10, .y = 2 } ) );

  REQUIRE( Coord{ .x = 20, .y = 2 } > Coord{ .x = 10, .y = 1 } );
  REQUIRE( Coord{ .x = 20, .y = 2 } > Coord{ .x = 10, .y = 2 } );
  REQUIRE(
      !( Coord{ .x = 10, .y = 2 } > Coord{ .x = 20, .y = 2 } ) );
  REQUIRE(
      !( Coord{ .x = 10, .y = 2 } > Coord{ .x = 10, .y = 2 } ) );
}

TEST_CASE( "[coord] centered*" ) {
  Rect  rect;
  Delta delta;
  Coord expect;

  rect   = Rect{ .x = 1, .y = 1, .w = 0, .h = 0 };
  delta  = Delta{ .w = 4, .h = 3 };
  expect = Coord{ .x = -1, .y = 0 };
  REQUIRE( centered( delta, rect ) == expect );

  rect   = Rect{ .x = 1, .y = 2, .w = 5, .h = 6 };
  delta  = Delta{ .w = 3, .h = 4 };
  expect = Coord{ .x = 2, .y = 3 };
  REQUIRE( centered( delta, rect ) == expect );

  rect   = Rect{ .x = 1, .y = 2, .w = 5, .h = 6 };
  delta  = Delta{ .w = 3, .h = 4 };
  expect = Coord{ .x = 2, .y = 4 };
  REQUIRE( centered_bottom( delta, rect ) == expect );

  rect   = Rect{ .x = 1, .y = 2, .w = 5, .h = 6 };
  delta  = Delta{ .w = 3, .h = 4 };
  expect = Coord{ .x = 2, .y = 0 };
  REQUIRE( centered_top( delta, rect ) == expect );

  rect   = Rect{ .x = 1, .y = 2, .w = 5, .h = 6 };
  delta  = Delta{ .w = 3, .h = 4 };
  expect = Coord{ .x = 0, .y = 3 };
  REQUIRE( centered_left( delta, rect ) == expect );

  rect   = Rect{ .x = 1, .y = 2, .w = 4, .h = 6 };
  delta  = Delta{ .w = 3, .h = 4 };
  expect = Coord{ .x = 2, .y = 3 };
  REQUIRE( centered_right( delta, rect ) == expect );
}

TEST_CASE( "[coord] Delta::abs" ) {
  Delta d{ .w = -6, .h = -2 };
  REQUIRE( d.abs() == Delta{ .w = 6, .h = 2 } );
}

TEST_CASE( "[coord] centered_on" ) {
  Coord coord;
  Rect  rect;
  Rect  expect;

  // Zero size.
  rect   = Rect{ .x = 0, .y = 0, .w = 0, .h = 0 };
  coord  = Coord{ .x = 10, .y = 10 };
  expect = Rect{ .x = 10, .y = 10, .w = 0, .h = 0 };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ .x = 0, .y = 0, .w = 0, .h = 0 };
  coord  = Coord{ .x = -1, .y = -2 };
  expect = Rect{ .x = -1, .y = -2, .w = 0, .h = 0 };
  REQUIRE( rect.centered_on( coord ) == expect );

  // Small size.
  rect   = Rect{ .x = 5, .y = 5, .w = 1, .h = 2 };
  coord  = Coord{ .x = 10, .y = 10 };
  expect = Rect{ .x = 10, .y = 9, .w = 1, .h = 2 };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ .x = 5, .y = 5, .w = 2, .h = 1 };
  coord  = Coord{ .x = 10, .y = 10 };
  expect = Rect{ .x = 9, .y = 10, .w = 2, .h = 1 };
  REQUIRE( rect.centered_on( coord ) == expect );

  // Large size.
  rect   = Rect{ .x = 5, .y = 5, .w = 5, .h = 5 };
  coord  = Coord{ .x = 10, .y = 10 };
  expect = Rect{ .x = 8, .y = 8, .w = 5, .h = 5 };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ .x = 100, .y = 5, .w = 300, .h = 150 };
  coord  = Coord{ .x = -8, .y = -2 };
  expect = Rect{ .x = -158, .y = -77, .w = 300, .h = 150 };
  REQUIRE( rect.centered_on( coord ) == expect );
}

TEST_CASE( "[coord] as_if_origin_were" ) {
  auto rect   = Rect{ .x = 5, .y = 5, .w = 7, .h = 9 };
  auto coord  = Coord{ .x = 10, .y = 10 };
  auto expect = Rect{ .x = 15, .y = 15, .w = 7, .h = 9 };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );

  rect   = Rect{ .x = 2, .y = 3, .w = 7, .h = 9 };
  coord  = Coord{ .x = 1, .y = 2 };
  expect = Rect{ .x = 3, .y = 5, .w = 7, .h = 9 };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );

  rect   = Rect{ .x = 2, .y = 3, .w = 1, .h = 1 };
  coord  = Coord{ .x = -1, .y = -2 };
  expect = Rect{ .x = 1, .y = 1, .w = 1, .h = 1 };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_right_edge" ) {
  auto rect = Rect{ .x = 5, .y = 5, .w = 7, .h = 9 };
  X    new_edge{};
  Rect expect{};

  new_edge = 7;
  expect   = Rect{ .x = 5, .y = 5, .w = 2, .h = 9 };
  REQUIRE( rect.with_new_right_edge( new_edge ) == expect );

  new_edge = 50;
  expect   = Rect{ 5, 5, 45, 9 };
  REQUIRE( rect.with_new_right_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_left_edge" ) {
  auto rect = Rect{ .x = 5, .y = 5, .w = 7, .h = 9 };
  X    new_edge{};
  Rect expect{};

  new_edge = 7;
  expect   = Rect{ .x = 7, .y = 5, .w = 5, .h = 9 };
  REQUIRE( rect.with_new_left_edge( new_edge ) == expect );

  new_edge = 3;
  expect   = Rect{ .x = 3, .y = 5, .w = 9, .h = 9 };
  REQUIRE( rect.with_new_left_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_top_edge" ) {
  auto rect = Rect{ .x = 5, .y = 5, .w = 7, .h = 9 };
  Y    new_edge{};
  Rect expect{};

  new_edge = 7;
  expect   = Rect{ .x = 5, .y = 7, .w = 7, .h = 7 };
  REQUIRE( rect.with_new_top_edge( new_edge ) == expect );

  new_edge = 3;
  expect   = Rect{ .x = 5, .y = 3, .w = 7, .h = 11 };
  REQUIRE( rect.with_new_top_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::with_new_bottom_edge" ) {
  auto rect = Rect{ .x = 5, .y = 5, .w = 7, .h = 9 };
  Y    new_edge{};
  Rect expect{};

  new_edge = 7;
  expect   = Rect{ .x = 5, .y = 5, .w = 7, .h = 2 };
  REQUIRE( rect.with_new_bottom_edge( new_edge ) == expect );

  new_edge = 50;
  expect   = Rect{ .x = 5, .y = 5, .w = 7, .h = 45 };
  REQUIRE( rect.with_new_bottom_edge( new_edge ) == expect );
}

TEST_CASE( "[coord] Rect::normalized" ) {
  Rect rect, expect;

  rect   = Rect{ .x = 5, .y = 5, .w = 0, .h = 0 };
  expect = rect;
  REQUIRE( rect.normalized() == expect );

  rect   = Rect{ .x = 5, .y = 5, .w = 7, .h = 9 };
  expect = rect;
  REQUIRE( rect.normalized() == expect );

  rect   = Rect{ .x = 5, .y = 5, .w = -2, .h = -1 };
  expect = Rect{ .x = 3, .y = 4, .w = 2, .h = 1 };
  REQUIRE( rect.normalized() == expect );
}

TEST_CASE( "[coord] rounded_to_multiple_to_minus_inf" ) {
  Delta delta;
  Coord coord;
  Coord expect;

  delta  = Delta{ .w = 0, .h = 1 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };

  delta  = Delta{ .w = 1, .h = 0 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };

  delta  = Delta{ .w = 1, .h = 1 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 1, .h = 1 };
  coord  = Coord{ .x = 10, .y = 8 };
  expect = Coord{ .x = 10, .y = 8 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 1, .h = 1 };
  coord  = Coord{ .x = -10, .y = -8 };
  expect = Coord{ .x = -10, .y = -8 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 5, .y = 3 };
  expect = Coord{ .x = 0, .y = 0 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 5, .y = 15 };
  expect = Coord{ .x = 0, .y = 10 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 20, .y = 20 };
  expect = Coord{ .x = 20, .y = 20 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 23, .y = 29 };
  expect = Coord{ .x = 20, .y = 20 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = -5, .y = 29 };
  expect = Coord{ .x = -10, .y = 20 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = -5, .y = -29 };
  expect = Coord{ .x = -10, .y = -30 };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );
}

TEST_CASE( "[coord] rounded_to_multiple_to_plus_inf" ) {
  Delta delta;
  Coord coord;
  Coord expect;

  delta  = Delta{ .w = 0, .h = 1 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };

  delta  = Delta{ .w = 1, .h = 0 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };

  delta  = Delta{ .w = 1, .h = 1 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 1, .h = 1 };
  coord  = Coord{ .x = 10, .y = 8 };
  expect = Coord{ .x = 10, .y = 8 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 1, .h = 1 };
  coord  = Coord{ .x = -10, .y = -8 };
  expect = Coord{ .x = -10, .y = -8 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 0, .y = 0 };
  expect = Coord{ .x = 0, .y = 0 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 5, .y = 3 };
  expect = Coord{ .x = 10, .y = 10 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 5, .y = 15 };
  expect = Coord{ .x = 10, .y = 20 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 20, .y = 20 };
  expect = Coord{ .x = 20, .y = 20 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = 23, .y = 29 };
  expect = Coord{ .x = 30, .y = 30 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = -5, .y = 29 };
  expect = Coord{ .x = 0, .y = 30 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = -7, .y = 29 };
  expect = Coord{ .x = 0, .y = 30 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = -2, .y = 29 };
  expect = Coord{ .x = 0, .y = 30 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ .w = 10, .h = 10 };
  coord  = Coord{ .x = -5, .y = -29 };
  expect = Coord{ .x = 0, .y = -20 };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );
}

TEST_CASE( "[coord] to gfx" ) {
  Delta d{ .w = 5, .h = 6 };
  Coord c{ .x = 5, .y = 6 };
  Rect  r = Rect::from( Coord{ .x = 1, .y = 2 },
                        Delta{ .w = 5, .h = 6 } );

  REQUIRE( d == gfx::size{ .w = 5, .h = 6 } );
  REQUIRE( c == gfx::point{ .x = 5, .y = 6 } );
  REQUIRE( r == gfx::rect{ .origin = { .x = 1, .y = 2 },
                           .size   = { .w = 5, .h = 6 } } );
}

TEST_CASE( "[coord] from gfx" ) {
  Delta d{ .w = 5, .h = 6 };
  Coord c{ .x = 5, .y = 6 };
  Rect  r = Rect::from( Coord{ .x = 1, .y = 2 },
                        Delta{ .w = 5, .h = 6 } );

  gfx::size  d_gfx{ .w = 5, .h = 6 };
  gfx::point c_gfx{ .x = 5, .y = 6 };
  gfx::rect  r_gfx = gfx::rect{ .origin = { .x = 1, .y = 2 },
                                .size   = { .w = 5, .h = 6 } };

  REQUIRE( Delta::from_gfx( d_gfx ) == d );
  REQUIRE( Coord::from_gfx( c_gfx ) == c );
  REQUIRE( Rect::from_gfx( r_gfx ) == r );
}

TEST_CASE( "[coord] Rect::with_new_origin" ) {
  Rect  rect = Rect::from( Coord{ .x = 4, .y = 5 },
                           Delta{ .w = 2, .h = 3 } );
  Coord new_origin{ .x = 1, .y = 3 };
  Rect  expected = Rect::from( Coord{ .x = 3, .y = 2 },
                               Delta{ .w = 2, .h = 3 } );
  REQUIRE( rect.with_new_origin( new_origin ) == expected );
}

TEST_CASE( "[coord] Coord::concentric_square_distance" ) {
  Coord const src{ .x = 3, .y = 4 };
  Coord       other;

  auto f = [&] {
    return src.concentric_square_distance( other );
  };

  other = src;
  REQUIRE( f() == 0 );
  --other.x;
  REQUIRE( f() == 1 );
  ++other.y;
  REQUIRE( f() == 1 );
  ++other.y;
  REQUIRE( f() == 2 );
  ++other.y;
  REQUIRE( f() == 3 );
  --other.x;
  REQUIRE( f() == 3 );
  --other.x;
  REQUIRE( f() == 3 );
  --other.x;
  REQUIRE( f() == 4 );
}

TEST_CASE( "[coord] std::hash<Coord>" ) {
  Coord                 input = {};
  unordered_set<size_t> answers;

  auto f = [&] { return std::hash<Coord>{}( input ); };

  input = Coord{ .x = -2, .y = 0 };
  answers.insert( f() );

  input = Coord{ .x = 0, .y = -2 };
  answers.insert( f() );

  input = Coord{ .x = 0, .y = 0 };
  answers.insert( f() );

  input = Coord{ .x = 2, .y = 0 };
  answers.insert( f() );

  input = Coord{ .x = 0, .y = 2 };
  answers.insert( f() );

  input = Coord{ .x = 2, .y = 2 };
  answers.insert( f() );

  input = Coord{ .x = -2, .y = -2 };
  answers.insert( f() );

  input = Coord{ .x = -2, .y = 2 };
  answers.insert( f() );

  input = Coord{ .x = 2, .y = -2 };
  answers.insert( f() );

  // This will check that the hash of each of the above coordi-
  // nates is unique. I guess this is not guaranteed, but any
  // sensible implementation should satisfy this. If this breaks
  // then the hashes should be examined as something might be
  // wrong.
  REQUIRE( answers.size() == 9 );
}

} // namespace
