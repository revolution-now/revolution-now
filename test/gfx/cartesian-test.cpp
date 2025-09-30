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

// gfx
#include "src/gfx/coord.hpp" // FIXME: temporary

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

using namespace Catch::literals;

using ::base::nothing;
using ::Catch::Detail::Approx;

/****************************************************************
** e_side
*****************************************************************/
TEST_CASE( "[gfx/cartesian] reverse( e_side )" ) {
  REQUIRE( reverse( e_side::left ) == e_side::right );
  REQUIRE( reverse( e_side::right ) == e_side::left );
}

/****************************************************************
** e_direction
*****************************************************************/
TEST_CASE( "[gfx/cartesian] e_direction/direction type*" ) {
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

TEST_CASE( "[gfx/cartesian] e_direction/reverse_direction*" ) {
  REQUIRE( reverse_direction( e_direction::nw ) ==
           e_direction::se );
  REQUIRE( reverse_direction( e_direction::ne ) ==
           e_direction::sw );
  REQUIRE( reverse_direction( e_direction::sw ) ==
           e_direction::ne );
  REQUIRE( reverse_direction( e_direction::se ) ==
           e_direction::nw );
  REQUIRE( reverse_direction( e_direction::n ) ==
           e_direction::s );
  REQUIRE( reverse_direction( e_direction::w ) ==
           e_direction::e );
  REQUIRE( reverse_direction( e_direction::e ) ==
           e_direction::w );
  REQUIRE( reverse_direction( e_direction::s ) ==
           e_direction::n );

  REQUIRE( reverse_direction( e_diagonal_direction::nw ) ==
           e_diagonal_direction::se );
  REQUIRE( reverse_direction( e_diagonal_direction::ne ) ==
           e_diagonal_direction::sw );
  REQUIRE( reverse_direction( e_diagonal_direction::sw ) ==
           e_diagonal_direction::ne );
  REQUIRE( reverse_direction( e_diagonal_direction::se ) ==
           e_diagonal_direction::nw );
}

TEST_CASE( "[gfx/cartesian] to_diagonal" ) {
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

TEST_CASE(
    "[gfx/cartesian] to_direction( diagonal/cardinal )" ) {
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
  REQUIRE( to_cdirection( e_cardinal_direction::n ) ==
           e_cdirection::n );
  REQUIRE( to_cdirection( e_cardinal_direction::w ) ==
           e_cdirection::w );
  REQUIRE( to_cdirection( e_cardinal_direction::e ) ==
           e_cdirection::e );
  REQUIRE( to_cdirection( e_cardinal_direction::s ) ==
           e_cdirection::s );
}

/****************************************************************
** e_cdirection
*****************************************************************/
TEST_CASE( "[gfx/cartesian] to_direction( e_cdirection )" ) {
  REQUIRE( to_direction( e_cdirection::nw ) == e_direction::nw );
  REQUIRE( to_direction( e_cdirection::n ) == e_direction::n );
  REQUIRE( to_direction( e_cdirection::ne ) == e_direction::ne );
  REQUIRE( to_direction( e_cdirection::w ) == e_direction::w );
  REQUIRE( to_direction( e_cdirection::c ) == base::nothing );
  REQUIRE( to_direction( e_cdirection::e ) == e_direction::e );
  REQUIRE( to_direction( e_cdirection::sw ) == e_direction::sw );
  REQUIRE( to_direction( e_cdirection::s ) == e_direction::s );
  REQUIRE( to_direction( e_cdirection::se ) == e_direction::se );
}

TEST_CASE( "[gfx/cartesian] to_cdirection*" ) {
  REQUIRE( to_cdirection( e_direction::nw ) ==
           e_cdirection::nw );
  REQUIRE( to_cdirection( e_direction::n ) == e_cdirection::n );
  REQUIRE( to_cdirection( e_direction::ne ) ==
           e_cdirection::ne );
  REQUIRE( to_cdirection( e_direction::w ) == e_cdirection::w );
  REQUIRE( to_cdirection( e_direction::e ) == e_cdirection::e );
  REQUIRE( to_cdirection( e_direction::sw ) ==
           e_cdirection::sw );
  REQUIRE( to_cdirection( e_direction::s ) == e_cdirection::s );
  REQUIRE( to_cdirection( e_direction::se ) ==
           e_cdirection::se );
}

/****************************************************************
** e_diagonal_direction
*****************************************************************/
TEST_CASE( "[gfx/cartesian] side_for( e_diagonal_direction )" ) {
  REQUIRE( side_for( e_diagonal_direction::se ) ==
           e_side::right );
  REQUIRE( side_for( e_diagonal_direction::sw ) ==
           e_side::left );
  REQUIRE( side_for( e_diagonal_direction::ne ) ==
           e_side::right );
  REQUIRE( side_for( e_diagonal_direction::nw ) ==
           e_side::left );
}

/****************************************************************
** size
*****************************************************************/
TEST_CASE( "[gfx/cartesian] side::with_w/with_h" ) {
  size const s{ .w = 4, .h = 8 };
  REQUIRE( s.with_w( 7 ) == size{ .w = 7, .h = 8 } );
  REQUIRE( s.with_h( 7 ) == size{ .w = 4, .h = 7 } );
}

TEST_CASE( "[gfx/cartesian] empty/area" ) {
  size s1{ .w = 4, .h = 8 };
  size s2{ .w = 4, .h = 0 };
  size s3{ .w = 0, .h = 8 };
  size s4{ .w = 0, .h = 0 };

  REQUIRE( !s1.empty() );
  REQUIRE( !s2.empty() );
  REQUIRE( !s3.empty() );
  REQUIRE( s4.empty() );

  REQUIRE( s1.area() == 32 );
  REQUIRE( s2.area() == 0 );
  REQUIRE( s3.area() == 0 );
  REQUIRE( s4.area() == 0 );
}

TEST_CASE( "[gfx/cartesian] size::max_with" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 2, .h = 8 };
  REQUIRE( s1.max_with( s2 ) == size{ .w = 4, .h = 8 } );
}

TEST_CASE( "[gfx/cartesian] size::pythagorean" ) {
  size s = {};

  s = { .w = 4, .h = -2 };
  REQUIRE( s.pythagorean() == Approx( 4.472136 ) );
  s = { .w = 5, .h = 12 };
  REQUIRE( s.pythagorean() == 13.0 );
}

TEST_CASE( "[gfx/cartesian] size::chessboard_distance" ) {
  size s = {};

  s = { .w = 4, .h = -2 };
  REQUIRE( s.chessboard_distance() == 4 );
  s = { .w = 5, .h = 12 };
  REQUIRE( s.chessboard_distance() == 12 );
}

TEST_CASE( "[gfx/cartesian] size::abs" ) {
  size s = {};

  s = { .w = 4, .h = 2 };
  REQUIRE( s.abs() == size{ .w = 4, .h = 2 } );
  s = { .w = 4, .h = -2 };
  REQUIRE( s.abs() == size{ .w = 4, .h = 2 } );
  s = { .w = -4, .h = 2 };
  REQUIRE( s.abs() == size{ .w = 4, .h = 2 } );
  s = { .w = -4, .h = -2 };
  REQUIRE( s.abs() == size{ .w = 4, .h = 2 } );
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

TEST_CASE( "[gfx/cartesian] operator-( size )" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 4, .h = 3 };
  REQUIRE( s1 - s2 == size{ .w = 0, .h = -1 } );
}

TEST_CASE( "[gfx/cartesian] size::operator-()" ) {
  size s1{ .w = 4, .h = -2 };
  size s2{ .w = -3, .h = 0 };
  REQUIRE( -s1 == size{ .w = -4, .h = 2 } );
  REQUIRE( -s2 == size{ .w = 3, .h = 0 } );
}

TEST_CASE( "[gfx/cartesian] size::operator+=( size )" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 4, .h = 3 };
  s1 += s2;
  REQUIRE( s1 == size{ .w = 8, .h = 5 } );
  REQUIRE( s2 == size{ .w = 4, .h = 3 } );
}

