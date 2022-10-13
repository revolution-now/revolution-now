/****************************************************************
**cartesian.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-26.
*
* Description: Unit tests for the src/gfx/cartesian.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/cartesian.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

using namespace Catch::literals;

using ::base::nothing;

/****************************************************************
** size
*****************************************************************/
TEST_CASE( "[gfx/cartesian] size::max_with" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 2, .h = 8 };
  REQUIRE( s1.max_with( s2 ) == size{ .w = 4, .h = 8 } );
}

TEST_CASE( "[gfx/cartesian] size::operator*( int )" ) {
  size s{ .w = 4, .h = 2 };
  REQUIRE( s * 10 == size{ .w = 40, .h = 20 } );
}

TEST_CASE( "[gfx/cartesian] size::operator/( int )" ) {
  size s{ .w = 4, .h = 2 };
  REQUIRE( s / 2 == size{ .w = 2, .h = 1 } );
}

TEST_CASE( "[gfx/cartesian] operator+( size )" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 4, .h = 3 };
  REQUIRE( s1 + s2 == size{ .w = 8, .h = 5 } );
}

TEST_CASE( "[gfx/cartesian] size::operator+=( size )" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 4, .h = 3 };
  s1 += s2;
  REQUIRE( s1 == size{ .w = 8, .h = 5 } );
  REQUIRE( s2 == size{ .w = 4, .h = 3 } );
}

/****************************************************************
** dsize
*****************************************************************/
TEST_CASE( "[gfx/cartesian] dsize::operator+=( size )" ) {
  dsize s1{ .w = 4.3, .h = 2.1 };
  dsize s2{ .w = 4, .h = 3 };
  s1 += s2;
  REQUIRE( s1 == dsize{ .w = 8.3, .h = 5.1 } );
  REQUIRE( s2 == dsize{ .w = 4, .h = 3 } );
}

TEST_CASE( "[gfx/cartesian] dsize::truncated" ) {
  dsize s{ .w = 4.3, .h = 2.1 };
  REQUIRE( s.truncated() == size{ .w = 4, .h = 2 } );
}

TEST_CASE( "[gfx/cartesian] to_double( size )" ) {
  size s{ .w = 4, .h = 2 };
  REQUIRE( to_double( s ) == dsize{ .w = 4, .h = 2 } );
}

TEST_CASE( "[gfx/cartesian] dsize::operator*( double )" ) {
  dsize s{ .w = 4, .h = 2 };
  REQUIRE( s * 10 == dsize{ .w = 40, .h = 20 } );
}

TEST_CASE( "[gfx/cartesian] dsize::operator/( double )" ) {
  dsize s{ .w = 4.2, .h = 2.2 };
  REQUIRE( s / 2 == dsize{ .w = 2.1, .h = 1.1 } );
}

/****************************************************************
** point
*****************************************************************/
TEST_CASE( "[gfx/cartesian] point::distance_from_origin" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p.distance_from_origin() == size{ .w = 4, .h = 2 } );
}

TEST_CASE( "[gfx/cartesian] point::moved_left" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p.moved_left() == point{ .x = 3, .y = 2 } );
  REQUIRE( p.moved_left( 2 ) == point{ .x = 2, .y = 2 } );
}

TEST_CASE( "[gfx/cartesian] point::clamped" ) {
  rect const r{ .origin = { .x = 3, .y = 4 },
                .size   = { .w = 2, .h = 3 } };
  point      p, ex;

  p  = { .x = 3, .y = 4 };
  ex = { .x = 3, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 2, .y = 4 };
  ex = { .x = 3, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 2, .y = 3 };
  ex = { .x = 3, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 4, .y = 1 };
  ex = { .x = 4, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 4, .y = 10 };
  ex = { .x = 4, .y = 7 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 10, .y = 6 };
  ex = { .x = 5, .y = 6 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 10, .y = 10 };
  ex = { .x = 5, .y = 7 };
  REQUIRE( p.clamped( r ) == ex );
}

TEST_CASE( "[gfx/cartesian] point::is_inside" ) {
  rect const r{ .origin = { .x = 3, .y = 4 },
                .size   = { .w = 2, .h = 3 } };
  point      p;
  bool       ex = {};

  p  = { .x = 3, .y = 4 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 5 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 2, .y = 4 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 2, .y = 3 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 1 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 10 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 6 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 7 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 10, .y = 10 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );
}

TEST_CASE( "[gfx/cartesian] operator+=( size )" ) {
  point      p{ .x = 4, .y = 2 };
  size const s{ .w = 5, .h = 1 };
  p += s;
  REQUIRE( p == point{ .x = 9, .y = 3 } );
}

TEST_CASE( "[gfx/cartesian] operator-( point )" ) {
  point p1{ .x = 4, .y = 2 };
  point p2{ .x = 5, .y = 1 };
  REQUIRE( p1 - p2 == size{ .w = -1, .h = 1 } );
}

