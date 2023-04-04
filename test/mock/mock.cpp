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
#include "test/testing.hpp"

// Testing
#include "test/mocking.hpp"
#include "test/monitoring-types.hpp"

// Under test.
#include "src/mock/mock.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace mock {
namespace {

using namespace std;
using ::Catch::Matches;
using ::mock::matchers::_;
using ::testing::monitoring_types::MovedFromCounter;

/****************************************************************
** Static Checks
*****************************************************************/
using TestResponder =
    detail::Responder<int, tuple<>,
                      decltype( make_index_sequence<0>() )>;
using TestResponderQueue = detail::ResponderQueue<TestResponder>;

static_assert( !is_move_constructible_v<TestResponderQueue> );
static_assert( !is_move_assignable_v<TestResponderQueue> );
static_assert( !is_copy_constructible_v<TestResponderQueue> );
static_assert( !is_copy_assignable_v<TestResponderQueue> );

static_assert(
    is_constructible_v<detail::RetHolder<int&>, int&> );

/****************************************************************
** IPoint
*****************************************************************/
struct IPoint {
  virtual ~IPoint() = default;

  virtual int get_x() const = 0;

  virtual int get_y() const = 0;

  virtual bool get_xy( int* x_out, int& y_out ) const = 0;

  virtual std::string repeat_str( std::string const& s,
                                  int count ) const = 0;

  virtual void set_x( int x ) = 0;

  virtual void set_y( int y ) = 0;

  virtual void set_xy( int x, int y ) = 0;

  virtual double length() const = 0;

  virtual void output_c_array( size_t* size_written,
                               int*    arr ) const = 0;

  virtual int& returns_lvalue_ref() const = 0;

  virtual MovedFromCounter moves_result() const = 0;
};

/****************************************************************
** MockPoint
*****************************************************************/
struct MockPoint : IPoint {
  MockConfig::binder config =
      MockConfig{ .throw_on_unexpected = true };

  MOCK_METHOD( int, get_x, (), ( const ) );
  MOCK_METHOD( int, get_y, (), ( const ) );
  MOCK_METHOD( bool, get_xy, (int*, int&), ( const ) );
  MOCK_METHOD( std::string, repeat_str,
               (std::string const&, int), ( const ) );
  MOCK_METHOD( void, set_x, (int), () );
  MOCK_METHOD( void, set_y, (int), () );
  MOCK_METHOD( void, set_xy, (int, int), () );
  MOCK_METHOD( double, length, (), ( const ) );
  MOCK_METHOD( void, output_c_array, (size_t*, int*),
               ( const ) );
  MOCK_METHOD( int&, returns_lvalue_ref, (), ( const ) );
  MOCK_METHOD( MovedFromCounter, moves_result, (), ( const ) );
};

/****************************************************************
** PointUser
*****************************************************************/
struct PointUser {
  PointUser( IPoint* p ) : p_( p ) { DCHECK( p_ != nullptr ); }

  int increment_y() {
    int new_val = p_->get_y() + 1;
    p_->set_y( new_val );
    return new_val;
  }

  int some_method_1( int new_x ) {
    p_->set_x( new_x );
    increment_y();
    return static_cast<int>( p_->length() );
  }

  int get_x() const { return p_->get_x(); }

  int get_y() const { return p_->get_y(); }

  bool get_xy( int* x_out, int& y_out ) const {
    return p_->get_xy( x_out, y_out );
  }

  std::string repeat_str( std::string const& s,
                          int                count ) const {
    return p_->repeat_str( s, count );
  }

  void set_xy( int x, int y ) { p_->set_xy( x, y ); }

  int output_c_array() const {
    int    buf[10];
    size_t n_written;
    p_->output_c_array( &n_written, buf );
    int res = 0;
    for( size_t i = 0; i < n_written; ++i ) res += buf[i];
    return res;
  }

  int get_lvalue_ref() const { return p_->returns_lvalue_ref(); }

  MovedFromCounter moves_result() const {
    return p_->moves_result();
  }

  IPoint* p_;
};

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[mock] one off calls" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_xy( 3, 4 );
  user.set_xy( 3, 4 );

  mp.EXPECT__get_x().returns( 7 );
  REQUIRE( user.get_x() == 7 );

  mp.EXPECT__get_y().returns( 10 );
  REQUIRE( user.get_y() == 10 );

  mp.EXPECT__get_y().returns( 10 );
  mp.EXPECT__set_y( 11 );
  REQUIRE( user.increment_y() == 11 );
}

