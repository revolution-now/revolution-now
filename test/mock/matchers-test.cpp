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

// testing
#include "test/monitoring-types.hpp"

// mock
#include "src/mock/mock.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

#define REQUIRE_UNEXPECTED_ARGS( ... ) \
  REQUIRE_THROWS_WITH(                 \
      __VA_ARGS__, Catch::Contains( "unexpected arguments" ) );

namespace mock {
namespace {

using namespace std;

using namespace ::mock::matchers;
using namespace ::Catch::literals;

using ::testing::monitoring_types::NoCopy;
using ::testing::monitoring_types::Trivial;

struct Foo {
  bool operator==( Foo const& ) const = default;

  int bar       = 6;
  int const baz = 7;

  int get_bar() const { return bar; }
  int const& get_baz() const { return baz; }
};

struct NonEqualityComparable {
  bool operator==( NonEqualityComparable const& ) const = delete;
};

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

  virtual void set_xy_pair(
      std::pair<int /*x*/, int /*y*/> p ) = 0;

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

  virtual double length() const               = 0;
  virtual double double_add( double d ) const = 0;

  virtual int sum_ints( vector<int> const& v ) const = 0;

  virtual int sum_ints_ptr(
      vector<int const*> const& v ) const = 0;

  virtual int sum_ptr_ints_ptr(
      vector<int const*> const* v ) const = 0;

  virtual int sum_ints_nested(
      vector<vector<unsigned int>> const& v ) const = 0;

  virtual string say_hello( string const& to ) const     = 0;
  virtual string say_hello_ptr( string const* to ) const = 0;
  virtual string say_hello_s( string to ) const          = 0;

  virtual void set_foo( Foo const& foo ) = 0;

  virtual void take_bool( bool b ) const = 0;

  virtual void takes_non_eq_comparable(
      NonEqualityComparable const& nc ) const = 0;

  virtual void no_copy_arg_1( NoCopy const& nc ) const  = 0;
  virtual void no_copy_arg_2( Trivial const& nc ) const = 0;
};

/****************************************************************
** MockPoint
*****************************************************************/
struct MockPoint : IPoint {
  virtual ~MockPoint() override = default;
  MockConfig::binder config =
      MockConfig{ .throw_on_unexpected = true };

  MOCK_METHOD( int, get_x, (), ( const ) );
  MOCK_METHOD( int, get_y, (), ( const ) );
  MOCK_METHOD( bool, get_xy, (int*, int&), ( const ) );
  MOCK_METHOD( void, set_x, (int), () );
  MOCK_METHOD( void, set_y, (int), () );
  MOCK_METHOD( void, set_xy, (int, int), () );

  using pair_int_int = std::pair<int, int>;
  MOCK_METHOD( void, set_xy_pair, ( pair_int_int ), () );

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
  MOCK_METHOD( double, double_add, (double), ( const ) );

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
  MOCK_METHOD( string, say_hello_s, ( string ), ( const ) );

  MOCK_METHOD( void, set_foo, (Foo const&), () );

  MOCK_METHOD( void, take_bool, (bool), ( const ) );

  MOCK_METHOD( void, takes_non_eq_comparable,
               (NonEqualityComparable const&), ( const ) );

  MOCK_METHOD( void, no_copy_arg_1, (NoCopy const&), ( const ) );
  MOCK_METHOD( void, no_copy_arg_2, (Trivial const&),
               ( const ) );
};

/****************************************************************
** PointUser
*****************************************************************/
struct PointUser {
  PointUser( IPoint* p ) : p_( p ) { DCHECK( p_ != nullptr ); }

  void set_xy_pair( std::pair<int /*x*/, int /*y*/> p ) {
    p_->set_xy_pair( p );
  }

  void set_x( int x ) { p_->set_x( x ); }

  void set_x_from_ptr( int* x ) { p_->set_x_from_ptr( x ); }
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
  string say_hello_s( string to ) const {
    return p_->say_hello_s( to );
  }

  void set_foo( Foo const& foo ) { p_->set_foo( foo ); }