TEST_CASE( "[gfx/cartesian] operator*( int )" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p * 10 == point{ .x = 40, .y = 20 } );
}

TEST_CASE( "[gfx/cartesian] operator/( int )" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p / 2 == point{ .x = 2, .y = 1 } );
}

TEST_CASE( "[gfx/cartesian] point::point_becomes_origin" ) {
  point const p{ .x = 4, .y = 2 };
  point const arg{ .x = 2, .y = 1 };
  point       expected{ .x = 2, .y = 1 };
  REQUIRE( p.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] point::origin_becomes_point" ) {
  point const p{ .x = 4, .y = 2 };
  point const arg{ .x = 2, .y = 1 };
  point       expected{ .x = 6, .y = 3 };
  REQUIRE( p.origin_becomes_point( arg ) == expected );
}

/****************************************************************
** dpoint
*****************************************************************/
TEST_CASE( "[gfx/cartesian] dpoint::truncated" ) {
  dpoint p{ .x = 4.4, .y = 2.4 };
  REQUIRE( p.truncated() == point{ .x = 4, .y = 2 } );
}

TEST_CASE( "[gfx/cartesian] dpoint::fmod" ) {
  dpoint p{ .x = 4.4, .y = 2.4 };
  // _a is a literal from Catch2 that means "approximately".
  REQUIRE( p.fmod( 2.1 ).w == .2_a );
  REQUIRE( p.fmod( 2.1 ).h == .3_a );
}

TEST_CASE( "[gfx/cartesian] dpoint::clamped" ) {
  drect const r{ .origin = { .x = 3.1, .y = 4 },
                 .size   = { .w = 2, .h = 3.1 } };
  dpoint      p, ex;

  p  = { .x = 3.1, .y = 4 };
  ex = { .x = 3.1, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 2, .y = 4 };
  ex = { .x = 3.1, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 2, .y = 3 };
  ex = { .x = 3.1, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 4, .y = 1 };
  ex = { .x = 4, .y = 4 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 4, .y = 10 };
  ex = { .x = 4, .y = 7.1 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 10, .y = 6 };
  ex = { .x = 5.1, .y = 6 };
  REQUIRE( p.clamped( r ) == ex );

  p  = { .x = 10, .y = 10 };
  ex = { .x = 5.1, .y = 7.1 };
  REQUIRE( p.clamped( r ) == ex );
}

TEST_CASE( "[gfx/cartesian] dpoint::is_inside" ) {
  drect const r{ .origin = { .x = 3.1, .y = 4 },
                 .size   = { .w = 2, .h = 3.1 } };
  dpoint      p;
  bool        ex = {};

  p  = { .x = 3, .y = 4 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 3.1, .y = 4 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 5 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 2, .y = 4 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 2, .y = 3 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 1 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 10 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 6 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 7 };
  ex = true;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 4, .y = 7.1 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );

  p  = { .x = 10, .y = 10 };
  ex = false;
  REQUIRE( p.is_inside( r ) == ex );
}

TEST_CASE( "[gfx/cartesian] dpoint::operator+=( dsize )" ) {
  dpoint p{ .x = 4.4, .y = 2.4 };
  dsize  s{ .w = 5.2, .h = 1.5 };
  p += s;
  REQUIRE( p.x == 9.6_a );
  REQUIRE( p.y == 3.9_a );
}

TEST_CASE( "[gfx/cartesian] dpoint::operator-=( dsize )" ) {
  dpoint p{ .x = 4.4, .y = 2.4 };
  dsize  s{ .w = 5.2, .h = 1.5 };
  p -= s;
  REQUIRE( p.x == -.8_a );
  REQUIRE( p.y == .9_a );
  REQUIRE( ( p - s ).x == -6.0_a );
  REQUIRE( ( p - s ).y == -.6_a );
}

TEST_CASE( "[gfx/cartesian] operator-( dsize )" ) {
  dpoint p{ .x = 4, .y = 2 };
  dsize  s{ .w = 5.2, .h = 1.5 };
  REQUIRE( ( p - s ).x == -1.2_a );
  REQUIRE( ( p - s ).y == .5_a );
}

TEST_CASE( "[gfx/cartesian] operator-()" ) {
  dpoint p{ .x = 4, .y = 2 };
  REQUIRE( -p == dpoint{ .x = -4, .y = -2 } );
}

TEST_CASE( "[gfx/cartesian] operator*( dpoint, double )" ) {
  dpoint p{ .x = 4, .y = 2 };
  REQUIRE( p * 1.5 == dpoint{ .x = 6, .y = 3 } );
}

TEST_CASE( "[gfx/cartesian] dpoint::operator*( double )" ) {
  dpoint p{ .x = 4.2, .y = 2.1 };
  REQUIRE( p * 10 == dpoint{ .x = 42, .y = 21 } );
}

TEST_CASE( "[gfx/cartesian] dpoint::operator/( double )" ) {
  dpoint p{ .x = 4.2, .y = 2.2 };
  REQUIRE( p / 2 == dpoint{ .x = 2.1, .y = 1.1 } );
}

