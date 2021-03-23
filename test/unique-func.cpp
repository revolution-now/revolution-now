/****************************************************************
**unique-func.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-03-20.
*
* Description: Unit tests for the src/base/unique-func.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/unique-func.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {
namespace {

using namespace std;

using Catch::Contains;

TEST_CASE( "[unique-func] callable" ) {
  SECTION( "non-const" ) {
    int x = 4;

    unique_func<int( string )> f = [x]( string s ) mutable {
      ++x;
      return int( s.size() ) + x;
    };
    REQUIRE( x == 4 );
    REQUIRE( f( "hello" ) == 10 );
    REQUIRE( x == 4 );
    REQUIRE( f( "hello" ) == 11 );
  }
  SECTION( "const" ) {
    int x = 4;

    unique_func<int( string ) const> func = [&x]( string s ) {
      return int( s.size() ) + x;
    };
    // Test accessing it through a const-ref, which is the point
    // of supporting const callables.
    auto const& f = func;
    REQUIRE( x == 4 );
    REQUIRE( f( "hello" ) == 9 );
    ++x;
    REQUIRE( x == 5 );
    REQUIRE( f( "hello" ) == 10 );
  }
}

struct NonCopyable {
  NonCopyable( int n_ ) : n( n_ ) {}
  NonCopyable( NonCopyable const& ) = delete;
  NonCopyable& operator=( NonCopyable const& ) = delete;

  int n;
};

TEST_CASE( "[unique-func] non-copyable type" ) {
  SECTION( "non-const" ) {
    unique_func<int( NonCopyable const& )> f =
        []( NonCopyable const& nc ) mutable { return nc.n; };
    REQUIRE( f( NonCopyable{ 5 } ) == 5 );
  }
  SECTION( "const" ) {
    unique_func<int( NonCopyable const& ) const> f =
        []( NonCopyable const& nc ) { return nc.n; };
    REQUIRE( f( NonCopyable{ 5 } ) == 5 );
  }
}

TEST_CASE( "[unique-func] non-const move-only callable" ) {
  SECTION( "non-const" ) {
    auto  x     = make_unique<int>( 4 );
    auto* x_ptr = x.get();

    unique_func<int( string )> f =
        [x = std::move( x )]( string s ) mutable {
          ++( *x );
          return int( s.size() ) + *x;
        };
    REQUIRE( *x_ptr == 4 );
    REQUIRE( f( "hello" ) == 10 );
    REQUIRE( *x_ptr == 5 );
    REQUIRE( f( "hello" ) == 11 );
  }
  SECTION( "const" ) {
    auto  x     = make_unique<int>( 4 );
    auto* x_ptr = x.get();

    unique_func<int( string ) const> func =
        [x = std::move( x )]( string s ) {
          return int( s.size() ) + *x;
        };
    // Test accessing it through a const-ref, which is the point
    // of supporting const callables.
    auto const& f = func;
    REQUIRE( *x_ptr == 4 );
    REQUIRE( f( "hello" ) == 9 );
    ++( *x_ptr );
    REQUIRE( *x_ptr == 5 );
    REQUIRE( f( "hello" ) == 10 );
  }
}

} // namespace
} // namespace base