  void take_bool( bool b ) const { p_->take_bool( b ); }

  void takes_non_eq_comparable(
      NonEqualityComparable const& nc ) const {
    p_->takes_non_eq_comparable( nc );
  }

  double add_two( double d ) const {
    return d + p_->double_add( d + .1 );
  }

  int calls_no_copy_arg_1( NoCopy const& nc ) const {
    p_->no_copy_arg_1( nc );
    return nc.c;
  }

  int calls_no_copy_arg_2( Trivial const& tr ) const {
    p_->no_copy_arg_2( tr );
    return tr.n;
  }

  IPoint* p_;
};

/****************************************************************
** Tests
*****************************************************************/
TEST_CASE( "[mock] use stuff" ) {
  // For some reason if we don't explicitly use this then clang
  // warns that operator== is unused, even though it is needed by
  // the mocking framework since Foo is a parameter to a mocked
  // function.
  Foo foo1;
  Foo foo2;
  REQUIRE( foo1 == foo2 );
}

TEST_CASE( "[mock] Pointee" ) {
  MockPoint mp;
  PointUser user( &mp );

  // int*
  mp.EXPECT__set_x_from_ptr( Pointee( 8 ) );
  int n = 8;
  user.set_x_from_ptr( &n );

  // int const*
  mp.EXPECT__set_x_from_const_ptr( Pointee( 8 ) );
  user.set_x_from_const_ptr( 8 );

  // int**
  mp.EXPECT__set_x_from_ptr_ptr( Pointee( Pointee( 8 ) ) );
  user.set_x_from_ptr_ptr( 8 );

  // int const**

  mp.EXPECT__set_x_from_const_ptr_ptr( Pointee( Pointee( 8 ) ) );
  user.set_x_from_const_ptr_ptr( 8 );

  // int* const*

  mp.EXPECT__set_x_from_ptr_const_ptr( Pointee( Pointee( 8 ) ) );
  user.set_x_from_ptr_const_ptr( 8 );

  // int const* const*
  mp.EXPECT__set_x_from_const_ptr_const_ptr(
      Pointee( Pointee( 8 ) ) );
  user.set_x_from_const_ptr_const_ptr( 8 );

  // unique_ptr<int>
  mp.EXPECT__set_x_from_uptr( Pointee( 8 ) );
  user.set_x_from_uptr( 8 );

  // unique_ptr<int> const&
  mp.EXPECT__set_x_from_uptr_ref( Pointee( 8 ) );
  user.set_x_from_uptr_ref( 8 );

  // unique_ptr<int const>
  mp.EXPECT__set_x_from_const_uptr( Pointee( 8 ) );
  user.set_x_from_const_uptr( 8 );

  // unique_ptr<int const> const&
  mp.EXPECT__set_x_from_const_uptr_ref( Pointee( 8 ) );
  user.set_x_from_const_uptr_ref( 8 );
}

TEST_CASE( "[mock] Pointee arg match failure" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x_from_const_ptr( Pointee( 9 ) );
  // Wrong one.
  REQUIRE_UNEXPECTED_ARGS( user.set_x_from_const_ptr( 8 ) );
  // Right one.
  user.set_x_from_const_ptr( 9 );
}

TEST_CASE( "[mock] IterableElementsAre" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__sum_ints( IterableElementsAre( 3, 4, 5 ) )
      .returns( 12 );
  vector<int> v1{ 3, 4, 5 };
  REQUIRE( user.sum_ints( v1 ) == 12 );

  int n1 = 3, n2 = 4, n3 = 5;

  mp
      .EXPECT__sum_ints_ptr( IterableElementsAre(
          Pointee( 3 ), Pointee( 4 ), Pointee( 5 ) ) )
      .returns( 12 );
  vector<int const*> v2{ &n1, &n2, &n3 };
  REQUIRE( user.sum_ints_ptr( v2 ) == 12 );

  mp
      .EXPECT__sum_ptr_ints_ptr( Pointee( IterableElementsAre(
          Pointee( 3 ), Pointee( 4 ), Pointee( 5 ) ) ) )
      .returns( 12 );
  REQUIRE( user.sum_ptr_ints_ptr( &v2 ) == 12 );

  mp.EXPECT__sum_ints_nested(
        IterableElementsAre( IterableElementsAre( 1, 2 ),
                             IterableElementsAre( 2, 2 ),
                             IterableElementsAre( 2, 3 ) ) )
      .returns( 12 );
  vector<vector<unsigned int>> v3{
    { 1, 2 }, { 2, 2 }, { 2, 3 } };
  REQUIRE( user.sum_ints_nested( v3 ) == 12 );
}

TEST_CASE( "[mock] IterableElementsAre arg match failure" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__sum_ints( IterableElementsAre( 3, 5, 5 ) )
      .returns( 12 );
  vector<int> v1{ 3, 4, 5 };
  // Wrong one.
  REQUIRE_UNEXPECTED_ARGS( user.sum_ints( v1 ) );
  // Right one.
  user.sum_ints( { 3, 5, 5 } );
}

TEST_CASE( "[mock] Ge" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Ge( 8 ) ).times( 4 );
  user.set_x( 10 );
  user.set_x( 9 );
  user.set_x( 8 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 7 ) );
  user.set_x( 8 );
}