TEST_CASE( "[gfx/cartesian] dpoint::point_becomes_origin" ) {
  dpoint const p{ .x = 4.2, .y = 2 };
  dpoint const arg{ .x = 2, .y = 1 };
  dpoint       expected{ .x = 2.2, .y = 1 };
  REQUIRE( p.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] dpoint::origin_becomes_point" ) {
  dpoint const p{ .x = 4.2, .y = 2 };
  dpoint const arg{ .x = 2, .y = 1 };
  dpoint       expected{ .x = 6.2, .y = 3 };
  REQUIRE( p.origin_becomes_point( arg ) == expected );
}

/****************************************************************
** rect
*****************************************************************/
TEST_CASE( "[gfx/cartesian] rect::from" ) {
  point p1{ .x = 4, .y = 2 };
  point p2{ .x = 2, .y = 4 };
  REQUIRE( rect::from( p1, p2 ) ==
           rect{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = -2, .h = 2 } } );
  REQUIRE( rect::from( p2, p1 ) ==
           rect{ .origin = { .x = 2, .y = 4 },
                 .size   = { .w = 2, .h = -2 } } );
  REQUIRE( rect::from( p1, p2 ).normalized() ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 2, .h = 2 } } );
}

TEST_CASE( "[gfx/cartesian] rect::nw, rect::se, etc." ) {
  rect r;

  REQUIRE( r.nw() == point{} );
  REQUIRE( r.ne() == point{} );
  REQUIRE( r.se() == point{} );
  REQUIRE( r.sw() == point{} );
  REQUIRE( r.top() == 0 );
  REQUIRE( r.bottom() == 0 );
  REQUIRE( r.right() == 0 );
  REQUIRE( r.left() == 0 );

  r = rect{ .origin = { .x = 3, .y = 4 },
            .size   = { .w = 1, .h = 3 } };
  REQUIRE( r.nw() == point{ .x = 3, .y = 4 } );
  REQUIRE( r.ne() == point{ .x = 4, .y = 4 } );
  REQUIRE( r.se() == point{ .x = 4, .y = 7 } );
  REQUIRE( r.sw() == point{ .x = 3, .y = 7 } );
  REQUIRE( r.top() == 4 );
  REQUIRE( r.bottom() == 7 );
  REQUIRE( r.right() == 4 );
  REQUIRE( r.left() == 3 );
}

TEST_CASE(
    "[gfx/cartesian] negative rect::nw, rect::se, etc." ) {
  rect r;

  r = rect{ .origin = { .x = 3, .y = 4 },
            .size   = { .w = -1, .h = -3 } };
  REQUIRE( r.nw() == point{ .x = 2, .y = 1 } );
  REQUIRE( r.ne() == point{ .x = 3, .y = 1 } );
  REQUIRE( r.se() == point{ .x = 3, .y = 4 } );
  REQUIRE( r.sw() == point{ .x = 2, .y = 4 } );
  REQUIRE( r.top() == 1 );
  REQUIRE( r.bottom() == 4 );
  REQUIRE( r.right() == 3 );
  REQUIRE( r.left() == 2 );
}

