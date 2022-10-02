/****************************************************************
**flag.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-02.
*
* Description: Unit tests for the src/base/flag.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/base/flag.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace base {
namespace {

using namespace std;

TEST_CASE( "[base/flag] one_way_flag" ) {
  one_way_flag flag;
  REQUIRE( !flag );
  REQUIRE( !flag );
  REQUIRE( flag.set() == true );
  REQUIRE( !!flag );
  REQUIRE( flag.set() == false );
  REQUIRE( !!flag );
  REQUIRE( flag.set() == false );
  REQUIRE( !!flag );
}

} // namespace
} // namespace base
