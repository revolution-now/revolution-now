/****************************************************************
**matchers.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-11-14.
*
* Description: Unit tests for the src/mock/matchers.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/mock/matchers.hpp"

// mock
#include "src/mock/mock.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace mock {
namespace {

using namespace std;
using namespace ::mock::matchers;

using ::Catch::Matches;

/****************************************************************
** IPoint
*****************************************************************/
struct IPoint {
  virtual int get_x() const = 0;

  virtual int get_y() const = 0;

  virtual bool get_xy( int* x_out, int& y_out ) const = 0;

  virtual void set_x( int x ) = 0;

  virtual void set_y( int y ) = 0;

  virtual void set_xy( int x, int y ) = 0;

  virtual void set_x_from_ptr( int* x )                  = 0;
  virtual void set_x_from_const_ptr( int const* x )      = 0;
  virtual void set_x_from_ptr_ptr( int** x )             = 0;
  virtual void set_x_from_const_ptr_ptr( int const** x ) = 0;
  virtual void set_x_from_ptr_const_ptr( int* const* x ) = 0;
  virtual void set_x_from_const_ptr_const_ptr(
      int const* const* x ) = 0;

  virtual void set_x_from_uptr( unique_ptr<int> x ) = 0;
  virtual void set_x_from_uptr_ref(
      unique_ptr<int> const& x ) = 0;
  virtual void set_x_from_const_uptr(
      unique_ptr<int const> x ) = 0;
  virtual void set_x_from_const_uptr_ref(
      unique_ptr<int const> const& x ) = 0;

  virtual double length() const = 0;

  virtual int sum_ints( vector<int> const& v ) const = 0;

  virtual int sum_ints_ptr(
      vector<int const*> const& v ) const = 0;

  virtual int sum_ptr_ints_ptr(
      vector<int const*> const* v ) const = 0;

  virtual int sum_ints_nested(
      vector<vector<unsigned int>> const& v ) const = 0;

  virtual string say_hello( string const& to ) const     = 0;
  virtual string say_hello_ptr( string const* to ) const = 0;
  virtual string say_hello_sv( string_view to ) const    = 0;
  virtual string_view say_hello_sv_sv(
      string_view to ) const = 0;
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

  MOCK_METHOD( void, set_x_from_ptr, (int*), () );
  MOCK_METHOD( void, set_x_from_const_ptr, (int const*), () );
  MOCK_METHOD( void, set_x_from_ptr_ptr, (int**), () );
  MOCK_METHOD( void, set_x_from_const_ptr_ptr, (int const**),
               () );
  MOCK_METHOD( void, set_x_from_ptr_const_ptr, (int* const*),
               () );
  MOCK_METHOD( void, set_x_from_const_ptr_const_ptr,
               (int const* const*), () );

  MOCK_METHOD( void, set_x_from_uptr, (unique_ptr<int>), () );
  MOCK_METHOD( void, set_x_from_uptr_ref,
               (unique_ptr<int> const&), () );
  MOCK_METHOD( void, set_x_from_const_uptr,
               (unique_ptr<int const>), () );
  MOCK_METHOD( void, set_x_from_const_uptr_ref,
               (unique_ptr<int const> const&), () );

  MOCK_METHOD( double, length, (), ( const ) );

  MOCK_METHOD( int, sum_ints, (vector<int> const&), ( const ) );
  MOCK_METHOD( int, sum_ints_ptr, (vector<int const*> const&),
               ( const ) );
  MOCK_METHOD( int, sum_ptr_ints_ptr,
               (vector<int const*> const*), ( const ) );
  MOCK_METHOD( int, sum_ints_nested,
               (vector<vector<unsigned int>> const&),
               ( const ) );

