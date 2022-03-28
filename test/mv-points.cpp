/****************************************************************
**mv-points.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-17.
*
* Description: Unit tests for the src/mv-points.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/mv-points.hpp"

// cdr
#include "src/cdr/converter.hpp"
#include "src/cdr/ext-builtin.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace ::std;
using namespace ::cdr::literals;

using ::cdr::testing::conv_from_bt;

TEST_CASE( "[mv-points] cdr" ) {
  cdr::converter conv;
  SECTION( "to_canonical" ) {
    MovementPoints p;
    REQUIRE( conv.to( p ) == cdr::table{ "atoms"_key = 0 } );
    p += MovementPoints::_1_3();
    REQUIRE( conv.to( p ) == cdr::table{ "atoms"_key = 1 } );
    p += MovementPoints::_2_3();
    REQUIRE( conv.to( p ) == cdr::table{ "atoms"_key = 3 } );
  }
  SECTION( "from_canonical" ) {
    // From int.
    REQUIRE( conv_from_bt<MovementPoints>( conv, 0 ) ==
             MovementPoints( 0 ) );
    REQUIRE( conv_from_bt<MovementPoints>( conv, 5 ) ==
             MovementPoints( 5 ) );
    // From table.
    REQUIRE( conv_from_bt<MovementPoints>(
                 conv, cdr::table{ "atoms"_key = 0 } ) ==
             MovementPoints( 0 ) );
    MovementPoints frac( 1 );
    frac += MovementPoints::_2_3();
    REQUIRE( conv_from_bt<MovementPoints>(
                 conv, cdr::table{ "atoms"_key = 5 } ) == frac );
  }
}

TEST_CASE( "[mv-points] negative" ) {
  MovementPoints negative1( -3 );
  MovementPoints negative2 = -MovementPoints::_2_3();
  REQUIRE( negative1 + negative2 ==
           MovementPoints( -4 ) + MovementPoints::_1_3() );
  MovementPoints positive = MovementPoints::_1_3();
  REQUIRE( negative1 + positive ==
           MovementPoints( -2 ) - MovementPoints::_2_3() );
}

} // namespace
} // namespace rn
