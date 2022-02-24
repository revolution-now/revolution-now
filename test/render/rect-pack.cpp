/****************************************************************
**rect-pack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-24.
*
* Description: Unit tests for the src/render/rect-pack.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/rect-pack.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

TEST_CASE( "[render/rect-pack] some test" ) {
  vector<rect_packing> v;
  REQUIRE( pack_rects( v, 10 ) == 0 );
}

} // namespace
} // namespace rr
