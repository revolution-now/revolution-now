/****************************************************************
**typer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-11.
*
* Description: Unit tests for the src/render/typer.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/typer.hpp"

// render
#include "ascii-font.hpp"
#include "atlas.hpp"
#include "emitter.hpp"
#include "painter.hpp"
#include "vertex.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::nothing;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

constexpr pixel B{ .r = 0, .g = 0, .b = 255, .a = 10 };

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
      trimmed[i] = rect{ .size = { .w = 3, .h = 5 } };
    }
    return AtlasMap( std::move( rects ), std::move( trimmed ) );
  }();
  return m;
}

TEST_CASE( "[render/typer] write_char/monospace" ) {
  vector<GenericVertex> v, expected;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );
  typer.layout().monospace = true;

  auto Vert = [&]( point p, point atlas_p, rect atlas_rect ) {
    auto vert = SpriteVertex( p, atlas_p, atlas_rect );
    vert.set_fixed_color( B );
    return vert.generic();
  };

  typer.write( "ha {}\nYes", "bob" );
  REQUIRE( v.size() == 9 * 6 );
  REQUIRE( typer.position() ==
           point{ .x = 20 + 12, .y = 30 + 6 + 1 } );
  REQUIRE( typer.color() == B );

  // clang-format off
  expected = {
      // (20, 30): h
      Vert( { .x = 20+4*0, .y = 30 }, { .x = 8*4,   .y = 6*6   }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 36 }, { .x = 8*4,   .y = 6*6+6 }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 36 }, { .x = 8*4+4, .y = 6*6+6 }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 30 }, { .x = 8*4,   .y = 6*6   }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 30 }, { .x = 8*4+4, .y = 6*6   }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 36 }, { .x = 8*4+4, .y = 6*6+6 }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (24, 30): a
      Vert( { .x = 20+4*1, .y = 30 }, { .x = 1*4,   .y = 6*6   }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 36 }, { .x = 1*4,   .y = 6*6+6 }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 36 }, { .x = 1*4+4, .y = 6*6+6 }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 30 }, { .x = 1*4,   .y = 6*6   }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 30 }, { .x = 1*4+4, .y = 6*6   }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 36 }, { .x = 1*4+4, .y = 6*6+6 }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (28, 30): ' '
      Vert( { .x = 20+4*2, .y = 30 }, { .x = 0*4,   .y = 2*6   }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 36 }, { .x = 0*4,   .y = 2*6+6 }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 36 }, { .x = 0*4+4, .y = 2*6+6 }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 30 }, { .x = 0*4,   .y = 2*6   }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 30 }, { .x = 0*4+4, .y = 2*6   }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 36 }, { .x = 0*4+4, .y = 2*6+6 }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      // (32, 30): b
      Vert( { .x = 20+4*3, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*3, .y = 36 }, { .x = 2*4,   .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*3, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*3, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*3, .y = 30 }, { .x = 2*4+4, .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*3, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (36, 30): o
      Vert( { .x = 20+4*4, .y = 30 }, { .x = 15*4,   .y = 6*6   }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*4, .y = 36 }, { .x = 15*4,   .y = 6*6+6 }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*4, .y = 36 }, { .x = 15*4+4, .y = 6*6+6 }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*4, .y = 30 }, { .x = 15*4,   .y = 6*6   }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*4, .y = 30 }, { .x = 15*4+4, .y = 6*6   }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*4, .y = 36 }, { .x = 15*4+4, .y = 6*6+6 }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (40, 30): b
      Vert( { .x = 20+4*5, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*5, .y = 36 }, { .x = 2*4,   .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*5, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*5, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*5, .y = 30 }, { .x = 2*4+4, .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*5, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // The ones below have a +1 added to y because the Typer
      // will leave one pixel of space between the rows after a
      // new line.
      // (20, 36): Y
      Vert( { .x = 20+4*0, .y = 36 + 1 }, { .x = 9*4,   .y = 5*6   }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 42 + 1 }, { .x = 9*4,   .y = 5*6+6 }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 42 + 1 }, { .x = 9*4+4, .y = 5*6+6 }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 36 + 1 }, { .x = 9*4,   .y = 5*6   }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 36 + 1 }, { .x = 9*4+4, .y = 5*6   }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 42 + 1 }, { .x = 9*4+4, .y = 5*6+6 }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      // (24, 36): e
      Vert( { .x = 20+4*1, .y = 36 + 1 }, { .x = 5*4,   .y = 6*6   }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 42 + 1 }, { .x = 5*4,   .y = 6*6+6 }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 42 + 1 }, { .x = 5*4+4, .y = 6*6+6 }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 36 + 1 }, { .x = 5*4,   .y = 6*6   }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 36 + 1 }, { .x = 5*4+4, .y = 6*6   }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 42 + 1 }, { .x = 5*4+4, .y = 6*6+6 }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (24, 36): s
      Vert( { .x = 20+4*2, .y = 36 + 1 }, { .x = 3*4,   .y = 7*6   }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 42 + 1 }, { .x = 3*4,   .y = 7*6+6 }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 42 + 1 }, { .x = 3*4+4, .y = 7*6+6 }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 36 + 1 }, { .x = 3*4,   .y = 7*6   }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 36 + 1 }, { .x = 3*4+4, .y = 7*6   }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 42 + 1 }, { .x = 3*4+4, .y = 7*6+6 }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
  };
  // clang-format on
  REQUIRE( v == expected );
}

TEST_CASE( "[render/typer] write_char/non-monospace" ) {
  vector<GenericVertex> v, expected;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );
  typer.layout().monospace = false;

  auto Vert = [&]( point p, point atlas_p, rect atlas_rect ) {
    auto vert = SpriteVertex( p, atlas_p, atlas_rect );
    vert.set_fixed_color( B );
    return vert.generic();
  };

  typer.write( "ha {}\nYes", "bob" );
  REQUIRE( v.size() == 9 * 6 );
  REQUIRE( typer.position() ==
           point{ .x = 20 + 9 + 3, .y = 30 + 6 + 1 } );
  REQUIRE( typer.color() == B );

  // clang-format off
  expected = {
      // (20, 30): h
      Vert( { .x = 20+4*0, .y = 30 }, { .x = 8*4,   .y = 6*6   }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 36 }, { .x = 8*4,   .y = 6*6+6 }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 36 }, { .x = 8*4+4, .y = 6*6+6 }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 30 }, { .x = 8*4,   .y = 6*6   }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 30 }, { .x = 8*4+4, .y = 6*6   }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 36 }, { .x = 8*4+4, .y = 6*6+6 }, { .origin={ .x = 8*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (24, 30): a
      Vert( { .x = 20+4*1, .y = 30 }, { .x = 1*4,   .y = 6*6   }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 36 }, { .x = 1*4,   .y = 6*6+6 }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 36 }, { .x = 1*4+4, .y = 6*6+6 }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 30 }, { .x = 1*4,   .y = 6*6   }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 30 }, { .x = 1*4+4, .y = 6*6   }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 36 }, { .x = 1*4+4, .y = 6*6+6 }, { .origin={ .x = 1*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (28, 30): ' '
      Vert( { .x = 20+4*2, .y = 30 }, { .x = 0*4,   .y = 2*6   }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 36 }, { .x = 0*4,   .y = 2*6+6 }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 36 }, { .x = 0*4+4, .y = 2*6+6 }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 30 }, { .x = 0*4,   .y = 2*6   }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 30 }, { .x = 0*4+4, .y = 2*6   }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 36 }, { .x = 0*4+4, .y = 2*6+6 }, { .origin={ .x = 0*4,  .y = 2*6 }, .size={ .w=4, .h=6 } } ),
      // (31, 30): b
      // The (previous) space occupies slightly less than the full char width.
      Vert( { .x = 20+4*3-1, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*3-1, .y = 36 }, { .x = 2*4,   .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*3-1, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*3-1, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*3-1, .y = 30 }, { .x = 2*4+4, .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*3-1, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (35, 30): o
      Vert( { .x = 20+4*4-1, .y = 30 }, { .x = 15*4,   .y = 6*6   }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*4-1, .y = 36 }, { .x = 15*4,   .y = 6*6+6 }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*4-1, .y = 36 }, { .x = 15*4+4, .y = 6*6+6 }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*4-1, .y = 30 }, { .x = 15*4,   .y = 6*6   }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*4-1, .y = 30 }, { .x = 15*4+4, .y = 6*6   }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*4-1, .y = 36 }, { .x = 15*4+4, .y = 6*6+6 }, { .origin={ .x = 15*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (39, 30): b
      Vert( { .x = 20+4*5-1, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*5-1, .y = 36 }, { .x = 2*4,   .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*5-1, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*5-1, .y = 30 }, { .x = 2*4,   .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*5-1, .y = 30 }, { .x = 2*4+4, .y = 6*6   }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*5-1, .y = 36 }, { .x = 2*4+4, .y = 6*6+6 }, { .origin={ .x = 2*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // The ones below have a +1 added to y because the Typer
      // will leave one pixel of space between the rows after a
      // new line.
      // (20, 36): Y
      Vert( { .x = 20+4*0, .y = 36 + 1 }, { .x = 9*4,   .y = 5*6   }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 42 + 1 }, { .x = 9*4,   .y = 5*6+6 }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 42 + 1 }, { .x = 9*4+4, .y = 5*6+6 }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*0, .y = 36 + 1 }, { .x = 9*4,   .y = 5*6   }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 36 + 1 }, { .x = 9*4+4, .y = 5*6   }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*0, .y = 42 + 1 }, { .x = 9*4+4, .y = 5*6+6 }, { .origin={ .x = 9*4,  .y = 5*6 }, .size={ .w=4, .h=6 } } ),
      // (24, 36): e
      Vert( { .x = 20+4*1, .y = 36 + 1 }, { .x = 5*4,   .y = 6*6   }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 42 + 1 }, { .x = 5*4,   .y = 6*6+6 }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 42 + 1 }, { .x = 5*4+4, .y = 6*6+6 }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*1, .y = 36 + 1 }, { .x = 5*4,   .y = 6*6   }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 36 + 1 }, { .x = 5*4+4, .y = 6*6   }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*1, .y = 42 + 1 }, { .x = 5*4+4, .y = 6*6+6 }, { .origin={ .x = 5*4,  .y = 6*6 }, .size={ .w=4, .h=6 } } ),
      // (28, 36): s
      Vert( { .x = 20+4*2, .y = 36 + 1 }, { .x = 3*4,   .y = 7*6   }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 42 + 1 }, { .x = 3*4,   .y = 7*6+6 }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 42 + 1 }, { .x = 3*4+4, .y = 7*6+6 }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 20+4*2, .y = 36 + 1 }, { .x = 3*4,   .y = 7*6   }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 36 + 1 }, { .x = 3*4+4, .y = 7*6   }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
      Vert( { .x = 24+4*2, .y = 42 + 1 }, { .x = 3*4+4, .y = 7*6+6 }, { .origin={ .x = 3*4,  .y = 7*6 }, .size={ .w=4, .h=6 } } ),
  };
  // clang-format on
  REQUIRE( v == expected );
}

TEST_CASE( "[render/typer] default layout" ) {
  vector<GenericVertex> v, expected;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );

  REQUIRE( typer.layout() == TextLayout{} );

  REQUIRE_FALSE( typer.layout().monospace );
  REQUIRE( typer.layout().spacing == nothing );
  REQUIRE( typer.layout().line_spacing == nothing );
}

TEST_CASE( "[render/typer] dimensions_for_line/monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );
  typer.layout().monospace = true;

  REQUIRE( typer.dimensions_for_line( "" ) ==
           size{ .w = 4 * 0, .h = 6 } );
  REQUIRE( typer.dimensions_for_line( "h" ) ==
           size{ .w = 4 * 1, .h = 6 } );
  REQUIRE( typer.dimensions_for_line( "hello" ) ==
           size{ .w = 4 * 5, .h = 6 } );
  REQUIRE( typer.dimensions_for_line( "hello\nhello" ) ==
           size{ .w = 4 * 11, .h = 6 } );
}

TEST_CASE( "[render/typer] dimensions_for_line/non-monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );
  typer.layout().monospace = false;

  REQUIRE( typer.dimensions_for_line( "" ) ==
           size{ .w = 0, .h = 6 } );
  REQUIRE( typer.dimensions_for_line( "h" ) ==
           size{ .w = 3, .h = 6 } );
  REQUIRE( typer.dimensions_for_line( "hello" ) ==
           size{ .w = 3 * 5 + 4, .h = 6 } );
  REQUIRE( typer.dimensions_for_line( "hello\nhello" ) ==
           size{ .w = 3 * 11 + 10, .h = 6 } );
}

TEST_CASE( "[render/typer] spacing/monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  SECTION( "default spacing" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace = true;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 5 * 4, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );
  }

  SECTION( "spacing=2" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace = true;
    typer.layout().spacing   = 2;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 5 * 4 + 2 * 5, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );
  }
}

TEST_CASE( "[render/typer] spacing/non-monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  SECTION( "default spacing" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace = false;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 5 * 3 + 1 * 5, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );
  }

  SECTION( "spacing=2" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace = false;
    typer.layout().spacing   = 2;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 5 * 3 + 2 * 5, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );
  }
}

TEST_CASE( "[render/typer] line spacing/monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  SECTION( "default line spacing" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace = true;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello\nhello2" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 6 * 4, .y = 30 + 6 + 1 } );
    REQUIRE( typer.line_start() ==
             point{ .x = 20, .y = 30 + 6 + 1 } );
  }

  SECTION( "line spacing=2" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace    = true;
    typer.layout().line_spacing = 2;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello\nhello2" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 6 * 4, .y = 30 + 6 + 2 } );
    REQUIRE( typer.line_start() ==
             point{ .x = 20, .y = 30 + 6 + 2 } );
  }
}

TEST_CASE( "[render/typer] line spacing/non-monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  SECTION( "default line spacing" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace = false;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello\nhello2" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 6 * 3 + 6, .y = 30 + 6 + 1 } );
    REQUIRE( typer.line_start() ==
             point{ .x = 20, .y = 30 + 6 + 1 } );
  }

  SECTION( "line spacing=2" ) {
    Typer typer( painter, ascii_font(), TextLayout{},
                 { .x = 20, .y = 30 }, B );
    typer.layout().monospace    = false;
    typer.layout().line_spacing = 2;

    REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
    REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

    typer.write( "hello\nhello2" );
    REQUIRE( typer.position() ==
             point{ .x = 20 + 6 * 3 + 6, .y = 30 + 6 + 2 } );
    REQUIRE( typer.line_start() ==
             point{ .x = 20, .y = 30 + 6 + 2 } );
  }
}

TEST_CASE( "[render/typer] frame position/monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );
  typer.layout().monospace = true;

  REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
  REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

  typer.write( "hello" );
  REQUIRE( typer.position() ==
           point{ .x = 20 + 4 * 5, .y = 30 } );
  REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

  typer.move_frame_by( size{ .w = 10, .h = 30 } );
  REQUIRE( typer.position() ==
           point{ .x = 30 + 4 * 5, .y = 60 } );
  REQUIRE( typer.line_start() == point{ .x = 30, .y = 60 } );

  auto typer2 =
      typer.with_frame_offset( size{ .w = 5, .h = 3 } );
  REQUIRE( typer2.position() ==
           point{ .x = 35 + 4 * 5, .y = 63 } );
  REQUIRE( typer2.line_start() == point{ .x = 35, .y = 63 } );
}

TEST_CASE( "[render/typer] frame position/non-monospace" ) {
  vector<GenericVertex> v;
  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), TextLayout{},
               { .x = 20, .y = 30 }, B );
  typer.layout().monospace = false;

  REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
  REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

  typer.write( "hello" );
  REQUIRE( typer.position() ==
           point{ .x = 20 + 3 * 5 + 5, .y = 30 } );
  REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

  typer.move_frame_by( size{ .w = 10, .h = 30 } );
  REQUIRE( typer.position() ==
           point{ .x = 30 + 3 * 5 + 5, .y = 60 } );
  REQUIRE( typer.line_start() == point{ .x = 30, .y = 60 } );

  auto typer2 =
      typer.with_frame_offset( size{ .w = 5, .h = 3 } );
  REQUIRE( typer2.position() ==
           point{ .x = 35 + 3 * 5 + 5, .y = 63 } );
  REQUIRE( typer2.line_start() == point{ .x = 35, .y = 63 } );
}

} // namespace
} // namespace rr
