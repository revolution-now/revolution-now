/****************************************************************
**rand-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-02.
*
* Description: Unit tests for the rand module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rand.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rand] generate_deterministic_seed" ) {
  Rand rand;

  auto const f = [&] [[clang::noinline]] {
    return rand.generate_deterministic_seed();
  };

  // Should be deterministic because rand by default will not
  // seed the underlying engine and the implementation of the
  // uniform methods used don't include any further platform de-
  // pendent stuff. Thus our marsenne twister engine should give
  // consistent results, even across platforms it is said.

  REQUIRE( base::to_str( f() ) ==
           "d5c31f79e7e1faee22ae9ef6d091bb5c" );

  REQUIRE( base::to_str( f() ) ==
           "3895afe1e9d30005f807b7df2082352c" );
}

} // namespace
} // namespace rn