TEST_CASE( "[gfx/cartesian] rect::is_inside" ) {
  rect r1;
  rect r2{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = 0, .h = 0 } };
  rect r3{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = 1, .h = 0 } };
  rect r4{ .origin = { .x = 2, .y = 2 },
           .size   = { .w = 0, .h = 1 } };
  rect r5{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = 3, .h = 3 } };
  rect r6{ .origin = { .x = 0, .y = 2 },
           .size   = { .w = 4, .h = 3 } };
  rect r7{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = 2, .h = 1 } };
  rect r8{ .origin = { .x = 2, .y = 1 },
           .size   = { .w = 2, .h = 1 } };

  REQUIRE( r1.is_inside( r1 ) == true );
  REQUIRE( r1.is_inside( r2 ) == false );
  REQUIRE( r1.is_inside( r3 ) == false );
  REQUIRE( r1.is_inside( r4 ) == false );
  REQUIRE( r1.is_inside( r5 ) == false );
  REQUIRE( r1.is_inside( r6 ) == false );
  REQUIRE( r1.is_inside( r7 ) == false );
  REQUIRE( r1.is_inside( r8 ) == false );

  REQUIRE( r2.is_inside( r1 ) == false );
  REQUIRE( r2.is_inside( r2 ) == true );
  REQUIRE( r2.is_inside( r3 ) == false );
  REQUIRE( r2.is_inside( r4 ) == false );
  REQUIRE( r2.is_inside( r5 ) == false );
  REQUIRE( r2.is_inside( r6 ) == false );
  REQUIRE( r2.is_inside( r7 ) == true );
  REQUIRE( r2.is_inside( r8 ) == false );

  REQUIRE( r3.is_inside( r1 ) == false );
  REQUIRE( r3.is_inside( r2 ) == false );
  REQUIRE( r3.is_inside( r3 ) == true );
  REQUIRE( r3.is_inside( r4 ) == false );
  REQUIRE( r3.is_inside( r5 ) == true );
  REQUIRE( r3.is_inside( r6 ) == true );
  REQUIRE( r3.is_inside( r7 ) == true );
  REQUIRE( r3.is_inside( r8 ) == false );

  REQUIRE( r4.is_inside( r1 ) == false );
  REQUIRE( r4.is_inside( r2 ) == false );
  REQUIRE( r4.is_inside( r3 ) == false );
  REQUIRE( r4.is_inside( r4 ) == true );
  REQUIRE( r4.is_inside( r5 ) == true );
  REQUIRE( r4.is_inside( r6 ) == true );
  REQUIRE( r4.is_inside( r7 ) == false );
  REQUIRE( r4.is_inside( r8 ) == false );

  REQUIRE( r5.is_inside( r1 ) == false );
  REQUIRE( r5.is_inside( r2 ) == false );
  REQUIRE( r5.is_inside( r3 ) == false );
  REQUIRE( r5.is_inside( r4 ) == false );
  REQUIRE( r5.is_inside( r5 ) == true );
  REQUIRE( r5.is_inside( r6 ) == true );
  REQUIRE( r5.is_inside( r7 ) == false );
  REQUIRE( r5.is_inside( r8 ) == false );

  REQUIRE( r6.is_inside( r1 ) == false );
  REQUIRE( r6.is_inside( r2 ) == false );
  REQUIRE( r6.is_inside( r3 ) == false );
  REQUIRE( r6.is_inside( r4 ) == false );
  REQUIRE( r6.is_inside( r5 ) == false );
  REQUIRE( r6.is_inside( r6 ) == true );
  REQUIRE( r6.is_inside( r7 ) == false );
  REQUIRE( r6.is_inside( r8 ) == false );

  REQUIRE( r7.is_inside( r1 ) == false );
  REQUIRE( r7.is_inside( r2 ) == false );
  REQUIRE( r7.is_inside( r3 ) == false );
  REQUIRE( r7.is_inside( r4 ) == false );
  REQUIRE( r7.is_inside( r5 ) == false );
  REQUIRE( r7.is_inside( r6 ) == false );
  REQUIRE( r7.is_inside( r7 ) == true );
  REQUIRE( r7.is_inside( r8 ) == false );

  REQUIRE( r8.is_inside( r1 ) == false );
  REQUIRE( r8.is_inside( r2 ) == false );
  REQUIRE( r8.is_inside( r3 ) == false );
  REQUIRE( r8.is_inside( r4 ) == false );
  REQUIRE( r8.is_inside( r5 ) == false );
  REQUIRE( r8.is_inside( r6 ) == false );
  REQUIRE( r8.is_inside( r7 ) == false );
  REQUIRE( r8.is_inside( r8 ) == true );
}