TEST_CASE( "[mock] Le" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Le( 8 ) ).times( 4 );
  user.set_x( 6 );
  user.set_x( 7 );
  user.set_x( 8 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 9 ) );
  user.set_x( 8 );
}

TEST_CASE( "[mock] Gt" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Gt( 8 ) ).times( 3 );
  user.set_x( 10 );
  user.set_x( 9 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 8 ) );
  user.set_x( 9 );
}

TEST_CASE( "[mock] Lt" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Lt( 8 ) ).times( 3 );
  user.set_x( 6 );
  user.set_x( 7 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 8 ) );
  user.set_x( 7 );
}

TEST_CASE( "[mock] Ne" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Ne( 8 ) ).times( 3 );
  user.set_x( 7 );
  user.set_x( 9 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 8 ) );
  user.set_x( 7 );
}

TEST_CASE( "[mock] Eq" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Eq( 8 ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 7 ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 9 ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 10 ) );
  user.set_x( 8 );
}

TEST_CASE( "[mock] Not" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( Not( Ge( 8 ) ) ).times( 4 );
  user.set_x( 5 );
  user.set_x( 6 );
  user.set_x( 7 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 8 ) );
  user.set_x( 7 );
}

TEST_CASE( "[mock] string" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__say_hello( "bob" ).returns( "hello bob" );
  REQUIRE( user.say_hello( "bob" ) == "hello bob" );
}

TEST_CASE( "[mock] string_view" ) {
  MockPoint mp;
  PointUser user( &mp );

  SECTION( "takes string_view" ) {
    // Note that if "bob" were a temporary std::string then this
    // would crash due to stored dangling string_view.
    mp.EXPECT__say_hello_s( "bob" ).returns( "hello bob" );
    REQUIRE( user.say_hello_s( "bob" ) == "hello bob" );
  }

  SECTION( "takes string_view (use string for return)" ) {
    // Note that if "bob" were a temporary std::string then this
    // would crash due to stored dangling string_view.
    mp.EXPECT__say_hello_s( "bob" )
        // Passing a temporary string here should be OK because
        // say_hello_s returns a std::string, so nothing will
        // dangle.
        .returns( string( "hello bob" ) );
    REQUIRE( user.say_hello_s( "bob" ) == "hello bob" );
  }
}

TEST_CASE( "[mock] StartsWith" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__say_hello( StartsWith( "bob" ) )
      .returns( "hello bob" );
  REQUIRE( user.say_hello( "bob bob" ) == "hello bob" );

  mp.EXPECT__say_hello_ptr( Pointee( StartsWith( "bob" ) ) )
      .returns( "hello bob" );
  string bobbob = "bob bob";
  REQUIRE( user.say_hello_ptr( &bobbob ) == "hello bob" );
}

