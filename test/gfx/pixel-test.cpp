/****************************************************************
**pixel-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-11-16.
*
* Description: Unit tests for the gfx/pixel module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/pixel.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace gfx {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/pixel] from_hex_rgba" ) {
  pixel expected;
  uint32_t hex = {};

  auto f = [&] { return pixel::from_hex_rgba( hex ); };

  hex      = 0;
  expected = pixel{};
  REQUIRE( f() == expected );

  hex      = 0xff;
  expected = { .r = 0, .g = 0, .b = 0, .a = 0xff };
  REQUIRE( f() == expected );

  hex      = 0x12345678;
  expected = { .r = 0x12, .g = 0x34, .b = 0x56, .a = 0x78 };
  REQUIRE( f() == expected );

  hex      = 0x12345600;
  expected = { .r = 0x12, .g = 0x34, .b = 0x56, .a = 0x00 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[gfx/pixel] from_hex_rgb" ) {
  pixel expected;
  uint32_t hex = {};

  auto f = [&] { return pixel::from_hex_rgb( hex ); };

  hex      = 0;
  expected = pixel{ .a = 0xff };
  REQUIRE( f() == expected );

  hex      = 0xff;
  expected = { .r = 0, .g = 0, .b = 0xff, .a = 0xff };
  REQUIRE( f() == expected );

  hex      = 0x12345678;
  expected = { .r = 0x34, .g = 0x56, .b = 0x78, .a = 0xff };
  REQUIRE( f() == expected );

  hex      = 0x00345678;
  expected = { .r = 0x34, .g = 0x56, .b = 0x78, .a = 0xff };
  REQUIRE( f() == expected );

  hex      = 0xff345678;
  expected = { .r = 0x34, .g = 0x56, .b = 0x78, .a = 0xff };
  REQUIRE( f() == expected );
}

TEST_CASE( "[gfx/pixel] shaded" ) {
  pixel const p1{ .r = 30, .g = 50, .b = 70, .a = 255 };
  pixel const p2{ .r = 60, .g = 100, .b = 140, .a = 100 };

  REQUIRE( p1.shaded( 1 ) != p1.shaded( 2 ) );
  REQUIRE( p1.shaded( 1 ) != p1.shaded( 3 ) );
  REQUIRE( p1.shaded( 2 ) != p1.shaded( 3 ) );

  REQUIRE( p1.shaded( 1 ) ==
           pixel{ .r = 0x1c, .g = 0x2a, .b = 0x3d, .a = 0xff } );
  REQUIRE( p1.shaded( 3 ) ==
           pixel{ .r = 0x18, .g = 0x1f, .b = 0x2e, .a = 255 } );

  // Same again: test that caching doesn't mess things up.
  REQUIRE( p1.shaded( 1 ) ==
           pixel{ .r = 0x1c, .g = 0x2a, .b = 0x3d, .a = 0xff } );
  REQUIRE( p1.shaded( 3 ) ==
           pixel{ .r = 0x18, .g = 0x1f, .b = 0x2e, .a = 255 } );

  REQUIRE( p2.shaded( 1 ) != p2.shaded( 2 ) );
  REQUIRE( p2.shaded( 1 ) != p2.shaded( 3 ) );
  REQUIRE( p2.shaded( 2 ) != p2.shaded( 3 ) );

  REQUIRE( p2.shaded( 1 ) ==
           pixel{ .r = 0x39, .g = 0x55, .b = 0x7a, .a = 0x64 } );
  REQUIRE( p2.shaded( 3 ) ==
           pixel{ .r = 0x32, .g = 0x3f, .b = 0x5d, .a = 0x64 } );

  // Same again: test that caching doesn't mess things up.
  REQUIRE( p2.shaded( 1 ) ==
           pixel{ .r = 0x39, .g = 0x55, .b = 0x7a, .a = 0x64 } );
  REQUIRE( p2.shaded( 3 ) ==
           pixel{ .r = 0x32, .g = 0x3f, .b = 0x5d, .a = 0x64 } );
}

TEST_CASE( "[gfx/pixel] highlighted" ) {
  pixel const p1{ .r = 30, .g = 50, .b = 70, .a = 255 };
  pixel const p2{ .r = 60, .g = 100, .b = 140, .a = 100 };

  REQUIRE( p1.highlighted( 1 ) != p1.highlighted( 2 ) );
  REQUIRE( p1.highlighted( 1 ) != p1.highlighted( 3 ) );
  REQUIRE( p1.highlighted( 2 ) != p1.highlighted( 3 ) );

  REQUIRE( p1.highlighted( 1 ) ==
           pixel{ .r = 0x21, .g = 0x3f, .b = 0x56, .a = 0xff } );
  REQUIRE( p1.highlighted( 3 ) ==
           pixel{ .r = 0x25, .g = 0x5f, .b = 0x79, .a = 255 } );

  // Same again: test that caching doesn't mess things up.
  REQUIRE( p1.highlighted( 1 ) ==
           pixel{ .r = 0x21, .g = 0x3f, .b = 0x56, .a = 0xff } );
  REQUIRE( p1.highlighted( 3 ) ==
           pixel{ .r = 0x25, .g = 0x5f, .b = 0x79, .a = 255 } );

  REQUIRE( p2.highlighted( 1 ) != p2.highlighted( 2 ) );
  REQUIRE( p2.highlighted( 1 ) != p2.highlighted( 3 ) );
  REQUIRE( p2.highlighted( 2 ) != p2.highlighted( 3 ) );

  REQUIRE( p2.highlighted( 1 ) ==
           pixel{ .r = 0x3d, .g = 0x74, .b = 0x9e, .a = 0x64 } );
  REQUIRE( p2.highlighted( 3 ) ==
           pixel{ .r = 0x3e, .g = 0x9c, .b = 0xc3, .a = 0x64 } );

  // Same again: test that caching doesn't mess things up.
  REQUIRE( p2.highlighted( 1 ) ==
           pixel{ .r = 0x3d, .g = 0x74, .b = 0x9e, .a = 0x64 } );
  REQUIRE( p2.highlighted( 3 ) ==
           pixel{ .r = 0x3e, .g = 0x9c, .b = 0xc3, .a = 0x64 } );
}

TEST_CASE( "[gfx/pixel] to_uint32" ) {
  pixel const p{ .r = 0x21, .g = 0x3f, .b = 0x56, .a = 0x64 };
  REQUIRE( p.to_uint32() == 0x213f5664 );
}

TEST_CASE( "[gfx/pixel] traverse" ) {
}

} // namespace
} // namespace gfx