TEST_CASE( "[gfx/cartesian] size::to_double" ) {
  size s{ .w = 4, .h = 2 };
  REQUIRE( s.to_double() == dsize{ .w = 4, .h = 2 } );
}

TEST_CASE( "[gfx/cartesian] size::fits_inside" ) {
  size const s0{ .w = 4, .h = 2 };
  size const s1{ .w = 2, .h = 2 };
  size const s2{ .w = 3, .h = 3 };
  size const s3{ .w = 0, .h = 0 };
  size const s4{ .w = -1, .h = 0 };
  size const s5{ .w = 4, .h = -2 };
  size const s6{ .w = -2, .h = -1 };
  size const s7{ .w = -1, .h = 0 };
  size const s8{ .w = -4, .h = 2 };
  size const s9{ .w = -2, .h = 1 };

  auto f = []( size const l, size const r ) {
    return l.fits_inside( r );
  };

  // clang-format off
  REQUIRE( f( s0, s0 ) == true  ); REQUIRE( f( s0, s1 ) == false );
  REQUIRE( f( s0, s2 ) == false ); REQUIRE( f( s0, s3 ) == false );
  REQUIRE( f( s0, s4 ) == false ); REQUIRE( f( s0, s5 ) == false );
  REQUIRE( f( s0, s6 ) == false ); REQUIRE( f( s0, s7 ) == false );
  REQUIRE( f( s0, s8 ) == false ); REQUIRE( f( s0, s9 ) == false );
  REQUIRE( f( s1, s0 ) == true  ); REQUIRE( f( s2, s0 ) == false );
  REQUIRE( f( s3, s0 ) == true  ); REQUIRE( f( s4, s0 ) == false );
  REQUIRE( f( s5, s0 ) == false ); REQUIRE( f( s6, s0 ) == false );
  REQUIRE( f( s7, s0 ) == false ); REQUIRE( f( s8, s0 ) == false );
  REQUIRE( f( s9, s0 ) == false ); REQUIRE( f( s1, s1 ) == true  );
  REQUIRE( f( s1, s2 ) == true  ); REQUIRE( f( s1, s3 ) == false );
  REQUIRE( f( s1, s4 ) == false ); REQUIRE( f( s1, s5 ) == false );
  REQUIRE( f( s1, s6 ) == false ); REQUIRE( f( s1, s7 ) == false );
  REQUIRE( f( s1, s8 ) == false ); REQUIRE( f( s1, s9 ) == false );
  REQUIRE( f( s2, s1 ) == false ); REQUIRE( f( s3, s1 ) == true  );
  REQUIRE( f( s4, s1 ) == false ); REQUIRE( f( s5, s1 ) == false );
  REQUIRE( f( s6, s1 ) == false ); REQUIRE( f( s7, s1 ) == false );
  REQUIRE( f( s8, s1 ) == false ); REQUIRE( f( s9, s1 ) == false );
  REQUIRE( f( s2, s2 ) == true  ); REQUIRE( f( s2, s3 ) == false );
  REQUIRE( f( s2, s4 ) == false ); REQUIRE( f( s2, s5 ) == false );
  REQUIRE( f( s2, s6 ) == false ); REQUIRE( f( s2, s7 ) == false );
  REQUIRE( f( s2, s8 ) == false ); REQUIRE( f( s2, s9 ) == false );
  REQUIRE( f( s3, s2 ) == true  ); REQUIRE( f( s4, s2 ) == false );
  REQUIRE( f( s5, s2 ) == false ); REQUIRE( f( s6, s2 ) == false );
  REQUIRE( f( s7, s2 ) == false ); REQUIRE( f( s8, s2 ) == false );
  REQUIRE( f( s9, s2 ) == false ); REQUIRE( f( s3, s3 ) == true  );
  REQUIRE( f( s3, s4 ) == true  ); REQUIRE( f( s3, s5 ) == true  );
  REQUIRE( f( s3, s6 ) == true  ); REQUIRE( f( s3, s7 ) == true  );
  REQUIRE( f( s3, s8 ) == true  ); REQUIRE( f( s3, s9 ) == true  );
  REQUIRE( f( s4, s3 ) == false ); REQUIRE( f( s5, s3 ) == false );
  REQUIRE( f( s6, s3 ) == false ); REQUIRE( f( s7, s3 ) == false );
  REQUIRE( f( s8, s3 ) == false ); REQUIRE( f( s9, s3 ) == false );
  REQUIRE( f( s4, s4 ) == true  ); REQUIRE( f( s4, s5 ) == false );
  REQUIRE( f( s4, s6 ) == true  ); REQUIRE( f( s4, s7 ) == true  );
  REQUIRE( f( s4, s8 ) == true  ); REQUIRE( f( s4, s9 ) == true  );
  REQUIRE( f( s5, s4 ) == false ); REQUIRE( f( s6, s4 ) == false );
  REQUIRE( f( s7, s4 ) == true  ); REQUIRE( f( s8, s4 ) == false );
  REQUIRE( f( s9, s4 ) == false ); REQUIRE( f( s5, s5 ) == true  );
  REQUIRE( f( s5, s6 ) == false ); REQUIRE( f( s5, s7 ) == false );
  REQUIRE( f( s5, s8 ) == false ); REQUIRE( f( s5, s9 ) == false );
  REQUIRE( f( s6, s5 ) == false ); REQUIRE( f( s7, s5 ) == false );
  REQUIRE( f( s8, s5 ) == false ); REQUIRE( f( s9, s5 ) == false );
  REQUIRE( f( s6, s6 ) == true  ); REQUIRE( f( s6, s7 ) == false );
  REQUIRE( f( s6, s8 ) == false ); REQUIRE( f( s6, s9 ) == false );
  REQUIRE( f( s7, s6 ) == true  ); REQUIRE( f( s8, s6 ) == false );
  REQUIRE( f( s9, s6 ) == false ); REQUIRE( f( s7, s7 ) == true  );
  REQUIRE( f( s7, s8 ) == true  ); REQUIRE( f( s7, s9 ) == true  );
  REQUIRE( f( s8, s7 ) == false ); REQUIRE( f( s9, s7 ) == false );
  REQUIRE( f( s8, s8 ) == true  ); REQUIRE( f( s8, s9 ) == false );
  REQUIRE( f( s9, s8 ) == true  ); REQUIRE( f( s9, s9 ) == true  );
  // clang-format on
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
TEST_CASE( "[gfx/cartesian] point::with_x/with_y" ) {
  REQUIRE( point{ .x = 1, .y = 2 }.with_x( 5 ) ==
           point{ .x = 5, .y = 2 } );
  REQUIRE( point{ .x = 1, .y = 2 }.with_y( 5 ) ==
           point{ .x = 1, .y = 5 } );
}

TEST_CASE( "[gfx/cartesian] point::operator+=( int )" ) {
  point p{ .x = 4, .y = 2 };
  int scale = 3;
  p *= scale;
  REQUIRE( p.x == 12 );
  REQUIRE( p.y == 6 );
}

TEST_CASE( "[gfx/cartesian] point::operator/=( int )" ) {
  point p{ .x = 6, .y = 2 };
  int scale = 3;
  p /= scale;
  REQUIRE( p.x == 2 );
  REQUIRE( p.y == 0 );
}

TEST_CASE( "[gfx/cartesian] point::distance_from_origin" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p.distance_from_origin() == size{ .w = 4, .h = 2 } );
}

