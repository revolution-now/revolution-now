/****************************************************************
**co-maybe.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-31.
*
* Description: Unit tests for the src/base/co-maybe.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/co-maybe.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

maybe<int> get_num() { return 110; }
maybe<int> get_den() { return 10; }
maybe<int> get_den0() { return 0; }

maybe<int> divide( int num, int den ) {
  if( den == 0 ) return nothing;
  return num / den;
}

maybe<int> my_coroutine() {
  int num = co_await get_num();
  int den = co_await get_den();
  int res = co_await divide( num, den );
  co_return res;
}

maybe<int> my_nothing( bool with_value ) {
  if( !with_value ) co_await nothing;
  co_return 5;
}

maybe<int> my_coroutine0() {
  int num = co_await get_num();
  int den = co_await get_den0();
  int res = co_await divide( num, den );
  co_return res;
}

maybe<int> my_coro_with_bools( bool b ) {
  int num = co_await get_num();
  co_await b;
  co_return num;
}

maybe<int const&> ref_coro_0() {
  static int const n = 5;
  // FIXME: we don't currently support coroutines with maybe-ref
  // promise types, though we can co_await maybe-refs in general.
  return n;
}

maybe<int> ref_coro_1() {
  decltype( auto ) n = co_await ref_coro_0();
  static_assert( is_same_v<decltype( n ), int const&> );
  co_return n;
}

TEST_CASE( "[co-maybe] simple test" ) {
  auto res = my_coroutine();
  REQUIRE( res.has_value() );
  REQUIRE( *res == 11 );

  auto res0 = my_coroutine0();
  REQUIRE( !res0.has_value() );
}

TEST_CASE( "[co-maybe] test co_await nothing" ) {
  SECTION( "with value" ) {
    auto res = my_nothing( true );
    REQUIRE( res.has_value() );
    REQUIRE( *res == 5 );
  }
  SECTION( "without value" ) {
    auto res = my_nothing( false );
    REQUIRE_FALSE( res.has_value() );
  }
}

TEST_CASE( "[co-maybe] co_awaiting on bools" ) {
  REQUIRE( my_coro_with_bools( true ) == 110 );
  REQUIRE( my_coro_with_bools( false ) == nothing );
}

TEST_CASE( "[co-maybe] co_awaiting on maybe refs" ) {
  maybe<int> const res = ref_coro_1();
  REQUIRE( res == 5 );
}

} // namespace
} // namespace base
