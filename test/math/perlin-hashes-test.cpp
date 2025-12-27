/****************************************************************
**perlin-hashes-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-26.
*
* Description: Unit tests for the math/perlin-hashes module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/math/perlin-hashes.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

// C++ standard library
#include <map>

namespace math {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[math/perlin-hashes] perlin_hashes" ) {
  PerlinHashes const& hashes = perlin_hashes();
  REQUIRE( hashes.size() == 65536 );
  map<uint16_t, int /*count*/> m;
  for( int i = 0; i < 65536; ++i ) {
    // Not strictly necessary, but it would seem to be nice to
    // have this property if the numbers are properly shuffled.
    // That said, it took a few iterations to find a set that had
    // this property; surprisingly, there were typically 1 or 2
    // violations.
    REQUIRE( hashes[i] != i );
    ++m[hashes[i]];
  }
  REQUIRE( m.size() == 65536 );
  for( uint16_t i = 0; i < 65535; ++i ) {
    INFO( format( "i: {}", i ) );
    REQUIRE( m[i] == 1 );
  }
}

} // namespace
} // namespace math
