/****************************************************************
**query-struct-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-17.
*
* Description: Unit tests for the refl/query-struct module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/query-struct.hpp"

// Testing.
#include "test/rds/testing.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace refl {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[refl/query-struct] alternative_names" ) {
  constexpr array<string_view, 3> const& names =
      alternative_names<rn::MySumtype>();
  static_assert( names[0] == "none" );
  static_assert( names[1] == "some" );
  static_assert( names[2] == "more" );
}

} // namespace
} // namespace refl