TEST_CASE( "[gfx/cartesian] negaitve rect::is_inside" ) {
  rect r1;
  rect r2{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = -0, .h = -0 } };
  rect r3{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = -1, .h = -0 } };
  rect r4{ .origin = { .x = 2, .y = 2 },
           .size   = { .w = -0, .h = -1 } };
  rect r5{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = -3, .h = -3 } };
  rect r6{ .origin = { .x = 0, .y = 2 },
           .size   = { .w = -4, .h = -3 } };
  rect r7{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = -2, .h = -1 } };
  rect r8{ .origin = { .x = 2, .y = 1 },
           .size   = { .w = -2, .h = -1 } };

  REQUIRE( r1.is_inside( r1 ) == true );
  REQUIRE( r1.is_inside( r2 ) == false );
  REQUIRE( r1.is_inside( r3 ) == false );
  REQUIRE( r1.is_inside( r4 ) == false );
  REQUIRE( r1.is_inside( r5 ) == true );
  REQUIRE( r1.is_inside( r6 ) == true );
  REQUIRE( r1.is_inside( r7 ) == true );
  REQUIRE( r1.is_inside( r8 ) == true );

  REQUIRE( r2.is_inside( r1 ) == false );
  REQUIRE( r2.is_inside( r2 ) == true );
  REQUIRE( r2.is_inside( r3 ) == false );
  REQUIRE( r2.is_inside( r4 ) == false );
  REQUIRE( r2.is_inside( r5 ) == true );
  REQUIRE( r2.is_inside( r6 ) == false );
  REQUIRE( r2.is_inside( r7 ) == true );
  REQUIRE( r2.is_inside( r8 ) == true );

  REQUIRE( r3.is_inside( r1 ) == false );
  REQUIRE( r3.is_inside( r2 ) == false );
  REQUIRE( r3.is_inside( r3 ) == true );
  REQUIRE( r3.is_inside( r4 ) == false );
  REQUIRE( r3.is_inside( r5 ) == true );
  REQUIRE( r3.is_inside( r6 ) == false );
  REQUIRE( r3.is_inside( r7 ) == false );
  REQUIRE( r3.is_inside( r8 ) == false );

  REQUIRE( r4.is_inside( r1 ) == false );
  REQUIRE( r4.is_inside( r2 ) == false );
  REQUIRE( r4.is_inside( r3 ) == false );
  REQUIRE( r4.is_inside( r4 ) == true );
  REQUIRE( r4.is_inside( r5 ) == false );
  REQUIRE( r4.is_inside( r6 ) == false );
  REQUIRE( r4.is_inside( r7 ) == false );
  REQUIRE( r4.is_inside( r8 ) == false );

  REQUIRE( r5.is_inside( r1 ) == false );
  REQUIRE( r5.is_inside( r2 ) == false );
  REQUIRE( r5.is_inside( r3 ) == false );
  REQUIRE( r5.is_inside( r4 ) == false );
  REQUIRE( r5.is_inside( r5 ) == true );
  REQUIRE( r5.is_inside( r6 ) == false );
  REQUIRE( r5.is_inside( r7 ) == false );
  REQUIRE( r5.is_inside( r8 ) == false );

  REQUIRE( r6.is_inside( r1 ) == false );
  REQUIRE( r6.is_inside( r2 ) == false );
  REQUIRE( r6.is_inside( r3 ) == false );
  REQUIRE( r6.is_inside( r4 ) == false );
  REQUIRE( r6.is_inside( r5 ) == false );
  REQUIRE( r6.is_inside( r6 ) == true );
  REQUIRE( r6.is_inside( r7 ) == false );
  REQUIRE( r6.is_inside( r8 ) == false );

  REQUIRE( r7.is_inside( r1 ) == false );
  REQUIRE( r7.is_inside( r2 ) == false );
  REQUIRE( r7.is_inside( r3 ) == false );
  REQUIRE( r7.is_inside( r4 ) == false );
  REQUIRE( r7.is_inside( r5 ) == true );
  REQUIRE( r7.is_inside( r6 ) == false );
  REQUIRE( r7.is_inside( r7 ) == true );
  REQUIRE( r7.is_inside( r8 ) == false );

  REQUIRE( r8.is_inside( r1 ) == false );
  REQUIRE( r8.is_inside( r2 ) == false );
  REQUIRE( r8.is_inside( r3 ) == false );
  REQUIRE( r8.is_inside( r4 ) == false );
  REQUIRE( r8.is_inside( r5 ) == false );
  REQUIRE( r8.is_inside( r6 ) == false );
  REQUIRE( r8.is_inside( r7 ) == false );
  REQUIRE( r8.is_inside( r8 ) == true );
}

TEST_CASE( "[gfx/cartesian] rect::contains" ) {
  rect r1;
  rect r2{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = 2, .h = 1 } };

  REQUIRE( r1.contains( point{} ) == true );
  REQUIRE( r1.contains( point{ .x = 1, .y = 0 } ) == false );
  REQUIRE( r1.contains( point{ .x = 0, .y = 1 } ) == false );
  REQUIRE( r1.contains( point{ .x = 1, .y = 1 } ) == false );

  REQUIRE( r2.contains( point{ .x = 0, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 2, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 3, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 0, .y = 1 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 1 } ) == true );
  REQUIRE( r2.contains( point{ .x = 2, .y = 1 } ) == true );
  REQUIRE( r2.contains( point{ .x = 3, .y = 1 } ) == true );
  REQUIRE( r2.contains( point{ .x = 0, .y = 2 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 2 } ) == true );
  REQUIRE( r2.contains( point{ .x = 2, .y = 2 } ) == true );
  REQUIRE( r2.contains( point{ .x = 3, .y = 2 } ) == true );
  REQUIRE( r2.contains( point{ .x = 0, .y = 3 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 3 } ) == false );
  REQUIRE( r2.contains( point{ .x = 2, .y = 3 } ) == false );
  REQUIRE( r2.contains( point{ .x = 3, .y = 3 } ) == false );
}

