/****************************************************************
**optional.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-28.
*
* Description: An alternative to std::optional, used in the RN
*              code base.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "base/optional.hpp"

// C++ standard library
#include <experimental/type_traits>

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename T>
using O = ::base::optional<T>;

TEST_CASE( "[optional] must be destructible" ) {
  struct NonDestructible {
    ~NonDestructible() = delete;
  };
  struct Destructible {};

  static_assert( experimental::is_detected_v<O, int>,
                 "optional should not be constructible when T "
                 "is not destructible." );
  static_assert( experimental::is_detected_v<O, Destructible>,
                 "optional should not be constructible when T "
                 "is not destructible." );
  static_assert(
      !experimental::is_detected_v<O, NonDestructible>,
      "optional should not be constructible when T "
      "is not destructible." );
}

TEST_CASE( "[optional] default construction" ) {
  O<int> o;
  REQUIRE( !o.has_value() );
}

TEST_CASE( "[optional] T with no default constructor" ) {
  struct A {
    A() = delete;
    A( int m ) : n( m ) {}
    int n;
  };

  O<A> o;
  REQUIRE( !o.has_value() );
  o = A{ 2 };
  REQUIRE( o.has_value() );
  REQUIRE( o->n == 2 );
  A a( 6 );
  o = a;
  REQUIRE( o.has_value() );
  REQUIRE( o->n == 6 );
}

TEST_CASE( "[optional] value construction" ) {
  //
}

TEST_CASE( "[optional] converting value construction" ) {
  //
}

TEST_CASE( "[optional] copy construction" ) {
  SECTION( "int" ) {
    O<int> o{ 4 };
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );

    O<int> o2( o );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == 4 );
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );

    O<int> o3( o );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == 4 );
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == 4 );

    O<int> o4;
    O<int> o5( o4 );
    REQUIRE( !o5.has_value() );
  }
  SECTION( "string" ) {
    O<string> o{ string( "hello" ) };
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );

    O<string> o2( o );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == "hello" );
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );

    O<string> o3( o );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == "hello" );
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == "hello" );

    O<string> o4;
    O<string> o5( o4 );
    REQUIRE( !o5.has_value() );
  }
}

TEST_CASE( "[optional] move construction" ) {
  SECTION( "int" ) {
    O<int> o{ 4 };
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );

    O<int> o2( std::move( o ) );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == 4 );
    REQUIRE( !o.has_value() );

    O<int> o3( O<int>{} );
    REQUIRE( !o3.has_value() );

    O<int> o4( O<int>{ 0 } );
    REQUIRE( o4.has_value() );
    REQUIRE( *o4 == 0 );

    O<int> o5( std::move( o4 ) );
    REQUIRE( !o4.has_value() );
    REQUIRE( o5.has_value() );
    REQUIRE( *o5 == 0 );

    O<int> o6{ std::move( o2 ) };
    REQUIRE( !o2.has_value() );
    REQUIRE( o6.has_value() );
    REQUIRE( *o6 == 4 );
  }
  SECTION( "string" ) {
    O<string> o{ "hello" };
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );

    O<string> o2( std::move( o ) );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == "hello" );
    REQUIRE( !o.has_value() );

    O<string> o3( O<string>{} );
    REQUIRE( !o3.has_value() );

    O<string> o4( O<string>{ "hello2" } );
    REQUIRE( o4.has_value() );
    REQUIRE( *o4 == "hello2" );

    O<string> o5( std::move( o4 ) );
    REQUIRE( !o4.has_value() );
    REQUIRE( o5.has_value() );
    REQUIRE( *o5 == "hello2" );

    O<string> o6{ std::move( o2 ) };
    REQUIRE( !o2.has_value() );
    REQUIRE( o6.has_value() );
    REQUIRE( *o6 == "hello" );
  }
}

TEST_CASE( "[optional] copy assignment" ) {
  SECTION( "int" ) {
    O<int> o{ 4 };
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );

    O<int> o2;
    o2 = o;
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == 4 );
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );

    O<int> o3( 5 );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == 5 );
    o3 = o2;
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == 4 );
  }
  SECTION( "string" ) {
    O<string> o{ "hello" };
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );

    O<string> o2;
    o2 = o;
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == "hello" );
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );

    O<string> o3( "yes" );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == "yes" );
    o3 = o2;
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == "hello" );
  }
}

TEST_CASE( "[optional] move assignment" ) {
  SECTION( "int" ) {
    O<int> o{ 4 };
    REQUIRE( o.has_value() );
    REQUIRE( *o == 4 );

    O<int> o2;
    o2 = std::move( o );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == 4 );
    REQUIRE( !o.has_value() );

    O<int> o3( 5 );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == 5 );
    o3 = std::move( o2 );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == 4 );
    REQUIRE( !o2.has_value() );
  }
  SECTION( "string" ) {
    O<string> o{ "hello" };
    REQUIRE( o.has_value() );
    REQUIRE( *o == "hello" );

    O<string> o2;
    o2 = std::move( o );
    REQUIRE( o2.has_value() );
    REQUIRE( *o2 == "hello" );
    REQUIRE( !o.has_value() );

    O<string> o3( "yes" );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == "yes" );
    o3 = std::move( o2 );
    REQUIRE( o3.has_value() );
    REQUIRE( *o3 == "hello" );
    REQUIRE( !o2.has_value() );
  }
}

// TEST_CASE( "[optional] converting assignments" ) {
//   O<int> o = 5;
//   REQUIRE( o.has_value() );
//   REQUIRE( *o == 5 );
//
//   struct A {
//     A() = default;
//     A( int m ) : n( m ) {}
//         operator int() const { return n; }
//     int n = {};
//   };
//
//   A a{ 7 };
//   o = a;
//   REQUIRE( o.has_value() );
//   REQUIRE( *o == 7 );
//
//   o = A{ 9 };
//   REQUIRE( o.has_value() );
//   REQUIRE( *o == 9 );
//
//   O<A> o2;
//   REQUIRE( !o2.has_value() );
//   o2 = A{3};
//
//   o = o2;
//   REQUIRE( o2.has_value() );
//   REQUIRE( *o2 == 9 );
// }

} // namespace
} // namespace base
