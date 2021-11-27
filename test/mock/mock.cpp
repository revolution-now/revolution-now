/****************************************************************
**mock.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-11.
*
* Description: Unit tests for the src/mock/mock.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/mock/mock.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace mock {
namespace {

using namespace std;
using ::Catch::Matches;
using ::mock::matchers::_;

/****************************************************************
** IPoint
*****************************************************************/
struct IPoint {
  virtual ~IPoint() = default;

  virtual int get_x() const = 0;

  virtual int get_y() const = 0;

  virtual bool get_xy( int* x_out, int& y_out ) const = 0;

  virtual void set_x( int x ) = 0;

  virtual void set_y( int y ) = 0;

  virtual void set_xy( int x, int y ) = 0;

  virtual double length() const = 0;
};

/****************************************************************
** MockPoint
*****************************************************************/
struct MockPoint : IPoint {
  MOCK_METHOD( int, get_x, (), ( const ) );
  MOCK_METHOD( int, get_y, (), ( const ) );
  MOCK_METHOD( bool, get_xy, (int*, int&), ( const ) );
  MOCK_METHOD( void, set_x, (int), () );
  MOCK_METHOD( void, set_y, (int), () );
  MOCK_METHOD( void, set_xy, (int, int), () );
  MOCK_METHOD( double, length, (), ( const ) );
};

/****************************************************************
** PointUser
*****************************************************************/
struct PointUser {
  PointUser( IPoint* p ) : p_( p ) {
    DCHECK( p_ != nullptr );
    p_->set_xy( 3, 4 );
  }

  int increment_y() {
    int new_val = p_->get_y() + 1;
    p_->set_y( new_val );
    return new_val;
  }

  int some_method_1( int new_x ) {
    p_->set_x( new_x );
    increment_y();
    return p_->length();
  }

  int some_method_2() {
    return some_method_1( 42 ) + p_->get_x();
  }

  int get_x() const { return p_->get_x(); }

  int get_y() const { return p_->get_y(); }

  bool get_xy( int* x_out, int& y_out ) const {
    return p_->get_xy( x_out, y_out );
  }

  IPoint* p_;
};

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[mock] one off calls" ) {
  MockPoint mp;

  EXPECT_CALL( mp, set_xy( 3, 4 ) );
  PointUser user( &mp );

  EXPECT_CALL( mp, get_x() ).returns( 7 );
  REQUIRE( user.get_x() == 7 );

  EXPECT_CALL( mp, get_y() ).returns( 10 );
  REQUIRE( user.get_y() == 10 );

  EXPECT_CALL( mp, get_y() ).returns( 10 );
  EXPECT_CALL( mp, set_y( 11 ) );
  REQUIRE( user.increment_y() == 11 );
}

TEST_CASE( "[mock] repeated calls" ) {
  MockPoint mp;

  EXPECT_CALL( mp, set_xy( 3, 4 ) );
  PointUser user( &mp );

  EXPECT_CALL( mp, get_x() ).times( 3 ).returns( 7 );
  REQUIRE( user.get_x() == 7 );
  REQUIRE( user.get_x() == 7 );
  REQUIRE( user.get_x() == 7 );
  REQUIRE_THROWS_WITH(
      user.get_x(),
      Matches( "unexpected mock function call.*" ) );

  EXPECT_CALL( mp, get_x() ).times( 1 ).returns( 8 );
  EXPECT_CALL( mp, get_x() ).times( 2 ).returns( 9 );
  REQUIRE( user.get_x() == 8 );
  REQUIRE( user.get_x() == 9 );
  REQUIRE( user.get_x() == 9 );

  REQUIRE_THROWS_WITH(
      user.get_x(),
      Matches( "unexpected mock function call.*" ) );
}

TEST_CASE( "[mock] sets_arg" ) {
  MockPoint mp;

  EXPECT_CALL( mp, set_xy( 3, 4 ) );
  PointUser user( &mp );

  EXPECT_CALL( mp, get_xy( _, _ ) )
      .sets_arg<0>( 5 )
      .sets_arg<1>( 6 )
      .returns( true );
  int x = 0, y = 0;
  REQUIRE( user.get_xy( &x, y ) == true );
  REQUIRE( x == 5 );
  REQUIRE( y == 6 );
}

TEST_CASE( "[mock] any" ) {
  MockPoint mp;

  EXPECT_CALL( mp, set_xy( _, 4 ) );
  PointUser user( &mp );

  EXPECT_CALL( mp, get_y() ).returns( 10 );
  EXPECT_CALL( mp, set_y( _ ) );
  REQUIRE( user.increment_y() == 11 );
}

TEST_CASE(
    "[mock] throws on unexpected mock function argument" ) {
  MockPoint mp;

  EXPECT_CALL( mp, set_xy( _, 5 ) );
  REQUIRE_THROWS_WITH( PointUser( &mp ),
                       Matches( ".*unexpected arguments.*" ) );
  // Make the expected call to an error isn't thrown.
  mp.set_xy( 0, 5 );
}

TEST_CASE( "[mock] throws on unexpected mock call" ) {
  MockPoint mp;

  EXPECT_CALL( mp, set_xy( _, 4 ) );
  PointUser user( &mp );

  REQUIRE_THROWS_WITH(
      user.increment_y(),
      Matches( "unexpected mock function call.*get_y.*" ) );
}

} // namespace
} // namespace mock