  MOCK_METHOD( string, say_hello, (string const&), ( const ) );
  MOCK_METHOD( string, say_hello_ptr, (string const*),
               ( const ) );
  MOCK_METHOD( string, say_hello_sv, ( string_view ),
               ( const ) );
  MOCK_METHOD( string_view, say_hello_sv_sv, ( string_view ),
               ( const ) );
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
    return p_->length();
  }

  int some_method_2() {
    return some_method_1( 42 ) + p_->get_x();
  }

  void set_x( int x ) { p_->set_x( x ); }

  int get_x() const { return p_->get_x(); }

  int get_y() const { return p_->get_y(); }

  bool get_xy( int* x_out, int& y_out ) const {
    return p_->get_xy( x_out, y_out );
  }

  void set_x_from_ptr( int x ) { p_->set_x_from_ptr( &x ); }
  void set_x_from_const_ptr( int x ) {
    p_->set_x_from_const_ptr( &x );
  }
  void set_x_from_ptr_ptr( int x ) {
    int* y = &x;
    p_->set_x_from_ptr_ptr( &y );
  }
  void set_x_from_const_ptr_ptr( int x ) {
    int const* y = &x;
    p_->set_x_from_const_ptr_ptr( &y );
  }
  void set_x_from_ptr_const_ptr( int x ) {
    int* y = &x;
    p_->set_x_from_ptr_const_ptr( &y );
  }
  void set_x_from_const_ptr_const_ptr( int x ) {
    int* y = &x;
    p_->set_x_from_const_ptr_const_ptr( &y );
  }

  void set_x_from_uptr( int x ) {
    p_->set_x_from_uptr( make_unique<int>( x ) );
  }
  void set_x_from_uptr_ref( int x ) {
    p_->set_x_from_uptr_ref( make_unique<int>( x ) );
  }
  void set_x_from_const_uptr( int x ) {
    p_->set_x_from_const_uptr( make_unique<int>( x ) );
  }
  void set_x_from_const_uptr_ref( int x ) {
    p_->set_x_from_const_uptr_ref( make_unique<int>( x ) );
  }

  int sum_ints( vector<int> const& v ) const {
    return p_->sum_ints( v );
  }

  int sum_ints_ptr( vector<int const*> const& v ) const {
    return p_->sum_ints_ptr( v );
  }

  int sum_ptr_ints_ptr( vector<int const*> const* v ) const {
    return p_->sum_ptr_ints_ptr( v );
  }

  int sum_ints_nested(
      vector<vector<unsigned int>> const& v ) const {
    return p_->sum_ints_nested( v );
  }

  string say_hello( string const& to ) const {
    return p_->say_hello( to );
  }
  string say_hello_ptr( string const* to ) const {
    return p_->say_hello_ptr( to );
  }
  string say_hello_sv( string_view to ) const {
    return p_->say_hello_sv( to );
  }
  string_view say_hello_sv_sv( string_view to ) const {
    return p_->say_hello_sv_sv( to );
  }

  IPoint* p_;
};

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[mock] Pointee" ) {
  MockPoint mp;
  PointUser user( &mp );

  // int*
  EXPECT_CALL( mp, set_x_from_ptr( Pointee( 8 ) ) );
  user.set_x_from_ptr( 8 );

  // int const*
  EXPECT_CALL( mp, set_x_from_const_ptr( Pointee( 8 ) ) );
  user.set_x_from_const_ptr( 8 );

  // int**
  EXPECT_CALL( mp,
               set_x_from_ptr_ptr( Pointee( Pointee( 8 ) ) ) );
  user.set_x_from_ptr_ptr( 8 );

  // int const**
  EXPECT_CALL(
      mp, set_x_from_const_ptr_ptr( Pointee( Pointee( 8 ) ) ) );
  user.set_x_from_const_ptr_ptr( 8 );

  // int* const*
  EXPECT_CALL(
      mp, set_x_from_ptr_const_ptr( Pointee( Pointee( 8 ) ) ) );
  user.set_x_from_ptr_const_ptr( 8 );

  // int const* const*
  EXPECT_CALL( mp, set_x_from_const_ptr_const_ptr(
                       Pointee( Pointee( 8 ) ) ) );
  user.set_x_from_const_ptr_const_ptr( 8 );

  // unique_ptr<int>
  EXPECT_CALL( mp, set_x_from_uptr( Pointee( 8 ) ) );
  user.set_x_from_uptr( 8 );

  // unique_ptr<int> const&
  EXPECT_CALL( mp, set_x_from_uptr_ref( Pointee( 8 ) ) );
  user.set_x_from_uptr_ref( 8 );

  // unique_ptr<int const>
  EXPECT_CALL( mp, set_x_from_const_uptr( Pointee( 8 ) ) );
  user.set_x_from_const_uptr( 8 );

  // unique_ptr<int const> const&
  EXPECT_CALL( mp, set_x_from_const_uptr_ref( Pointee( 8 ) ) );
  user.set_x_from_const_uptr_ref( 8 );
}

TEST_CASE( "[mock] Pointee arg match failure" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, set_x_from_const_ptr( Pointee( 9 ) ) );
  // Wrong one.
  REQUIRE_THROWS_WITH(
      user.set_x_from_const_ptr( 8 ),
      Matches(
          ".*mock function call with unexpected arguments.*" ) );
  // Right one.
  user.set_x_from_const_ptr( 9 );
}

