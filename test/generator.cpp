/****************************************************************
**generator.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Unit tests for the src/generator.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/generator.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/*
generator<int> fibonacci() {
  int n1 = 0;
  int n2 = 1;
  co_yield n1;
  co_yield n2;
  for( ;; ) {
    int const next = n1 + n2;
    co_yield next;
    n1 = n2;
    n2 = next;
  }
}
*/

TEST_CASE( "[generator] fibonacci" ) {
  // TODO
}

} // namespace
} // namespace rn