TEST_CASE( "[gfx/cartesian] point::clamped" ) {
  rect const r{ .origin = { .x = 3, .y = 4 },
                .size   = { .w = 2, .h = 3 } };
  point p, ex;

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
  point p;
  bool ex = {};

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
  point p{ .x = 4, .y = 2 };
  size const s{ .w = 5, .h = 1 };
  p += s;
  REQUIRE( p == point{ .x = 9, .y = 3 } );
}

TEST_CASE( "[gfx/cartesian] operator-=( size )" ) {
  point p{ .x = 4, .y = 2 };
  size const s{ .w = 5, .h = 1 };
  p -= s;
  REQUIRE( p == point{ .x = -1, .y = 1 } );
}

TEST_CASE( "[gfx/cartesian] operator-( point )" ) {
  point p1{ .x = 4, .y = 2 };
  point p2{ .x = 5, .y = 1 };
  REQUIRE( p1 - p2 == size{ .w = -1, .h = 1 } );
}

TEST_CASE( "[gfx/cartesian] point::operator-( size )" ) {
  point p{ .x = 4, .y = 2 };
  size s{ .w = 5, .h = 1 };
  REQUIRE( p - s == point{ .x = -1, .y = 1 } );
}

TEST_CASE( "[gfx/cartesian] operator*( int )" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p * 10 == point{ .x = 40, .y = 20 } );
}

TEST_CASE( "[gfx/cartesian] operator/( int )" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p / 2 == point{ .x = 2, .y = 1 } );
}

TEST_CASE( "[gfx/cartesian] operator/( size )" ) {
  point p{ .x = 12, .y = 12 };
  size s{ .w = 2, .h = 4 };
  REQUIRE( p / s == point{ .x = 6, .y = 3 } );
}

