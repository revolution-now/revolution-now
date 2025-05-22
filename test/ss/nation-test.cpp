/****************************************************************
**nation-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-21.
*
* Description: Unit tests for the ss/nation module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ss/nation.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ss/nation] european_nation_for" ) {
  REQUIRE( european_nation_for( e_nation::english ) ==
           e_european_nation::english );
  REQUIRE( european_nation_for( e_nation::french ) ==
           e_european_nation::french );
  REQUIRE( european_nation_for( e_nation::spanish ) ==
           e_european_nation::spanish );
  REQUIRE( european_nation_for( e_nation::dutch ) ==
           e_european_nation::dutch );
}

} // namespace
} // namespace rn
