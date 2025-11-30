/****************************************************************
**text-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-11-29.
*
* Description: Unit tests for the text module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/text.hpp"

// Testing.
#include "test/mocks/render/itextometer.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::size;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[text] rendered_text_size (no crash on empty)" ) {
  rr::MockTextometer textometer;
  rr::TextLayout const text_layout;
  TextReflowInfo const r_info;
  string_view text;
  size expected;

  auto const f = [&] [[clang::noinline]] {
    return rendered_text_size( textometer, text_layout, r_info,
                               text )
        .to_gfx();
  };

  textometer.EXPECT__font_height().returns( 10 );
  text     = "";
  expected = {};
  REQUIRE( f() == expected );
}

} // namespace
} // namespace rn