TEST_CASE( "[mock] StrContains" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__say_hello( StrContains( "b b" ) )
      .returns( "hello bob" );
  REQUIRE( user.say_hello( "bob bob" ) == "hello bob" );

  mp.EXPECT__say_hello( StrContains( "ccc" ) )
      .returns( "hello bob" );
  REQUIRE_UNEXPECTED_ARGS( user.say_hello( "bob bob" ) );
  REQUIRE( user.say_hello( "bob ccc bob" ) == "hello bob" );
}

TEST_CASE( "[mock] Matches" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__say_hello( Matches( "bob b.b"s ) )
      .returns( "hello bob" );
  REQUIRE( user.say_hello( "bob bob" ) == "hello bob" );

  mp.EXPECT__say_hello( Matches( "h.*c" ) )
      .returns( "hello bob" );
  REQUIRE_UNEXPECTED_ARGS( user.say_hello( "bob bob" ) );
  REQUIRE( user.say_hello( "hob ccc" ) == "hello bob" );
}

TEST_CASE( "[mock] Empty" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__sum_ints( Not( Empty() ) ).returns( 12 );
  vector<int> v{ 3, 4, 5 };
  REQUIRE( user.sum_ints( v ) == 12 );

  mp.EXPECT__sum_ints( Empty() ).returns( 0 );
  v.clear();
  REQUIRE( user.sum_ints( v ) == 0 );
}

TEST_CASE( "[mock] True" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__take_bool( True() );
  REQUIRE_UNEXPECTED_ARGS( user.take_bool( false ) );
  user.take_bool( true );
}

TEST_CASE( "[mock] False" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__take_bool( False() );
  REQUIRE_UNEXPECTED_ARGS( user.take_bool( true ) );
  user.take_bool( false );
}

// This tests that we can match an argument that is not
// equality-comparable so long as we use the Any matcher.
TEST_CASE( "[mock] non-eq-comparable" ) {
  MockPoint mp;
  PointUser user( &mp );

  // The commented line below would fail compilation.
  // EXPECT__takes_non_eq_comparable( NonEqualityComparable{} );
  mp.EXPECT__takes_non_eq_comparable( _ );
  user.takes_non_eq_comparable( NonEqualityComparable{} );
  mp.EXPECT__takes_non_eq_comparable( Any() );
  user.takes_non_eq_comparable( NonEqualityComparable{} );
}

TEST_CASE( "[mock] Null" ) {
  MockPoint mp;
  PointUser user( &mp );

  int n = 0;
  mp.EXPECT__set_x_from_ptr( Null() );
  REQUIRE_UNEXPECTED_ARGS( user.set_x_from_ptr( &n ) );
  user.set_x_from_ptr( nullptr );
}

TEST_CASE( "[mock] HasSize" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__sum_ints( HasSize( 3 ) ).returns( 12 );
  vector<int> v{ 3, 4, 5 };
  REQUIRE( user.sum_ints( v ) == 12 );

  mp.EXPECT__sum_ints( HasSize( Ge( 2 ) ) )
      .times( 3 )
      .returns( 42 );
  REQUIRE( user.sum_ints( v ) == 42 ); // 3, 4, 5
  v.pop_back();
  REQUIRE( user.sum_ints( v ) == 42 ); // 3, 4
  v.pop_back();
  REQUIRE_UNEXPECTED_ARGS( user.sum_ints( v ) ); // 3
  v.push_back( 1 );
  REQUIRE( user.sum_ints( v ) == 42 ); // 3, 1

  v = { 3, 4, 5 };
  mp.EXPECT__sum_ints( Not( HasSize( Ge( 2 ) ) ) ).returns( 42 );
  REQUIRE_UNEXPECTED_ARGS( user.sum_ints( v ) ); // 3, 4, 5
  v.pop_back();
  REQUIRE_UNEXPECTED_ARGS( user.sum_ints( v ) ); // 3, 4
  v.pop_back();
  REQUIRE( user.sum_ints( v ) == 42 ); // 3
}

