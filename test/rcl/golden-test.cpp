/****************************************************************
**golden-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-03-16.
*
* Description: Unit tests for the rcl/golden module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/rcl/golden-impl.rds.hpp"
#include "src/rcl/golden.hpp"

// refl
#include "src/refl/cdr.hpp"
#include "src/refl/to-str.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rcl {
namespace {

using namespace std;

using ::base::valid;

GoldenSampleStruct some_obj() {
  return { .z = 1.3, .w = { .x = 3, .y = 4 } };
}

static_assert( cdr::Canonical<GoldenSampleStruct> );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[rcl/golden] creates golden file" ) {
  GoldenSampleStruct const o = some_obj();

  // Will write to a file called "sample.rcl" in the
  // test/data/golden/golden/ folder.
  Golden const gold( o, "sample" );

  REQUIRE( gold.is_golden() == valid );
}

} // namespace
} // namespace rcl
