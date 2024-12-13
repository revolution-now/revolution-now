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

} // namespace
} // namespace gfx
