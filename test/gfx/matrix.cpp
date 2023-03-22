/****************************************************************
**matrix.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-22.
*
* Description: Unit tests for the src/gfx/matrix.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/matrix.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/matrix] construction" ) {
  Matrix<int> m( rn::Delta{ .w = 2, .h = 3 } );
  // TODO
  (void)m;
}

} // namespace
} // namespace gfx