TEST_CASE( "[gfx/cartesian] point::point_becomes_origin" ) {
  point const p{ .x = 4, .y = 2 };
  point const arg{ .x = 2, .y = 1 };
  point expected{ .x = 2, .y = 1 };
  REQUIRE( p.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] point::origin_becomes_point" ) {
  point const p{ .x = 4, .y = 2 };
  point const arg{ .x = 2, .y = 1 };
  point expected{ .x = 6, .y = 3 };
  REQUIRE( p.origin_becomes_point( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] point::to_double" ) {
  point const p{ .x = 4, .y = 2 };
  REQUIRE( p.to_double() == dpoint{ .x = 4.0, .y = 2.0 } );
}

TEST_CASE( "[gfx/cartesian] point::moved_*" ) {
  point p{ .x = 4, .y = 2 };
  REQUIRE( p.moved_left() == point{ .x = 3, .y = 2 } );
  REQUIRE( p.moved_left( 2 ) == point{ .x = 2, .y = 2 } );

  REQUIRE( p.moved_right() == point{ .x = 5, .y = 2 } );
  REQUIRE( p.moved_right( 2 ) == point{ .x = 6, .y = 2 } );

  REQUIRE( p.moved_up() == point{ .x = 4, .y = 1 } );
  REQUIRE( p.moved_up( 2 ) == point{ .x = 4, .y = 0 } );

  REQUIRE( p.moved_down() == point{ .x = 4, .y = 3 } );
  REQUIRE( p.moved_down( 2 ) == point{ .x = 4, .y = 4 } );
}

TEST_CASE( "[gfx/cartesian] point::moved*" ) {
  point const p{ .x = 3, .y = 5 };
  {
    using enum e_cdirection;
    REQUIRE( p.moved( nw ) == point{ .x = 2, .y = 4 } );
    REQUIRE( p.moved( n ) == point{ .x = 3, .y = 4 } );
    REQUIRE( p.moved( ne ) == point{ .x = 4, .y = 4 } );
    REQUIRE( p.moved( w ) == point{ .x = 2, .y = 5 } );
    REQUIRE( p.moved( e ) == point{ .x = 4, .y = 5 } );
    REQUIRE( p.moved( sw ) == point{ .x = 2, .y = 6 } );
    REQUIRE( p.moved( s ) == point{ .x = 3, .y = 6 } );
    REQUIRE( p.moved( se ) == point{ .x = 4, .y = 6 } );
    REQUIRE( p.moved( c ) == point{ .x = 3, .y = 5 } );
  }

  {
    using enum e_direction;
    REQUIRE( p.moved( nw ) == point{ .x = 2, .y = 4 } );
    REQUIRE( p.moved( n ) == point{ .x = 3, .y = 4 } );
    REQUIRE( p.moved( ne ) == point{ .x = 4, .y = 4 } );
    REQUIRE( p.moved( w ) == point{ .x = 2, .y = 5 } );
    REQUIRE( p.moved( e ) == point{ .x = 4, .y = 5 } );
    REQUIRE( p.moved( sw ) == point{ .x = 2, .y = 6 } );
    REQUIRE( p.moved( s ) == point{ .x = 3, .y = 6 } );
    REQUIRE( p.moved( se ) == point{ .x = 4, .y = 6 } );
  }

  {
    using enum e_cardinal_direction;
    REQUIRE( p.moved( n ) == point{ .x = 3, .y = 4 } );
    REQUIRE( p.moved( w ) == point{ .x = 2, .y = 5 } );
    REQUIRE( p.moved( e ) == point{ .x = 4, .y = 5 } );
    REQUIRE( p.moved( s ) == point{ .x = 3, .y = 6 } );
  }

  {
    using enum e_diagonal_direction;
    REQUIRE( p.moved( nw ) == point{ .x = 2, .y = 4 } );
    REQUIRE( p.moved( ne ) == point{ .x = 4, .y = 4 } );
    REQUIRE( p.moved( sw ) == point{ .x = 2, .y = 6 } );
    REQUIRE( p.moved( se ) == point{ .x = 4, .y = 6 } );
  }
}

TEST_CASE( "[gfx/cartesian] point::direction_to" ) {
  point const p = { .x = 1, .y = 2 };

  REQUIRE( p.direction_to( { .x = 0, .y = 1 } ) ==
           e_direction::nw );
  REQUIRE( p.direction_to( { .x = 1, .y = 1 } ) ==
           e_direction::n );
  REQUIRE( p.direction_to( { .x = 2, .y = 1 } ) ==
           e_direction::ne );
  REQUIRE( p.direction_to( { .x = 0, .y = 2 } ) ==
           e_direction::w );
  REQUIRE( p.direction_to( { .x = 2, .y = 2 } ) ==
           e_direction::e );
  REQUIRE( p.direction_to( { .x = 0, .y = 3 } ) ==
           e_direction::sw );
  REQUIRE( p.direction_to( { .x = 1, .y = 3 } ) ==
           e_direction::s );
  REQUIRE( p.direction_to( { .x = 2, .y = 3 } ) ==
           e_direction::se );

  REQUIRE( p.direction_to( { .x = 1, .y = 2 } ) == nothing );
  REQUIRE( p.direction_to( { .x = 3, .y = 2 } ) == nothing );
  REQUIRE( p.direction_to( { .x = 1, .y = 4 } ) == nothing );
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
  dpoint p, ex;

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
  dpoint p;
  bool ex = {};

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
  dsize s{ .w = 5.2, .h = 1.5 };
  p += s;
  REQUIRE( p.x == 9.6_a );
  REQUIRE( p.y == 3.9_a );
}

TEST_CASE( "[gfx/cartesian] dpoint::operator-=( dsize )" ) {
  dpoint p{ .x = 4.4, .y = 2.4 };
  dsize s{ .w = 5.2, .h = 1.5 };
  p -= s;
  REQUIRE( p.x == -.8_a );
  REQUIRE( p.y == .9_a );
  REQUIRE( ( p - s ).x == -6.0_a );
  REQUIRE( ( p - s ).y == -.6_a );
}

TEST_CASE( "[gfx/cartesian] operator-( dsize )" ) {
  dpoint p{ .x = 4, .y = 2 };
  dsize s{ .w = 5.2, .h = 1.5 };
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
  dpoint expected{ .x = 2.2, .y = 1 };
  REQUIRE( p.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] dpoint::origin_becomes_point" ) {
  dpoint const p{ .x = 4.2, .y = 2 };
  dpoint const arg{ .x = 2, .y = 1 };
  dpoint expected{ .x = 6.2, .y = 3 };
  REQUIRE( p.origin_becomes_point( arg ) == expected );
}

/****************************************************************
** rect
*****************************************************************/
TEST_CASE( "[gfx/cartesian] rect::from" ) {
  point p1{ .x = 4, .y = 2 };
  point p2{ .x = 2, .y = 4 };
  REQUIRE( rect::from( p1, p2 ) ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 2, .h = 2 } } );
  REQUIRE( rect::from( p2, p1 ) ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 2, .h = 2 } } );
  point p3{ .x = 2, .y = 2 };
  point p4{ .x = 4, .y = 4 };
  REQUIRE( rect::from( p3, p4 ) ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 2, .h = 2 } } );
  REQUIRE( rect::from( p4, p3 ) ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 2, .h = 2 } } );
}

TEST_CASE( "[gfx/cartesian] rect::moved_*" ) {
  rect const r{ .origin = { .x = 4, .y = 2 },
                .size   = { .w = 2, .h = 3 } };
  REQUIRE( r.moved_left() ==
           rect{ .origin = { .x = 3, .y = 2 },
                 .size   = { .w = 2, .h = 3 } } );
  REQUIRE( r.moved_left( 2 ) ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 2, .h = 3 } } );

  REQUIRE( r.moved_right() ==
           rect{ .origin = { .x = 5, .y = 2 },
                 .size   = { .w = 2, .h = 3 } } );
  REQUIRE( r.moved_right( 2 ) ==
           rect{ .origin = { .x = 6, .y = 2 },
                 .size   = { .w = 2, .h = 3 } } );

  REQUIRE( r.moved_up() == rect{ .origin = { .x = 4, .y = 1 },
                                 .size = { .w = 2, .h = 3 } } );
  REQUIRE( r.moved_up( 2 ) ==
           rect{ .origin = { .x = 4, .y = 0 },
                 .size   = { .w = 2, .h = 3 } } );

  REQUIRE( r.moved_down() ==
           rect{ .origin = { .x = 4, .y = 3 },
                 .size   = { .w = 2, .h = 3 } } );
  REQUIRE( r.moved_down( 2 ) ==
           rect{ .origin = { .x = 4, .y = 4 },
                 .size   = { .w = 2, .h = 3 } } );
}