TEST_CASE( "[mock] repeated calls" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__get_x().times( 3 ).returns( 7 );
  REQUIRE( user.get_x() == 7 );
  REQUIRE( user.get_x() == 7 );
  REQUIRE( user.get_x() == 7 );
  REQUIRE_THROWS_WITH(
      user.get_x(),
      Matches( "unexpected mock function call.*" ) );

  mp.EXPECT__get_x().times( 1 ).returns( 8 );
  mp.EXPECT__get_x().times( 2 ).returns( 9 );
  REQUIRE( user.get_x() == 8 );
  REQUIRE( user.get_x() == 9 );
  REQUIRE( user.get_x() == 9 );

  REQUIRE_THROWS_WITH(
      user.get_x(),
      Matches( "unexpected mock function call.*" ) );
}

TEST_CASE( "[mock] sets_arg" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__get_xy( _, _ )
      .sets_arg<0>( 5 )
      .sets_arg<1>( 6 )
      .returns( true );
  int x = 0, y = 0;
  REQUIRE( user.get_xy( &x, y ) == true );
  REQUIRE( x == 5 );
  REQUIRE( y == 6 );
}

TEST_CASE( "[mock] sets_arg_array" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__output_c_array( _, _ )
      .sets_arg<0>( 3 )
      .sets_arg_array<1>( vector{ 1, 2, 3 } );
  REQUIRE( user.output_c_array() == 1 + 2 + 3 );
}

TEST_CASE( "[mock] any" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__get_y().returns( 10 );
  mp.EXPECT__set_y( _ );
  REQUIRE( user.increment_y() == 11 );
}

TEST_CASE( "[mock] lvalue ref return type" ) {
  MockPoint mp;
  PointUser user( &mp );

  int n = 4;
  mp.EXPECT__returns_lvalue_ref().returns( n );
  // Make sure that we're really storing a ref.
  n = 5;
  REQUIRE( user.get_lvalue_ref() == 5 );
}

TEST_CASE(
    "[mock] throws on unexpected mock function argument" ) {
  MockPoint mp;
  PointUser user( &mp );

  SECTION( "set_xy" ) {
    mp.EXPECT__set_xy( _, 5 );
    REQUIRE_THROWS_WITH( user.set_xy( 0, 4 ),
                         Matches( ".*unexpected arguments.*" ) );
    // Make the expected call so an error isn't thrown.
    mp.set_xy( 0, 5 );
  }

  SECTION( "repeat_str" ) {
    mp.EXPECT__repeat_str( "hello", 5 ).returns( "none" );
    REQUIRE_THROWS_WITH(
        user.repeat_str( "hellx", 5 ),
        "mock function call with unexpected arguments: "
        "repeat_str( \"hellx\", 5 ); Argument #1 (one-based) "
        "does not match expected value \"hello\"." );
    // Make the expected call so an error isn't thrown.
    mp.repeat_str( "hello", 5 );
  }
}

TEST_CASE( "[mock] throws on unexpected mock call" ) {
  MockPoint mp;
  PointUser user( &mp );

  REQUIRE_THROWS_WITH(
      user.increment_y(),
      Matches( "unexpected mock function call.*get_y.*" ) );
}

TEST_CASE(
    "[mock] prints args on unexpected mock function call" ) {
  MockPoint mp;
  PointUser user( &mp );

  int n = 7;
  int m = 8;
  // The first arg, being a pointer, is not formattable.
  REQUIRE_THROWS_WITH(
      user.get_xy( &m, n ),
      "unexpected mock function call: get_xy( ?, 7 )" );

  // Test that string arguments get quoted.
  REQUIRE_THROWS_WITH( user.repeat_str( "hello", 5 ),
                       "unexpected mock function call: "
                       "repeat_str( \"hello\", 5 )" );
}

TEST_CASE( "[mock] some_method_1" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( 4 );
  mp.EXPECT__get_y().returns( 2 );
  mp.EXPECT__set_y( 3 );
  mp.EXPECT__length().returns( 2.3 );
  REQUIRE( user.some_method_1( 4 ) == 2 );
}

TEST_CASE(
    "[mock] movable return type will move out of the stored "
    "value instead of copying" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__moves_result().returns<MovedFromCounter>();
  MovedFromCounter const c = user.moves_result();
  // Move move into storage, then another into c1.
  REQUIRE( c.last_move_source_val == 1 );
  REQUIRE( c.moved_from == 0 );
}

TEST_CASE(
    "[mock] return type with multiple responses copies stored "
    "value instead of moving" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__moves_result().returns<MovedFromCounter>().times(
      3 );

  // In the below, the last_move_source_val would increase each
  // time if the stored return value were being moved from each
  // time.

  { // 1
    MovedFromCounter const c = user.moves_result();
    REQUIRE( c.last_move_source_val == 1 );
    REQUIRE( c.moved_from == 0 );
  }
  { // 2
    MovedFromCounter const c = user.moves_result();
    REQUIRE( c.last_move_source_val == 1 );
    REQUIRE( c.moved_from == 0 );
  }
  { // 3
    MovedFromCounter const c = user.moves_result();
    REQUIRE( c.last_move_source_val == 1 );
    REQUIRE( c.moved_from == 0 );
  }
}

} // namespace
} // namespace mock