TEST_CASE( "[mock] Each" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__sum_ints( Each( 5 ) ).returns( 12 );
  vector<int> v{ 5, 5, 5 };
  REQUIRE( user.sum_ints( v ) == 12 );

  mp.EXPECT__sum_ints( Each( Ge( 6 ) ) ).returns( 12 );
  v = { 5, 6, 7 };
  REQUIRE_UNEXPECTED_ARGS( user.sum_ints( v ) );

  v = { 6, 7, 8 };
  REQUIRE( user.sum_ints( v ) == 12 );
}

TEST_CASE( "[mock] AllOf" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( AllOf( Ge( 5 ), Not( Ge( 6 ) ) ) );
  int n = 5;
  user.set_x( n );

  // Explicit value as matcher.
  mp.EXPECT__set_x( AllOf( 5 ) );
  n = 5;
  user.set_x( n );

  mp.EXPECT__set_x( AllOf( Ge( 5 ), Not( Ge( 6 ) ) ) );
  n = 6;
  REQUIRE_UNEXPECTED_ARGS( user.set_x( n ) );
  n = 4;
  REQUIRE_UNEXPECTED_ARGS( user.set_x( n ) );
  n = 5;
  user.set_x( n );

  // Empty list of matchers should always succeed.
  mp.EXPECT__set_x( AllOf() );
  user.set_x( n );
}

TEST_CASE( "[mock] AnyOf" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_x( AnyOf( Ge( 5 ), Not( Ge( 3 ) ) ) )
      .times( 5 );
  user.set_x( 6 );
  user.set_x( 5 );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 4 ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 3 ) );
  user.set_x( 2 );
  user.set_x( 1 );
  user.set_x( 0 );

  // Empty list of matchers should fail.
  auto& responder = mp.EXPECT__set_x( AnyOf() );
  REQUIRE_UNEXPECTED_ARGS( user.set_x( 1 ) );
  // Since nothing can satisfy the above matcher, we must clear
  // the expectations.
  responder.clear_expectations();
}

