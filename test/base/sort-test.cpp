/****************************************************************
**sort-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-20.
*
* Description: Unit tests for the base/sort module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/sort.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

// C++ standard library.
#include <unordered_set>

namespace base {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[base/sort] some test" ) {
  unordered_set<string> input;
  vector<string>        expected;

  auto f = [&] { return sorted( input ); };

  input    = { "world", "hello", "this", "is", "a", "test" };
  expected = { "a", "hello", "is", "test", "this", "world" };
  REQUIRE( f() == expected );
}

} // namespace
} // namespace base
