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

// This is now fixed on clang trunk, see:
//
//   https://github.com/llvm/llvm-project/issues/56532
//
// but we will wait to enable it until the change rolls out to
// all of our machines.
#if 0
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

maybe<int> my_coroutine0() {
  int num = co_await get_num();
  int den = co_await get_den0();
  int res = co_await divide( num, den );
  co_return res;
}

TEST_CASE( "[co-maybe] simple test" ) {
  auto res = my_coroutine();
  REQUIRE( res.has_value() );
  REQUIRE( *res == 11 );

  auto res0 = my_coroutine0();
  REQUIRE( !res0.has_value() );
}
#endif

} // namespace
} // namespace base
