/****************************************************************
**pixel.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Unit tests for the src/gfx/pixel.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/pixel.hpp"

// Rcl
#include "rcl/model.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

TEST_CASE( "[pixel] rcl" ) {
  using namespace rcl;

  SECTION( "success no alpha" ) {
    value v{ "#0f12ea" };
    REQUIRE( convert_to<pixel>( v ) ==
             pixel{ 0x0f, 0x12, 0xea, 0xff } );
  }
  SECTION( "success with uppercase" ) {
    value v{ "#0F12Ea" };
    REQUIRE( convert_to<pixel>( v ) ==
             pixel{ 0x0f, 0x12, 0xea, 0xff } );
  }
  SECTION( "success with alpha" ) {
    value v{ "#0f12ea88" };
    REQUIRE( convert_to<pixel>( v ) ==
             pixel{ 0x0f, 0x12, 0xea, 0x88 } );
  }
  SECTION( "failure wrong type" ) {
    value v{ 5 };
    REQUIRE( convert_to<pixel>( v ) ==
             error( "cannot produce a pixel from type int. "
                    "String required." ) );
  }
  SECTION( "failure no hash" ) {
    value v{ "0101011" };
    REQUIRE( convert_to<pixel>( v ) ==
             error( "pixel objects must start with a '#'." ) );
  }
  SECTION( "failure length" ) {
    value v{ "#01011" };
    REQUIRE( convert_to<pixel>( v ) ==
             error( "pixel objects must be of the form "
                    "`#NNNNNN[NN]` with N in 0-f." ) );
  }
  SECTION( "failure invalid" ) {
    value v{ "#01010z" };
    REQUIRE( convert_to<pixel>( v ) ==
             error( "failed to parse color: `01010z'." ) );
  }
}

} // namespace
} // namespace gfx