TEST_CASE( "[gfx/cartesian] rect::moved*" ) {
  rect const r{ .origin = { .x = 3, .y = 5 },
                .size   = { .w = 2, .h = 3 } };
  {
    using enum e_cdirection;
    REQUIRE( r.moved( nw ) ==
             rect{ .origin = { .x = 2, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( n ) ==
             rect{ .origin = { .x = 3, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( ne ) ==
             rect{ .origin = { .x = 4, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( w ) ==
             rect{ .origin = { .x = 2, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( e ) ==
             rect{ .origin = { .x = 4, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( sw ) ==
             rect{ .origin = { .x = 2, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( s ) ==
             rect{ .origin = { .x = 3, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( se ) ==
             rect{ .origin = { .x = 4, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( c ) ==
             rect{ .origin = { .x = 3, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
  }

  {
    using enum e_direction;
    REQUIRE( r.moved( nw ) ==
             rect{ .origin = { .x = 2, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( n ) ==
             rect{ .origin = { .x = 3, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( ne ) ==
             rect{ .origin = { .x = 4, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( w ) ==
             rect{ .origin = { .x = 2, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( e ) ==
             rect{ .origin = { .x = 4, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( sw ) ==
             rect{ .origin = { .x = 2, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( s ) ==
             rect{ .origin = { .x = 3, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( se ) ==
             rect{ .origin = { .x = 4, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
  }

  {
    using enum e_cardinal_direction;
    REQUIRE( r.moved( n ) ==
             rect{ .origin = { .x = 3, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( w ) ==
             rect{ .origin = { .x = 2, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( e ) ==
             rect{ .origin = { .x = 4, .y = 5 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( s ) ==
             rect{ .origin = { .x = 3, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
  }

  {
    using enum e_diagonal_direction;
    REQUIRE( r.moved( nw ) ==
             rect{ .origin = { .x = 2, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( ne ) ==
             rect{ .origin = { .x = 4, .y = 4 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( sw ) ==
             rect{ .origin = { .x = 2, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
    REQUIRE( r.moved( se ) ==
             rect{ .origin = { .x = 4, .y = 6 },
                   .size   = { .w = 2, .h = 3 } } );
  }
}

TEST_CASE( "[gfx/cartesian] with_border_added" ) {
  rect const r{ .origin = { .x = 3, .y = 4 },
                .size   = { .w = 1, .h = 3 } };

  REQUIRE( r.with_border_added() ==
           rect{ .origin = { .x = 2, .y = 3 },
                 .size   = { .w = 3, .h = 5 } } );

  REQUIRE( r.with_border_added( 2 ) ==
           rect{ .origin = { .x = 1, .y = 2 },
                 .size   = { .w = 5, .h = 7 } } );
}

TEST_CASE( "[gfx/cartesian] with_edges_removed" ) {
  rect r;

  r = { .origin = { .x = 3, .y = 4 },
        .size   = { .w = 1, .h = 3 } };

  REQUIRE( r.with_edges_removed() ==
           rect{ .origin = { .x = 4, .y = 5 },
                 .size   = { .w = 0, .h = 1 } } );

  REQUIRE( r.with_edges_removed( 2 ) ==
           rect{ .origin = { .x = 5, .y = 6 },
                 .size   = { .w = 0, .h = 0 } } );

  r = { .origin = { .x = 1, .y = 1 },
        .size   = { .w = 1, .h = 1 } };
  REQUIRE( r.with_edges_removed( 2 ) ==
           rect{ .origin = { .x = 2, .y = 2 },
                 .size   = { .w = 0, .h = 0 } } );

  r = { .origin = { .x = 1, .y = 1 },
        .size   = { .w = 0, .h = 0 } };
  REQUIRE( r.with_edges_removed( 2 ) ==
           rect{ .origin = { .x = 1, .y = 1 },
                 .size   = { .w = 0, .h = 0 } } );
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
    "[gfx/cartesian] rect::{horizontal,vertical}_slice" ) {
  rect const r1{ .origin = { .x = 3, .y = 4 },
                 .size   = { .w = 1, .h = 3 } };
  REQUIRE( r1.horizontal_slice() ==
           interval{ .start = 3, .len = 1 } );
  REQUIRE( r1.vertical_slice() ==
           interval{ .start = 4, .len = 3 } );

  rect const r2{ .origin = { .x = 3, .y = 4 },
                 .size   = { .w = -1, .h = -3 } };
  REQUIRE( r2.horizontal_slice() ==
           interval{ .start = 3, .len = -1 } );
  REQUIRE( r2.vertical_slice() ==
           interval{ .start = 4, .len = -3 } );
}

TEST_CASE( "[gfx/cartesian] rect::corner" ) {
  rect r;

  REQUIRE( r.corner( e_diagonal_direction::nw ) == point{} );
  REQUIRE( r.corner( e_diagonal_direction::ne ) == point{} );
  REQUIRE( r.corner( e_diagonal_direction::se ) == point{} );
  REQUIRE( r.corner( e_diagonal_direction::sw ) == point{} );

  r = rect{ .origin = { .x = 3, .y = 4 },
            .size   = { .w = 1, .h = 3 } };
  REQUIRE( r.corner( e_diagonal_direction::nw ) ==
           point{ .x = 3, .y = 4 } );
  REQUIRE( r.corner( e_diagonal_direction::ne ) ==
           point{ .x = 4, .y = 4 } );
  REQUIRE( r.corner( e_diagonal_direction::se ) ==
           point{ .x = 4, .y = 7 } );
  REQUIRE( r.corner( e_diagonal_direction::sw ) ==
           point{ .x = 3, .y = 7 } );
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

TEST_CASE( "[gfx/cartesian] rect::with_size" ) {
  rect r{ .origin = { .x = 4, .y = 2 },
          .size   = { .w = 2, .h = 4 } };
  rect expected{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = 3, .h = 1 } };
  REQUIRE( r.with_size( size{ .w = 3, .h = 1 } ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::center" ) {
  rect r{ .origin = { .x = 4, .y = 2 },
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
  rect const r{ .origin = { .x = 4, .y = 2 },
                .size   = { .w = 7, .h = 8 } };
  point const arg{ .x = 2, .y = 1 };
  rect expected{ .origin = { .x = 2, .y = 1 },
                 .size   = { .w = 7, .h = 8 } };
  REQUIRE( r.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::origin_becomes_point" ) {
  rect const r{ .origin = { .x = 4, .y = 2 },
                .size   = { .w = 7, .h = 8 } };
  point const arg{ .x = 2, .y = 1 };
  rect expected{ .origin = { .x = 6, .y = 3 },
                 .size   = { .w = 7, .h = 8 } };
  REQUIRE( r.origin_becomes_point( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::to_double" ) {
  rect r{ .origin = { .x = 3, .y = 4 },
          .size   = { .w = 4, .h = 2 } };
  REQUIRE( r.to_double() ==
           drect{ .origin = { .x = 3.0, .y = 4.0 },
                  .size   = { .w = 4.0, .h = 2.0 } } );
}

TEST_CASE( "[gfx/cartesian] rect::with_new_right_edge" ) {
  auto const r = rect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 7, .h = 9 } };
  int new_edge{};
  rect expect{};

  new_edge = 7;
  expect   = { .origin = { .x = 5, .y = 5 },
               .size   = { .w = 2, .h = 9 } };
  REQUIRE( r.with_new_right_edge( new_edge ) == expect );

  new_edge = 50;
  expect   = { .origin = { 5, 5 }, .size = { 45, 9 } };
  REQUIRE( r.with_new_right_edge( new_edge ) == expect );
}

TEST_CASE( "[gfx/cartesian] rect::with_new_left_edge" ) {
  auto const r = rect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 7, .h = 9 } };
  int new_edge{};
  rect expect{};

  new_edge = 7;
  expect   = { .origin = { .x = 7, .y = 5 },
               .size   = { .w = 5, .h = 9 } };
  REQUIRE( r.with_new_left_edge( new_edge ) == expect );

  new_edge = 3;
  expect   = { .origin = { .x = 3, .y = 5 },
               .size   = { .w = 9, .h = 9 } };
  REQUIRE( r.with_new_left_edge( new_edge ) == expect );
}

TEST_CASE( "[gfx/cartesian] rect::with_new_top_edge" ) {
  auto const r = rect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 7, .h = 9 } };
  int new_edge{};
  rect expect{};

  new_edge = 7;
  expect   = { .origin = { .x = 5, .y = 7 },
               .size   = { .w = 7, .h = 7 } };
  REQUIRE( r.with_new_top_edge( new_edge ) == expect );

  new_edge = 3;
  expect   = { .origin = { .x = 5, .y = 3 },
               .size   = { .w = 7, .h = 11 } };
  REQUIRE( r.with_new_top_edge( new_edge ) == expect );
}

TEST_CASE( "[gfx/cartesian] rect::with_new_bottom_edge" ) {
  auto const r = rect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 7, .h = 9 } };
  int new_edge{};
  rect expect{};

  new_edge = 7;
  expect   = { .origin = { .x = 5, .y = 5 },
               .size   = { .w = 7, .h = 2 } };
  REQUIRE( r.with_new_bottom_edge( new_edge ) == expect );

  new_edge = 50;
  expect   = { .origin = { .x = 5, .y = 5 },
               .size   = { .w = 7, .h = 45 } };
  REQUIRE( r.with_new_bottom_edge( new_edge ) == expect );
}

TEST_CASE( "[gfx/cartesian] rect::with_inc_size" ) {
  rect r{ .origin = { .x = 4, .y = 2 },
          .size   = { .w = 2, .h = 4 } };
  rect expected{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = 3, .h = 5 } };
  REQUIRE( r.with_inc_size() == expected );
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 4, .h = 6 } };
  REQUIRE( r.with_inc_size( 2 ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::with_dec_size" ) {
  rect r{ .origin = { .x = 4, .y = 2 },
          .size   = { .w = 2, .h = 4 } };
  rect expected{ .origin = { .x = 4, .y = 2 },
                 .size   = { .w = 1, .h = 3 } };
  REQUIRE( r.with_dec_size() == expected );
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 0, .h = 2 } };
  REQUIRE( r.with_dec_size( 2 ) == expected );
  expected = { .origin = { .x = 4, .y = 2 },
               .size   = { .w = 0, .h = 0 } };
  REQUIRE( r.with_dec_size( 10 ) == expected );
}

TEST_CASE( "[gfx/cartesian] rect::uni0n" ) {
  rect const r1{ .origin{ .x = 1, .y = 1 },
                 .size = { .w = 1, .h = 1 } };
  rect const r2{ .origin{ .x = 1, .y = 2 },
                 .size = { .w = 1, .h = 1 } };
  rect const r3{ .origin{ .x = 1, .y = 1 },
                 .size = { .w = 2, .h = 1 } };
  rect const r4{ .origin{ .x = 3, .y = 3 },
                 .size = { .w = 2, .h = 2 } };
  rect const r5{ .origin{ .x = 1, .y = -1 },
                 .size = { .w = 1, .h = 10 } };
  rect const r6{ .origin{ .x = 5, .y = 5 },
                 .size = { .w = 0, .h = 0 } };

  // Self tests.
  REQUIRE( r1.uni0n( r1 ) == r1 );
  REQUIRE( r2.uni0n( r2 ) == r2 );
  REQUIRE( r3.uni0n( r3 ) == r3 );
  REQUIRE( r4.uni0n( r4 ) == r4 );
  REQUIRE( r5.uni0n( r5 ) == r5 );
  REQUIRE( r6.uni0n( r6 ) == r6 );

  rect expected;

  // Cross tests.
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.uni0n( r2 ) == expected );
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 2, .h = 1 } };
  REQUIRE( r1.uni0n( r3 ) == expected );
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 4, .h = 4 } };
  REQUIRE( r1.uni0n( r4 ) == expected );
  expected = { .origin = { .x = 1, .y = -1 },
               .size   = { .w = 1, .h = 10 } };
  REQUIRE( r1.uni0n( r5 ) == expected );
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 4, .h = 4 } };
  REQUIRE( r1.uni0n( r6 ) == expected );
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 2, .h = 2 } };
  REQUIRE( r2.uni0n( r3 ) == expected );
  expected = { .origin = { .x = 1, .y = 2 },
               .size   = { .w = 4, .h = 3 } };
  REQUIRE( r2.uni0n( r4 ) == expected );
  expected = { .origin = { .x = 1, .y = -1 },
               .size   = { .w = 1, .h = 10 } };
  REQUIRE( r2.uni0n( r5 ) == expected );
  expected = { .origin = { .x = 1, .y = 2 },
               .size   = { .w = 4, .h = 3 } };
  REQUIRE( r2.uni0n( r6 ) == expected );
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 4, .h = 4 } };
  REQUIRE( r3.uni0n( r4 ) == expected );
  expected = { .origin = { .x = 1, .y = -1 },
               .size   = { .w = 2, .h = 10 } };
  REQUIRE( r3.uni0n( r5 ) == expected );
  expected = { .origin = { .x = 1, .y = 1 },
               .size   = { .w = 4, .h = 4 } };
  REQUIRE( r3.uni0n( r6 ) == expected );
  expected = { .origin = { .x = 1, .y = -1 },
               .size   = { .w = 4, .h = 10 } };
  REQUIRE( r4.uni0n( r5 ) == expected );
  expected = { .origin = { .x = 3, .y = 3 },
               .size   = { .w = 2, .h = 2 } };
  REQUIRE( r4.uni0n( r6 ) == expected );
  expected = { .origin = { .x = 1, .y = -1 },
               .size   = { .w = 4, .h = 10 } };
  REQUIRE( r5.uni0n( r6 ) == expected );
}

/****************************************************************
** drect
*****************************************************************/
TEST_CASE( "[gfx/cartesian] drect::truncated" ) {
  dsize const s{ .w = 4.3, .h = 2.1 };
  dpoint const p{ .x = 4.4, .y = 2.4 };
  drect const r{ .origin = p, .size = s };
  REQUIRE( r.truncated() == rect{ .origin = { .x = 4, .y = 2 },
                                  .size = { .w = 4, .h = 2 } } );
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

  r1 = drect{
    .origin = { .x = -16.72272810753941, .y = 459.980374966315 },
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
  drect const r{ .origin = { .x = 4.2, .y = 2 },
                 .size   = { .w = 7.2, .h = 8 } };
  dpoint const arg{ .x = 2.1, .y = 1 };
  drect expected{ .origin = { .x = 2.1, .y = 1 },
                  .size   = { .w = 7.2, .h = 8 } };
  REQUIRE( r.point_becomes_origin( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] drect::origin_becomes_point" ) {
  drect const r{ .origin = { .x = 4.2, .y = 2 },
                 .size   = { .w = 7.2, .h = 8 } };
  dpoint const arg{ .x = 2.1, .y = 1 };
  drect const res = r.origin_becomes_point( arg );
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
  size s{ .w = 2, .h = 8 };
  REQUIRE( p + s == point{ .x = 6, .y = 10 } );
  REQUIRE( s + p == point{ .x = 6, .y = 10 } );
}

TEST_CASE( "[gfx/cartesian] point += size" ) {
  point p{ .x = 4, .y = 2 };
  size s{ .w = 2, .h = 8 };
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
  size s{ .w = 2, .h = 8 };
  REQUIRE( p * s == point{ .x = 8, .y = 16 } );
}

TEST_CASE( "[gfx/cartesian] dpoint + dsize" ) {
  dpoint p{ .x = 4, .y = 2 };
  dsize s{ .w = 2, .h = 8 };
  REQUIRE( p + s == dpoint{ .x = 6, .y = 10 } );
  REQUIRE( s + p == dpoint{ .x = 6, .y = 10 } );
}

/****************************************************************
** Free Functions
*****************************************************************/
TEST_CASE( "[gfx/cartesian] centered_in (double)" ) {
  drect rect;
  dsize delta;
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

TEST_CASE( "[gfx/cartesian] centered_in (int)" ) {
  rect r;
  size delta;
  point expect;

  r      = rect{ .origin = { .x = 1, .y = 1 },
                 .size   = { .w = 0, .h = 0 } };
  delta  = size{ .w = 4, .h = 3 };
  expect = point{ .x = -1, .y = 0 };
  REQUIRE( centered_in( delta, r ) == expect );

  r      = rect{ .origin = { .x = 1, .y = 2 },
                 .size   = { .w = 5, .h = 6 } };
  delta  = size{ .w = 3, .h = 4 };
  expect = point{ .x = 2, .y = 3 };
  REQUIRE( centered_in( delta, r ) == expect );
}

TEST_CASE( "[coord] centered_at/e_cdirection" ) {
  rect r;
  size s;
  e_cdirection d = {};
  point expect;

  auto f = [&] { return centered_at( s, r, d ); };

  r      = { .origin = { .x = 1, .y = 1 },
             .size   = { .w = 0, .h = 0 } };
  s      = { .w = 4, .h = 3 };
  d      = e_cdirection::c;
  expect = { .x = -1, .y = 0 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::c;
  expect = { .x = 2, .y = 3 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::s;
  expect = { .x = 2, .y = 4 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::n;
  expect = { .x = 2, .y = 2 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::w;
  expect = { .x = 1, .y = 3 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 4, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::e;
  expect = { .x = 2, .y = 3 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::sw;
  expect = { .x = 1, .y = 4 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::nw;
  expect = { .x = 1, .y = 2 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::se;
  expect = { .x = 3, .y = 4 };
  REQUIRE( f() == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 4, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  d      = e_cdirection::ne;
  expect = { .x = 2, .y = 2 };
  REQUIRE( f() == expect );
}

TEST_CASE( "[coord] centered_at_*" ) {
  rect r;
  size s;
  point expect;

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 4 };
  REQUIRE( centered_at_bottom( s, r ) == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 2 };
  REQUIRE( centered_at_top( s, r ) == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 5, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 1, .y = 3 };
  REQUIRE( centered_at_left( s, r ) == expect );

  r      = { .origin = { .x = 1, .y = 2 },
             .size   = { .w = 4, .h = 6 } };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 3 };
  REQUIRE( centered_at_right( s, r ) == expect );
}

TEST_CASE( "[gfx/cartesian] centered_on" ) {
  rect expected;
  size sz;
  point p;

  auto f = [&] { return centered_on( sz, p ); };

  p        = { .x = 2, .y = 3 };
  sz       = { .w = 2, .h = 8 };
  expected = { .origin = { .x = 1, .y = -1 }, .size = sz };
  REQUIRE( f() == expected );

  p        = { .x = 2, .y = 3 };
  sz       = { .w = 2, .h = 9 };
  expected = { .origin = { .x = 1, .y = -1 }, .size = sz };
  REQUIRE( f() == expected );

  p        = { .x = 2, .y = 3 };
  sz       = { .w = 2, .h = 10 };
  expected = { .origin = { .x = 1, .y = -2 }, .size = sz };
  REQUIRE( f() == expected );

  p        = { .x = 0, .y = 0 };
  sz       = { .w = 2, .h = 10 };
  expected = { .origin = { .x = -1, .y = -5 }, .size = sz };
  REQUIRE( f() == expected );

  p        = { .x = 0, .y = 0 };
  sz       = { .w = 0, .h = 0 };
  expected = { .origin = { .x = 0, .y = 0 }, .size = sz };
  REQUIRE( f() == expected );

  p        = { .x = 2, .y = 2 };
  sz       = { .w = 0, .h = 0 };
  expected = { .origin = { .x = 2, .y = 2 }, .size = sz };
  REQUIRE( f() == expected );
}

/****************************************************************
** oriented_point
*****************************************************************/
TEST_CASE( "[gfx/cartesian] oriented_point (refl/to_str)" ) {
  oriented_point const op{ .anchor    = { .x = 4, .y = 2 },
                           .placement = e_cdirection::sw };
  static_assert( base::Show<oriented_point> );
  REQUIRE( base::to_str( op ) ==
           "gfx::oriented_point{anchor=gfx::point{x=4,y=2},"
           "placement=sw}" );
}

TEST_CASE(
    "[gfx/cartesian] oriented_point::point_becomes_origin" ) {
  oriented_point const op{ .anchor    = { .x = 4, .y = 2 },
                           .placement = e_cdirection::sw };
  point const arg{ .x = 2, .y = 1 };
  oriented_point const expected{ .anchor    = { .x = 2, .y = 1 },
                                 .placement = e_cdirection::sw };
  REQUIRE( op.point_becomes_origin( arg ) == expected );
}

TEST_CASE(
    "[gfx/cartesian] oriented_point::origin_becomes_point" ) {
  oriented_point const op{ .anchor    = { .x = 4, .y = 2 },
                           .placement = e_cdirection::sw };
  point const arg{ .x = 2, .y = 1 };
  oriented_point const expected{ .anchor    = { .x = 6, .y = 3 },
                                 .placement = e_cdirection::sw };
  REQUIRE( op.origin_becomes_point( arg ) == expected );
}

TEST_CASE( "[gfx/cartesian] find_placement" ) {
  size s;
  oriented_point op;
  point expect;

  auto f = [&] { return find_placement( op, s ); };

  op     = { .anchor    = { .x = 1, .y = 1 },
             .placement = e_cdirection::c };
  s      = { .w = 4, .h = 3 };
  expect = { .x = -1, .y = 0 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 3, .y = 5 },
             .placement = e_cdirection::c };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 3 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 3, .y = 8 },
             .placement = e_cdirection::s };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 4 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 3, .y = 2 },
             .placement = e_cdirection::n };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 2 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 1, .y = 5 },
             .placement = e_cdirection::w };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 1, .y = 3 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 5, .y = 5 },
             .placement = e_cdirection::e };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 3 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 1, .y = 8 },
             .placement = e_cdirection::sw };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 1, .y = 4 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 1, .y = 2 },
             .placement = e_cdirection::nw };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 1, .y = 2 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 6, .y = 8 },
             .placement = e_cdirection::se };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 3, .y = 4 };
  REQUIRE( f() == expect );

  op     = { .anchor    = { .x = 5, .y = 2 },
             .placement = e_cdirection::ne };
  s      = { .w = 3, .h = 4 };
  expect = { .x = 2, .y = 2 };
  REQUIRE( f() == expect );
}

/****************************************************************
** std::hash<gfx::point>
*****************************************************************/
TEST_CASE( "[gfx/cartesian] hash point" ) {
  static std::hash<point> const hasher{};

  point p = {};

  p = {};
  REQUIRE( hasher( p ) == 0x0000000000000000ULL );

  p = { .x = 1 };
  REQUIRE( hasher( p ) == 0x0000000000000001ULL );

  p = { .y = 1 };
  REQUIRE( hasher( p ) == 0x0000000100000000ULL );

  p = { .x = 1, .y = 1 };
  REQUIRE( hasher( p ) == 0x0000000100000001ULL );

  p = { .x = -1, .y = 1 };
  REQUIRE( hasher( p ) == 0x00000001ffffffffULL );

  p = { .x = 1, .y = -1 };
  REQUIRE( hasher( p ) == 0xffffffff00000001ULL );

  p = { .x = 0x34500, .y = 0x56770 };
  REQUIRE( hasher( p ) == 0x0005677000034500ULL );

  p = { .x = -1, .y = -1 };
  REQUIRE( hasher( p ) == 0xffffffffffffffffULL );

  p = { .x = numeric_limits<int>::max(),
        .y = numeric_limits<int>::max() };
  REQUIRE( hasher( p ) == 0x7fffffff7fffffffULL );
}

/****************************************************************
** std::hash<gfx::size>
*****************************************************************/
TEST_CASE( "[gfx/cartesian] hash size" ) {
  static std::hash<size> const hasher{};

  size s = {};

  s = {};
  REQUIRE( hasher( s ) == 0x0000000000000000ULL );

  s = { .w = 1 };
  REQUIRE( hasher( s ) == 0x0000000000000001ULL );

  s = { .h = 1 };
  REQUIRE( hasher( s ) == 0x0000000100000000ULL );

  s = { .w = 1, .h = 1 };
  REQUIRE( hasher( s ) == 0x0000000100000001ULL );

  s = { .w = -1, .h = 1 };
  REQUIRE( hasher( s ) == 0x00000001ffffffffULL );

  s = { .w = 1, .h = -1 };
  REQUIRE( hasher( s ) == 0xffffffff00000001ULL );

  s = { .w = 0x34500, .h = 0x56770 };
  REQUIRE( hasher( s ) == 0x0005677000034500ULL );

  s = { .w = -1, .h = -1 };
  REQUIRE( hasher( s ) == 0xffffffffffffffffULL );

  s = { .w = numeric_limits<int>::max(),
        .h = numeric_limits<int>::max() };
  REQUIRE( hasher( s ) == 0x7fffffff7fffffffULL );
}

/****************************************************************
** Strong Types.
*****************************************************************/
TEST_CASE( "[gfx/cartesian] strong types" ) {
  REQUIRE( X{ 2 }.n == 2 );
  REQUIRE( X{ 2 } == X{ 2 } );
  REQUIRE( X{ 1 } != X{ 2 } );
  REQUIRE( X{ 1 } <= X{ 2 } );
  REQUIRE( X{ 1 } < X{ 2 } );
  REQUIRE( base::to_str( X{ 3 } ) == "gfx::X{n=3}" );

  {
    constexpr X cx = X{ 5 };
    static_assert( cx.n == 5 );
    REQUIRE( cx.n == 5 );
  }
  {
    constexpr Y cy = Y{ 5 };
    static_assert( cy.n == 5 );
    REQUIRE( cy.n == 5 );
  }

  REQUIRE( Y{ 2 }.n == 2 );
  REQUIRE( Y{ 2 } == Y{ 2 } );
  REQUIRE( Y{ 1 } != Y{ 2 } );
  REQUIRE( Y{ 1 } <= Y{ 2 } );
  REQUIRE( Y{ 1 } < Y{ 2 } );
  REQUIRE( base::to_str( Y{ 3 } ) == "gfx::Y{n=3}" );
}

/****************************************************************
** UDLs
*****************************************************************/
TEST_CASE( "[gfx/cartesian] UDLs" ) {
  using namespace ::gfx::literals;

  REQUIRE( 2_x == X{ 2 } );
  REQUIRE( 4_x == X{ 4 } );
  REQUIRE( 2_y == Y{ 2 } );
  REQUIRE( 4_y == Y{ 4 } );

  REQUIRE( ( 4_x, 3_y ) == point{ .x = 4, .y = 3 } );

  {
    constexpr X cx = 5_x;
    static_assert( cx.n == 5 );
    REQUIRE( cx.n == 5 );
  }
  {
    constexpr Y cy = 5_y;
    static_assert( cy.n == 5 );
    REQUIRE( cy.n == 5 );
  }

  auto constexpr p = ( 4_x, 3_y );
  REQUIRE( p == point{ .x = 4, .y = 3 } );

  auto constexpr ny = ( 4_x, -3_y );
  REQUIRE( ny == point{ .x = 4, .y = -3 } );

  auto constexpr nx = ( -4_x, 3_y );
  REQUIRE( nx == point{ .x = -4, .y = 3 } );
}

/****************************************************************
** Interconversion (FIXME: temporary)
*****************************************************************/
TEST_CASE( "[gfx/cartesian] interconversion" ) {
  ::rn::Coord const c = point{ .x = 6, .y = 7 };
  REQUIRE( c == ::rn::Coord{ .x = 6, .y = 7 } );

  ::rn::Delta const d = size{ .w = 6, .h = 7 };
  REQUIRE( d == ::rn::Delta{ .w = 6, .h = 7 } );

  ::rn::Rect const r = rect{ .origin = { .x = 1, .y = 2 },
                             .size   = { .w = 6, .h = 7 } };
  REQUIRE( r == ::rn::Rect{ .x = 1, .y = 2, .w = 6, .h = 7 } );
}

} // namespace
} // namespace gfx