TEST_CASE( "[gfx/cartesian] rect::clipped_by" ) {
  rect expected, r1, r2;

  r1       = rect{ .origin = { .x = -1, .y = 0 },
                   .size   = { .w = 3, .h = 3 } };
  r2       = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 5, .h = 7 } };
  expected = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 3 } };
  r2       = rect{ .origin = { .x = 0, .y = 1 },
                   .size   = { .w = 5, .h = 7 } };
  expected = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 3 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 0, .y = 0 },
                   .size   = { .w = 2, .h = 2 } };
  r2       = rect{ .origin = { .x = 1, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  expected = rect{ .origin = { .x = 1, .y = 2 },
                   .size   = { .w = 1, .h = 0 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  r2       = rect{ .origin = { .x = 2, .y = 0 },
                   .size   = { .w = 2, .h = 2 } };
  expected = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 0 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  r2       = rect{ .origin = { .x = 4, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  expected = rect{ .origin = { .x = 4, .y = 2 },
                   .size   = { .w = 0, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1 = rect{ .origin = { .x = 2, .y = 2 },
             .size   = { .w = 2, .h = 2 } };
  r2 = rect{ .origin = { .x = 6, .y = 2 },
             .size   = { .w = 2, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == nothing );

  r1       = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  r2       = rect{ .origin = { .x = 3, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  expected = rect{ .origin = { .x = 3, .y = 2 },
                   .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 0, .y = 0 },
                   .size   = { .w = 5, .h = 10 } };
  r2       = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 1, .h = 1 } };
  expected = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 1, .h = 1 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::clamped" ) {
  rect r, bounds, expected;

  r = { .origin = { .x = 4, .y = 2 },
        .size   = { .w = 7, .h = 4 } };

  bounds   = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 13, .h = 5 } };
  expected = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 5, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 2, .h = 3 } };
  expected = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 2, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 5, .y = 1 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 5, .y = 2 },
               .size   = { .w = 6, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 3, .y = 3 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 3 },
               .size   = { .w = 6, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 3, .y = 1 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 6, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 5, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 5, .y = 2 },
               .size   = { .w = 6, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 10, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 10, .y = 2 },
               .size   = { .w = 1, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 11, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 11, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 12, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 12, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = -2, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 1, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = -3, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = -4, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 3, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = -1 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 1 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = -2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 0 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = -3 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 1 },
               .size   = { .w = 7, .h = 0 } };
  REQUIRE( r.clamped( bounds ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::with_origin" ) {
  rect r{ .origin = { .x = 4, .y = 2 },
          .size   = { .w = 2, .h = 4 } };
  rect expected{ .origin = { .x = 1, .y = 10 },
                 .size   = { .w = 2, .h = 4 } };
  REQUIRE( r.with_origin( point{ .x = 1, .y = 10 } ) ==
           expected );
}

TEST_CASE( "[gfx/cartesian] rect::center" ) {
  rect  r{ .origin = { .x = 4, .y = 2 },
           .size   = { .w = 3, .h = 4 } };
  point expected{ .x = 5, .y = 4 };
  REQUIRE( r.center() == expected );

  r        = rect{ .origin = { .x = 0, .y = 0 },
                   .size   = { .w = 10, .h = 11 } };
  expected = point{ .x = 5, .y = 5 };
  REQUIRE( r.center() == expected );
}

TEST_CASE( "[gfx/cartesian] rect::operator{*,/}( double )" ) {
  rect r{ .origin = { .x = 3, .y = 4 },
          .size   = { .w = 6, .h = 7 } };
  REQUIRE( r * 2 == rect{ .origin = { .x = 6, .y = 8 },
                          .size   = { .w = 12, .h = 14 } } );
  REQUIRE( r / 2 == rect{ .origin = { .x = 1, .y = 2 },
                          .size   = { .w = 3, .h = 3 } } );
}

TEST_CASE( "[gfx/cartesian] rect::point_becomes_origin" ) {
  rect const  r{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = 7, .h = 8 } };
  point const arg{ .x = 2, .y = 1 };
  rect        expected{ .origin = { .x = 2, .y = 1 },
                        .size   = { .w = 7, .h = 8 } };
  REQUIRE( r.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::origin_becomes_point" ) {
  rect const  r{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = 7, .h = 8 } };
  point const arg{ .x = 2, .y = 1 };
  rect        expected{ .origin = { .x = 6, .y = 3 },
                        .size   = { .w = 7, .h = 8 } };
  REQUIRE( r.origin_becomes_point( arg ) == expected );
}

/****************************************************************
** drect
*****************************************************************/
TEST_CASE( "[gfx/cartesian] drect::truncated" ) {
  dsize const  s{ .w = 4.3, .h = 2.1 };
  dpoint const p{ .x = 4.4, .y = 2.4 };
  drect const  r{ .origin = p, .size = s };
  REQUIRE( r.truncated() == rect{ .origin = { .x = 4, .y = 2 },
                                  .size = { .w = 4, .h = 2 } } );
}

TEST_CASE( "[gfx/cartesian] to_double( rect )" ) {
  rect r{ .origin = { .x = 3, .y = 4 },
          .size   = { .w = 4, .h = 2 } };
  REQUIRE( to_double( r ) ==
           drect{ .origin = { .x = 3.0, .y = 4.0 },
                  .size   = { .w = 4.0, .h = 2.0 } } );
}

TEST_CASE( "[gfx/cartesian] drect::normalized()" ) {
  drect r{ .origin = { .x = 3, .y = 4 },
           .size   = { .w = -4, .h = -2 } };
  REQUIRE( r.normalized() ==
           drect{ .origin = { .x = -1, .y = 2 },
                  .size   = { .w = 4, .h = 2 } } );
}

TEST_CASE( "[gfx/cartesian] drect::clipped_by" ) {
  drect expected, r1, r2;

  r1       = drect{ .origin = { .x = -1, .y = 0 },
                    .size   = { .w = 3, .h = 3 } };
  r2       = drect{ .origin = { .x = 1, .y = 1 },
                    .size   = { .w = 5, .h = 7 } };
  expected = drect{ .origin = { .x = 1, .y = 1 },
                    .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = drect{ .origin = { .x = 2, .y = 2 },
                    .size   = { .w = 2, .h = 3 } };
  r2       = drect{ .origin = { .x = 0, .y = 1 },
                    .size   = { .w = 5, .h = 7 } };
  expected = drect{ .origin = { .x = 2, .y = 2 },
                    .size   = { .w = 2, .h = 3 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = drect{ .origin = { .x = 0, .y = 0 },
                    .size   = { .w = 2, .h = 2 } };
  r2       = drect{ .origin = { .x = 1, .y = 2 },
                    .size   = { .w = 2, .h = 2 } };
  expected = drect{ .origin = { .x = 1, .y = 2 },
                    .size   = { .w = 1, .h = 0 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = drect{ .origin = { .x = 2, .y = 2 },
                    .size   = { .w = 2, .h = 2 } };
  r2       = drect{ .origin = { .x = 2, .y = 0 },
                    .size   = { .w = 2, .h = 2 } };
  expected = drect{ .origin = { .x = 2, .y = 2 },
                    .size   = { .w = 2, .h = 0 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = drect{ .origin = { .x = 2, .y = 2 },
                    .size   = { .w = 2, .h = 2 } };
  r2       = drect{ .origin = { .x = 4, .y = 2 },
                    .size   = { .w = 2, .h = 2 } };
  expected = drect{ .origin = { .x = 4, .y = 2 },
                    .size   = { .w = 0, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1 = drect{ .origin = { .x = 2, .y = 2 },
              .size   = { .w = 2, .h = 2 } };
  r2 = drect{ .origin = { .x = 6, .y = 2 },
              .size   = { .w = 2, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == nothing );

  r1       = drect{ .origin = { .x = 2, .y = 2 },
                    .size   = { .w = 2, .h = 2 } };
  r2       = drect{ .origin = { .x = 3, .y = 2 },
                    .size   = { .w = 2, .h = 2 } };
  expected = drect{ .origin = { .x = 3, .y = 2 },
                    .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = drect{ .origin = { .x = 0, .y = 0 },
                    .size   = { .w = 5, .h = 10 } };
  r2       = drect{ .origin = { .x = 1, .y = 1 },
                    .size   = { .w = 1, .h = 1 } };
  expected = drect{ .origin = { .x = 1, .y = 1 },
                    .size   = { .w = 1, .h = 1 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1 = drect{ .origin = { .x = -16.72272810753941,
                          .y = 459.980374966315 },
              .size   = { .w = 1825.4454562150786,
                          .h = 1186.539546539801 } };
  r2 = drect{ .origin = { .x = 0, .y = 0 },
              .size   = { .w = 1792, .h = 2240 } };
  expected =
      drect{ .origin = { .x = 0, .y = 459.980374966315 },
             .size   = { .w = 1792, .h = 1186.539546539801 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );
}

TEST_CASE( "[gfx/cartesian] drect::clamped" ) {
  drect r, bounds, expected;

  r = { .origin = { .x = 4, .y = 2 },
        .size   = { .w = 7, .h = 4 } };

  bounds   = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 13, .h = 5 } };
  expected = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 5, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 2, .h = 3 } };
  expected = { .origin = { .x = 6, .y = 3 },
               .size   = { .w = 2, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 5, .y = 1 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 5, .y = 2 },
               .size   = { .w = 6, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 3, .y = 3 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 3 },
               .size   = { .w = 6, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 3, .y = 1 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 6, .h = 3 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 5, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 5, .y = 2 },
               .size   = { .w = 6, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 10, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 10, .y = 2 },
               .size   = { .w = 1, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 11, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 11, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 12, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 12, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = -2, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 1, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = -3, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = -4, .y = 2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 3, .y = 2 },
               .size   = { .w = 0, .h = 4 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = -1 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 1 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = -2 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 7, .h = 0 } };
  REQUIRE( r.clamped( bounds ) == expected );

  bounds   = { .origin = { .x = 4, .y = -3 },
               .size   = { .w = 7, .h = 4 } };
  expected = { .origin = { .x = 4, .y = 1 },
               .size   = { .w = 7, .h = 0 } };
  REQUIRE( r.clamped( bounds ) == expected );
}

TEST_CASE( "[gfx/cartesian] drect::nw, rect::se, etc." ) {
  drect r;

  REQUIRE( r.nw() == dpoint{} );
  REQUIRE( r.ne() == dpoint{} );
  REQUIRE( r.se() == dpoint{} );
  REQUIRE( r.sw() == dpoint{} );
  REQUIRE( r.top() == 0 );
  REQUIRE( r.bottom() == 0 );
  REQUIRE( r.right() == 0 );
  REQUIRE( r.left() == 0 );

  r = drect{ .origin = { .x = 3, .y = 4 },
             .size   = { .w = 1, .h = 3 } };
  REQUIRE( r.nw() == dpoint{ .x = 3, .y = 4 } );
  REQUIRE( r.ne() == dpoint{ .x = 4, .y = 4 } );
  REQUIRE( r.se() == dpoint{ .x = 4, .y = 7 } );
  REQUIRE( r.sw() == dpoint{ .x = 3, .y = 7 } );
  REQUIRE( r.top() == 4 );
  REQUIRE( r.bottom() == 7 );
  REQUIRE( r.right() == 4 );
  REQUIRE( r.left() == 3 );
}

TEST_CASE( "[gfx/cartesian] drect::operator{*,/}( double )" ) {
  drect r{ .origin = { .x = 3, .y = 4 },
           .size   = { .w = 6, .h = 7 } };
  REQUIRE( r * 2 == drect{ .origin = { .x = 6, .y = 8 },
                           .size   = { .w = 12, .h = 14 } } );
  REQUIRE( r / 2 == drect{ .origin = { .x = 1.5, .y = 2 },
                           .size   = { .w = 3, .h = 3.5 } } );
}

TEST_CASE( "[gfx/cartesian] drect::point_becomes_origin" ) {
  drect const  r{ .origin = { .x = 4.2, .y = 2 },
                  .size   = { .w = 7.2, .h = 8 } };
  dpoint const arg{ .x = 2.1, .y = 1 };
  drect        expected{ .origin = { .x = 2.1, .y = 1 },
                         .size   = { .w = 7.2, .h = 8 } };
  REQUIRE( r.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] drect::origin_becomes_point" ) {
  drect const  r{ .origin = { .x = 4.2, .y = 2 },
                  .size   = { .w = 7.2, .h = 8 } };
  dpoint const arg{ .x = 2.1, .y = 1 };
  drect const  res = r.origin_becomes_point( arg );
  REQUIRE( res.origin.x == 6.3_a );
  REQUIRE( res.origin.y == 3_a );
  REQUIRE( res.size.w == 7.2_a );
  REQUIRE( res.size.h == 8_a );
}

/****************************************************************
** Combining Operators
*****************************************************************/
TEST_CASE( "[gfx/cartesian] point + size" ) {
  point p{ .x = 4, .y = 2 };
  size  s{ .w = 2, .h = 8 };
  REQUIRE( p + s == point{ .x = 6, .y = 10 } );
  REQUIRE( s + p == point{ .x = 6, .y = 10 } );
}

TEST_CASE( "[gfx/cartesian] point += size" ) {
  point p{ .x = 4, .y = 2 };
  size  s{ .w = 2, .h = 8 };
  p += s;
  REQUIRE( p == point{ .x = 6, .y = 10 } );
}

TEST_CASE( "[gfx/cartesian] point - point" ) {
  point p1{ .x = 4, .y = 2 };
  point p2{ .x = 2, .y = 4 };
  REQUIRE( p1 - p2 == size{ .w = 2, .h = -2 } );
}

TEST_CASE( "[gfx/cartesian] dpoint - dpoint" ) {
  dpoint p1{ .x = 4.1, .y = 2 };
  dpoint p2{ .x = 2, .y = 4 };
  REQUIRE( ( p1 - p2 ).w == 2.1_a );
  REQUIRE( ( p1 - p2 ).h == -2.0_a );
}

TEST_CASE( "[gfx/cartesian] point*size" ) {
  point p{ .x = 4, .y = 2 };
  size  s{ .w = 2, .h = 8 };
  REQUIRE( p * s == point{ .x = 8, .y = 16 } );
}

TEST_CASE( "[gfx/cartesian] dpoint + dsize" ) {
  dpoint p{ .x = 4, .y = 2 };
  dsize  s{ .w = 2, .h = 8 };
  REQUIRE( p + s == dpoint{ .x = 6, .y = 10 } );
  REQUIRE( s + p == dpoint{ .x = 6, .y = 10 } );
}

/****************************************************************
** Free Functions
*****************************************************************/
TEST_CASE( "[gfx/cartesian] centered*" ) {
  drect  rect;
  dsize  delta;
  dpoint expect;

  rect   = drect{ .origin = { .x = 1, .y = 1 },
                  .size   = { .w = 0, .h = 0 } };
  delta  = dsize{ .w = 4, .h = 3 };
  expect = dpoint{ .x = -1, .y = -.5 };
  REQUIRE( centered_in( delta, rect ) == expect );

  rect   = drect{ .origin = { .x = 1, .y = 2 },
                  .size   = { .w = 5, .h = 6 } };
  delta  = dsize{ .w = 3, .h = 4 };
  expect = dpoint{ .x = 2, .y = 3 };
  REQUIRE( centered_in( delta, rect ) == expect );
}

} // namespace
} // namespace gfx
