/****************************************************************
**validate-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-04.
*
* Description: Unit tests for the refl/validate module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/validate.hpp"

// Testing.
#include "test/rds/testing.rds.hpp"

// refl
#include "src/refl/traverse.hpp"

// traverse
#include "src/traverse/ext-base.hpp"
#include "src/traverse/ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace refl {
namespace {

using namespace std;

using ::base::valid;
using ::base::valid_or;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[refl/validate] validate_recursive" ) {
  rdstest::ValidatableLevel1 top;

  auto const f = [&] [[clang::noinline]] {
    return validate_recursive( top, "top" );
  };

  REQUIRE( f() == "top.o.y: error: failed validation"s );

  top.o.y.x = 5;
  REQUIRE( f() == "top.o.y: error: failed validation"s );

  top.o.y.y = 7;
  REQUIRE( f() == "top: error: failed validation"s );

  top.n = 2;
  REQUIRE( f() == valid );

  top.o.y.v.resize( 3 );
  REQUIRE( f() == "top.o.y.v[0]: error: failed validation"s );

  top.o.y.v[0].b = true;
  REQUIRE( f() == "top.o.y.v[1]: error: failed validation"s );

  top.o.y.v[2].b = true;
  REQUIRE( f() == "top.o.y.v[1]: error: failed validation"s );

  top.o.y.v[1].b = true;
  REQUIRE( f() == valid );

  top.o.y.v[1].m = 5;
  REQUIRE( f() == valid );

  top.o.y.v[1].m = 6;
  REQUIRE( f() == "top.o.y.v[1]: error: failed m validation"s );

  top.o.y.v[1].m = 4;
  REQUIRE( f() == valid );
}

} // namespace
} // namespace refl
