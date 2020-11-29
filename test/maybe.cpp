/****************************************************************
**maybe.cpp
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
#include "base/maybe.hpp"

// C++ standard library
#include <experimental/type_traits>

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

template<typename T>
using M = ::base::maybe<T>;

TEST_CASE( "[maybe] default construction" ) {
  M<int> m;
  REQUIRE( !m.has_value() );
}

TEST_CASE( "[maybe] T with no default constructor" ) {
  struct A {
    A() = delete;
    A( int m ) : n( m ) {}
    int n;
  };

  M<A> m;
  REQUIRE( !m.has_value() );
  m = A{ 2 };
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 2 );
  A a( 6 );
  m = a;
  REQUIRE( m.has_value() );
  REQUIRE( m->n == 6 );
}

TEST_CASE( "[maybe] value construction" ) {
  //
}

TEST_CASE( "[maybe] converting value construction" ) {
  //
}

TEST_CASE( "[maybe] copy construction" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m3( m );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );

    M<int> m4;
    M<int> m5( m4 );
    REQUIRE( !m5.has_value() );
  }
  SECTION( "string" ) {
    M<string> m{ string( "hello" ) };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m3( m );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );

    M<string> m4;
    M<string> m5( m4 );
    REQUIRE( !m5.has_value() );
  }
}

TEST_CASE( "[maybe] move construction" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( !m.has_value() );

    M<int> m3( M<int>{} );
    REQUIRE( !m3.has_value() );

    M<int> m4( M<int>{ 0 } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == 0 );

    M<int> m5( std::move( m4 ) );
    REQUIRE( !m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == 0 );

    M<int> m6{ std::move( m2 ) };
    REQUIRE( !m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == 4 );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2( std::move( m ) );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( !m.has_value() );

    M<string> m3( M<string>{} );
    REQUIRE( !m3.has_value() );

    M<string> m4( M<string>{ "hellm2" } );
    REQUIRE( m4.has_value() );
    REQUIRE( *m4 == "hellm2" );

    M<string> m5( std::move( m4 ) );
    REQUIRE( !m4.has_value() );
    REQUIRE( m5.has_value() );
    REQUIRE( *m5 == "hellm2" );

    M<string> m6{ std::move( m2 ) };
    REQUIRE( !m2.has_value() );
    REQUIRE( m6.has_value() );
    REQUIRE( *m6 == "hello" );
  }
}

TEST_CASE( "[maybe] copy assignment" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2;
    m2 = m;
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = m2;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2;
    m2 = m;
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m3( "yes" );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "yes" );
    m3 = m2;
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
  }
}

TEST_CASE( "[maybe] move assignment" ) {
  SECTION( "int" ) {
    M<int> m{ 4 };
    REQUIRE( m.has_value() );
    REQUIRE( *m == 4 );

    M<int> m2;
    m2 = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == 4 );
    REQUIRE( !m.has_value() );

    M<int> m3( 5 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 5 );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == 4 );
    REQUIRE( !m2.has_value() );
  }
  SECTION( "string" ) {
    M<string> m{ "hello" };
    REQUIRE( m.has_value() );
    REQUIRE( *m == "hello" );

    M<string> m2;
    m2 = std::move( m );
    REQUIRE( m2.has_value() );
    REQUIRE( *m2 == "hello" );
    REQUIRE( !m.has_value() );

    M<string> m3( "yes" );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "yes" );
    m3 = std::move( m2 );
    REQUIRE( m3.has_value() );
    REQUIRE( *m3 == "hello" );
    REQUIRE( !m2.has_value() );
  }
}

// TEST_CASE( "[maybe] converting assignments" ) {
//   M<int> m = 5;
//   REQUIRE( m.has_value() );
//   REQUIRE( *m == 5 );
//
//   struct A {
//     A() = default;
//     A( int m ) : n( m ) {}
//         operator int() const { return n; }
//     int n = {};
//   };
//
//   A a{ 7 };
//   m = a;
//   REQUIRE( m.has_value() );
//   REQUIRE( *m == 7 );
//
//   m = A{ 9 };
//   REQUIRE( m.has_value() );
//   REQUIRE( *m == 9 );
//
//   M<A> m2;
//   REQUIRE( !m2.has_value() );
//   m2 = A{3};
//
//   m = m2;
//   REQUIRE( m2.has_value() );
//   REQUIRE( *m2 == 9 );
// }

} // namespace
} // namespace base