TEST_CASE( "[mock] TupleElement" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_xy_pair( AllOf( TupleElement<0>( Ge( 5 ) ),
                                 TupleElement<1>( Ge( 3 ) ) ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_xy_pair( { 5, 2 } ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_xy_pair( { 4, 3 } ) );
  user.set_xy_pair( { 5, 3 } );
}

TEST_CASE( "[mock] Key" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_xy_pair(
      AllOf( Key( Ge( 5 ) ), TupleElement<1>( Ge( 3 ) ) ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_xy_pair( { 5, 2 } ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_xy_pair( { 4, 3 } ) );
  user.set_xy_pair( { 5, 3 } );
}

TEST_CASE( "[mock] Field" ) {
  MockPoint mp;
  PointUser user( &mp );

  auto matcher = AllOf(      //
      Field( &Foo::bar, 5 ), //
      Field( &Foo::baz, 6 )  //
  );
  mp.EXPECT__set_foo( matcher );
  user.set_foo( Foo{ 5, 6 } );

  auto matcher2 = AllOf(           //
      Field( &Foo::bar, Ge( 7 ) ), //
      Field( &Foo::baz, 6 )        //
  );
  mp.EXPECT__set_foo( matcher2 );
  REQUIRE_UNEXPECTED_ARGS( user.set_foo( Foo{ 5, 6 } ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_foo( Foo{ 6, 6 } ) );
  user.set_foo( Foo{ 7, 6 } );
}

TEST_CASE( "[mock] Property" ) {
  MockPoint mp;
  PointUser user( &mp );

  auto matcher = AllOf(             //
      Property( &Foo::get_bar, 5 ), //
      Property( &Foo::get_baz, 6 )  //
  );
  mp.EXPECT__set_foo( matcher );
  user.set_foo( Foo{ 5, 6 } );

  auto matcher2 = AllOf(                  //
      Property( &Foo::get_bar, Ge( 7 ) ), //
      Property( &Foo::get_baz, 6 )        //
  );
  mp.EXPECT__set_foo( matcher2 );
  REQUIRE_UNEXPECTED_ARGS( user.set_foo( Foo{ 5, 6 } ) );
  REQUIRE_UNEXPECTED_ARGS( user.set_foo( Foo{ 6, 6 } ) );
  user.set_foo( Foo{ 7, 6 } );
}

TEST_CASE( "[mock] Approx" ) {
  MockPoint mp;
  PointUser user( &mp );
  mp.EXPECT__double_add( Approx( .6, .01 ) ).returns( .7 );
  REQUIRE( user.add_two( .499 ) == 1.199_a );
}

TEST_CASE( "[mock] Approxf" ) {
  MockPoint mp;
  PointUser user( &mp );
  mp.EXPECT__double_add( Approxf( .6, .01 ) ).returns( .7 );
  REQUIRE( user.add_two( .499 ) == 1.199_a );
}

TEST_CASE( "[mock] matcher stringification" ) {
  MockPoint mp;
  PointUser user( &mp );

  mp.EXPECT__set_xy_pair( AllOf( TupleElement<0>( Ge( 5 ) ),
                                 TupleElement<1>( Ge( 3 ) ) ) );
  REQUIRE_THROWS_WITH(
      user.set_xy_pair( { 5, 2 } ),
      "mock function `set_xy_pair` called with unexpected "
      "arguments:\n"
      "argument #1 (one-based) does not match.\n"
      "actual call: set_xy_pair( (5,2) )\n"
      "expected arg #1: AllOf( (TupleElement( (0,Ge( 5 )) ), "
      "TupleElement( (1,Ge( 3 )) )) )" );

  user.set_xy_pair( { 5, 3 } );
}

// This test is to verify that we can use Eq(std::ref(x)) to do
// simple equality matching in a way that prevents the expecta-
// tion holder from taking a copy of the argument which it would
// do if we used just x or even std::ref(x). Note that we cannot
// use Eq(x) since matchers such as Eq require rvalues.
TEST_CASE( "[mock] Eq-ref trick" ) {
  MockPoint mp;
  PointUser user( &mp );

  SECTION( "Can't copy" ) {
    NoCopy nc( 8 );
    REQUIRE( nc == nc );

    mp.EXPECT__no_copy_arg_1( Eq( ref( nc ) ) );
    // If nc had been copied then this would cause a failure when
    // matching the arg.
    ++nc.c;
    REQUIRE( user.calls_no_copy_arg_1( nc ) == 9 );
  }

  SECTION( "Can copy and does scenario" ) {
    Trivial tr;
    tr.n = 8;

    // This will still copy t even though we are using a ref, be-
    // cause the framework will treat this as a value (not a
    // matcher) and so the MatcherWrapper that holds the expecta-
    // tion will hold a Value matcher which itself holds a copy
    // of the object, and a copy of a ref into T still copies the
    // object.
    mp.EXPECT__no_copy_arg_2( ref( tr ) );
    ++tr.n;
    REQUIRE_THROWS_WITH(
        user.calls_no_copy_arg_2( tr ),
        "mock function `no_copy_arg_2` called with unexpected "
        "arguments:\n"
        "argument #1 (one-based) does not match.\n"
        "actual call: no_copy_arg_2( Trivial{d=0,n=9} )\n"
        "expected arg #1: Trivial{d=0,n=8}" );
    // Satisfy the expecation so the test doesn't fail.
    --tr.n;
    user.calls_no_copy_arg_2( tr );
  }

  SECTION( "Can copy but doesn't" ) {
    Trivial tr;
    tr.n = 8;

    mp.EXPECT__no_copy_arg_2( Eq( ref( tr ) ) );
    // If nc had been copied then this would cause a failure when
    // matching the arg.
    ++tr.n;
    REQUIRE( user.calls_no_copy_arg_2( tr ) == 9 );
  }
}

} // namespace
} // namespace mock
