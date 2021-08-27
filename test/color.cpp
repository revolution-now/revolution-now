/****************************************************************
**color.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-08-26.
*
* Description: Unit tests for the src/color.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/color.hpp"

// Rcl
#include "rcl/model.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::rcl::error );

namespace rn {
namespace {

using namespace std;

TEST_CASE( "[color] rcl" ) {
  using namespace rcl;

  SECTION( "success no alpha" ) {
    value v{ "#0f12ea" };
    REQUIRE( convert_to<Color>( v ) ==
             Color{ 0x0f, 0x12, 0xea, 0xff } );
  }
  SECTION( "success with uppercase" ) {
    value v{ "#0F12Ea" };
    REQUIRE( convert_to<Color>( v ) ==
             Color{ 0x0f, 0x12, 0xea, 0xff } );
  }
  SECTION( "success with alpha" ) {
    value v{ "#0f12ea88" };
    REQUIRE( convert_to<Color>( v ) ==
             Color{ 0x0f, 0x12, 0xea, 0x88 } );
  }
  SECTION( "failure wrong type" ) {
    value v{ 5 };
    REQUIRE( convert_to<Color>( v ) ==
             error( "cannot produce a Color from type int. "
                    "String required." ) );
  }
  SECTION( "failure no hash" ) {
    value v{ "0101011" };
    REQUIRE( convert_to<Color>( v ) ==
             error( "Color objects must start with a '#'." ) );
  }
  SECTION( "failure length" ) {
    value v{ "#01011" };
    REQUIRE( convert_to<Color>( v ) ==
             error( "Color objects must be of the form "
                    "`#NNNNNN[NN]` with N in 0-f." ) );
  }
  SECTION( "failure invalid" ) {
    value v{ "#01010z" };
    REQUIRE( convert_to<Color>( v ) ==
             error( "failed to parse color: `01010z'." ) );
  }
}

} // namespace
} // namespace rn
