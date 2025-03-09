/****************************************************************
**textometer-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-08.
*
* Description: Unit tests for the render/textometer module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/textometer.hpp"

// render
#include "src/render/ascii-font.hpp"
#include "src/render/atlas.hpp"

// gfx
#include "src/gfx/cartesian.hpp"
#include "src/gfx/pixel.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rr {
namespace {

using namespace std;

using ::base::nothing;
using ::gfx::interval;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

AsciiFont const& ascii_font() {
  static const auto af = [] {
    auto ids = make_unique<array<int, 256>>();
    for( int i = 0; i < 256; ++i ) ( *ids )[i] = i + 100;
    return AsciiFont( std::move( ids ), size{ .w = 4, .h = 6 } );
  }();
  return af;
}

AtlasMap const& atlas_map() {
  static const auto m = [] {
    vector<rect> rects( 356, rect{} );
    vector<rect> trimmed( 356, rect{} );
    for( int i = 100; i < 100 + 256; ++i ) {
      int char_idx = i - 100;
      int row      = char_idx / 16;
      int col      = char_idx % 16;
      rects[i] = rect{ .origin = { .x = col * 4, .y = row * 6 },
                       .size   = { .w = 4, .h = 6 } };
      trimmed[i] = rect{ .origin = { .x = 1, .y = 1 },
                         .size   = { .w = 2, .h = 4 } };
    }
    return AtlasMap( std::move( rects ), std::move( trimmed ) );
  }();
  return m;
}

/****************************************************************
** Harness
*****************************************************************/
struct Harness {
  [[maybe_unused]] Harness()
    : textometer_( atlas_map(), ascii_font() ) {}

  Textometer textometer_;
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE_METHOD( Harness,
                  "[render/textometer] dimensions_for_line" ) {
  TextLayout layout;

  auto const f = [&]( string const& text ) {
    return textometer_.dimensions_for_line( layout, text );
  };

  SECTION( "monospace" ) {
    layout.monospace = true;

    REQUIRE( f( "" ) == size{ .w = 4 * 0, .h = 6 } );
    REQUIRE( f( "h" ) == size{ .w = 4 * 1, .h = 6 } );
    REQUIRE( f( "hello" ) == size{ .w = 4 * 5, .h = 6 } );
    REQUIRE( f( "hello\nhello" ) ==
             size{ .w = 4 * 11, .h = 6 } );
  }

  SECTION( "non-monospace" ) {
    layout.monospace = false;

    REQUIRE( f( "" ) == size{ .w = 0, .h = 6 } );
    REQUIRE( f( "h" ) == size{ .w = 2, .h = 6 } );
    REQUIRE( f( "hello" ) == size{ .w = 2 * 5 + 4, .h = 6 } );
    REQUIRE( f( "hello\nhello" ) ==
             size{ .w = 2 * 11 + 10, .h = 6 } );
  }
}

TEST_CASE_METHOD( Harness,
                  "[render/textometer] spacing_between_chars" ) {
  TextLayout layout;

  auto const f = [&] {
    return textometer_.spacing_between_chars( layout );
  };

  layout = {};
  REQUIRE( f() == 1 );

  layout = { .monospace = true };
  REQUIRE( f() == 0 );

  layout = { .spacing = 1 };
  REQUIRE( f() == 1 );

  layout = { .spacing = 3 };
  REQUIRE( f() == 3 );

  layout = { .monospace = true, .spacing = 3 };
  REQUIRE( f() == 3 );
}

TEST_CASE_METHOD( Harness,
                  "[render/textometer] spacing_between_lines" ) {
  TextLayout layout;

  auto const f = [&] {
    return textometer_.spacing_between_lines( layout );
  };

  layout = {};
  REQUIRE( f() == 1 );

  layout = { .monospace = true };
  REQUIRE( f() == 1 );

  layout = { .line_spacing = 1 };
  REQUIRE( f() == 1 );

  layout = { .line_spacing = 3 };
  REQUIRE( f() == 3 );

  layout = { .monospace = true, .line_spacing = 3 };
  REQUIRE( f() == 3 );
}

TEST_CASE_METHOD( Harness,
                  "[render/textometer] trimmed_horizontally" ) {
  char c = {};
  TextLayout layout;

  auto const f = [&] {
    return textometer_.trimmed_horizontally( layout, c );
  };

  c      = 'A';
  layout = {};
  REQUIRE( f() == interval{ .start = 1, .len = 2 } );

  c      = 'A';
  layout = { .monospace = true };
  REQUIRE( f() == interval{ .start = 0, .len = 4 } );
}

TEST_CASE_METHOD( Harness, "[render/textometer] font_height" ) {
  auto const f = [&] { return textometer_.font_height(); };

  REQUIRE( f() == 6 );
}

} // namespace
} // namespace rr