TEST_CASE( "[mock] IterableElementsAre" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, sum_ints( IterableElementsAre( 3, 4, 5 ) ) )
      .returns( 12 );
  vector<int> v1{ 3, 4, 5 };
  REQUIRE( user.sum_ints( v1 ) == 12 );

  int n1 = 3, n2 = 4, n3 = 5;

  EXPECT_CALL( mp,
               sum_ints_ptr( IterableElementsAre(
                   Pointee( 3 ), Pointee( 4 ), Pointee( 5 ) ) ) )
      .returns( 12 );
  vector<int const*> v2{ &n1, &n2, &n3 };
  REQUIRE( user.sum_ints_ptr( v2 ) == 12 );

  EXPECT_CALL(
      mp, sum_ptr_ints_ptr( Pointee( IterableElementsAre(
              Pointee( 3 ), Pointee( 4 ), Pointee( 5 ) ) ) ) )
      .returns( 12 );
  REQUIRE( user.sum_ptr_ints_ptr( &v2 ) == 12 );

  EXPECT_CALL( mp, sum_ints_nested( IterableElementsAre(
                       IterableElementsAre( 1, 2 ),
                       IterableElementsAre( 2, 2 ),
                       IterableElementsAre( 2, 3 ) ) ) )
      .returns( 12 );
  vector<vector<unsigned int>> v3{
      { 1, 2 }, { 2, 2 }, { 2, 3 } };
  REQUIRE( user.sum_ints_nested( v3 ) == 12 );
}

TEST_CASE( "[mock] IterableElementsAre arg match failure" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, sum_ints( IterableElementsAre( 3, 5, 5 ) ) )
      .returns( 12 );
  vector<int> v1{ 3, 4, 5 };
  // Wrong one.
  REQUIRE_THROWS_WITH(
      user.sum_ints( v1 ),
      Matches(
          ".*mock function call with unexpected arguments.*" ) );
  // Right one.
  user.sum_ints( { 3, 5, 5 } );
}

TEST_CASE( "[mock] Ge" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, set_x( Ge( 8 ) ) ).times( 4 );
  user.set_x( 10 );
  user.set_x( 9 );
  user.set_x( 8 );
  REQUIRE_THROWS_WITH(
      user.set_x( 7 ),
      Matches(
          "mock function call with unexpected arguments.*" ) );
  user.set_x( 8 );
}

TEST_CASE( "[mock] Not" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, set_x( Not( Ge( 8 ) ) ) ).times( 4 );
  user.set_x( 5 );
  user.set_x( 6 );
  user.set_x( 7 );
  REQUIRE_THROWS_WITH(
      user.set_x( 8 ),
      Matches(
          "mock function call with unexpected arguments.*" ) );
  user.set_x( 7 );
}

TEST_CASE( "[mock] string" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, say_hello( "bob" ) ).returns( "hello bob" );
  REQUIRE( user.say_hello( "bob" ) == "hello bob" );
}

TEST_CASE( "[mock] string_view" ) {
  MockPoint mp;
  PointUser user( &mp );

  SECTION( "takes string_view" ) {
    // Note that if "bob" were a temporary std::string then this
    // would crash due to stored dangling string_view.
    EXPECT_CALL( mp, say_hello_sv( "bob" ) )
        .returns( "hello bob" );
    REQUIRE( user.say_hello_sv( "bob" ) == "hello bob" );
  }

  SECTION( "takes string_view (use string for return)" ) {
    // Note that if "bob" were a temporary std::string then this
    // would crash due to stored dangling string_view.
    EXPECT_CALL( mp, say_hello_sv( "bob" ) )
        // Passing a temporary string here should be OK because
        // say_hello_sv returns a std::string, so nothing will
        // dangle.
        .returns( string( "hello bob" ) );
    REQUIRE( user.say_hello_sv( "bob" ) == "hello bob" );
  }

  SECTION( "takes and returns string_view" ) {
    // Both of these must be non-dangling.
    EXPECT_CALL( mp, say_hello_sv_sv( "bob" ) )
        .returns( "hello bob" );
    REQUIRE( user.say_hello_sv_sv( "bob" ) == "hello bob" );
  }
}

TEST_CASE( "[mock] StartsWith" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, say_hello( StartsWith( "bob" ) ) )
      .returns( "hello bob" );
  REQUIRE( user.say_hello( "bob bob" ) == "hello bob" );

  EXPECT_CALL( mp,
               say_hello_ptr( Pointee( StartsWith( "bob" ) ) ) )
      .returns( "hello bob" );
  string bobbob = "bob bob";
  REQUIRE( user.say_hello_ptr( &bobbob ) == "hello bob" );
}

TEST_CASE( "[mock] StrContains" ) {
  MockPoint mp;
  PointUser user( &mp );

  EXPECT_CALL( mp, say_hello( StrContains( "b b" ) ) )
      .returns( "hello bob" );
  REQUIRE( user.say_hello( "bob bob" ) == "hello bob" );

  EXPECT_CALL( mp, say_hello( StrContains( "ccc" ) ) )
      .returns( "hello bob" );
  REQUIRE_THROWS_WITH(
      user.say_hello( "bob bob" ),
      Matches(
          "mock function call with unexpected arguments.*" ) );
  REQUIRE( user.say_hello( "bob ccc bob" ) == "hello bob" );
}

} // namespace
} // namespace mock
