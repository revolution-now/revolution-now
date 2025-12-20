/****************************************************************
**no-discard-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Unit tests for the base/no-discard module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/no-discard.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[vocab] NoDiscard const implicit conversion" ) {
  NoDiscard<bool> b = true;
  REQUIRE( as_const( b ) );
}

} // namespace
} // namespace base
