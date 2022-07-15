/****************************************************************
**function-ref.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-14.
*
* Description: Unit tests for the src/base/function-ref.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/function-ref.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

char foo1( int n ) { return 'c' + char( n % 256 ); }

char foo2( function_ref<char( int )> f ) { return f( 5 ); }

struct callable {
  char operator()( int n ) { return foo1( n ); }
};

struct const_callable {
  char operator()( int n ) const { return foo1( n ); }
};

/****************************************************************
** Static Checks
*****************************************************************/
static_assert(
    !is_default_constructible_v<function_ref<void()>> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/function-ref] function/function*" ) {
  function_ref<char( int )> f1 = foo1;
  function_ref<char( int )> f2 = &foo1;
  function_ref<char( int )> f3 = f1;

  REQUIRE( f1( 1 ) == 'd' );
  REQUIRE( f2( 1 ) == 'd' );
  REQUIRE( f3( 1 ) == 'd' );
}

TEST_CASE( "[base/function-ref] object" ) {
  callable                  c;
  function_ref<char( int )> f1 = c;

  const_callable const            cc;
  function_ref<char( int ) const> f2 = cc;

  REQUIRE( f1( 1 ) == 'd' );
  REQUIRE( f2( 1 ) == 'd' );
}

TEST_CASE( "[base/function-ref] parameter" ) {
  callable c;
  REQUIRE( foo2( c ) == 'h' );

  REQUIRE( foo2( callable{} ) == 'h' );
}

TEST_CASE( "[base/function-ref] lambda" ) {
  // non-mutable no captures.
  auto l1 = []( int n ) -> int { return n + 1; };
  function_ref<int( int )> f1 = l1;

  // non-mutable with captures.
  auto l2 = [&]( int n ) -> int { return l1( n ); };
  function_ref<int( int ) const> f2 = l2;

  // mutable with captures.
  auto l3 = [x = 4]( int n ) mutable -> int {
    ++x;
    return n + x;
  };
  function_ref<int( int )> f3 = l3;

  REQUIRE( f1( 1 ) == 2 );
  REQUIRE( f2( 1 ) == 2 );
  REQUIRE( f3( 1 ) == 6 );
}

TEST_CASE( "[base/function-ref] std::function" ) {
  std::function<char( int )> sf = callable{};

  function_ref<char( int )> f1 = sf;

  REQUIRE( f1( 1 ) == 'd' );
}

} // namespace
} // namespace base
