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
    return AsciiFont( std::move( ids ), size{ .w = 2, .h = 4 } );
  }();
  return af;
}

AtlasMap const& atlas_map() {
  static const auto m = [] {
    vector<rect> rects( 356, rect{} );
    for( int i = 100; i < 100 + 256; ++i ) {
      int char_idx = i - 100;
      int row      = char_idx / 16;
      int col      = char_idx % 16;
      rects[i] = rect{ .origin = { .x = col * 2, .y = row * 4 },
                       .size   = { .w = 2, .h = 4 } };
    }
    return AtlasMap( std::move( rects ) );
  }();
  return m;
}

TEST_CASE( "[render/typer] write_char" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), { .x = 20, .y = 30 }, B );

  auto Vert = [&]( point p, point atlas_p ) {
    return SilhouetteVertex( p, atlas_p, B ).generic();
  };

  typer.write( "ha {}\nYes", "bob" );
  REQUIRE( v.size() == 9 * 6 );
  REQUIRE( typer.position() ==
           point{ .x = 20 + 6, .y = 30 + 4 } );
  REQUIRE( typer.color() == B );
  REQUIRE( typer.scale() == size{ .w = 2, .h = 4 } );

  // clang-format off
  expected = {
      // (20, 30): h
      Vert( { .x = 20+2*0, .y = 30 }, { .x = 8*2,   .y = 6*4   } ),
      Vert( { .x = 20+2*0, .y = 34 }, { .x = 8*2,   .y = 6*4+4 } ),
      Vert( { .x = 22+2*0, .y = 34 }, { .x = 8*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20+2*0, .y = 30 }, { .x = 8*2,   .y = 6*4   } ),
      Vert( { .x = 22+2*0, .y = 30 }, { .x = 8*2+2, .y = 6*4   } ),
      Vert( { .x = 22+2*0, .y = 34 }, { .x = 8*2+2, .y = 6*4+4 } ),
      // (22, 30): a
      Vert( { .x = 20+2*1, .y = 30 }, { .x = 1*2,   .y = 6*4   } ),
      Vert( { .x = 20+2*1, .y = 34 }, { .x = 1*2,   .y = 6*4+4 } ),
      Vert( { .x = 22+2*1, .y = 34 }, { .x = 1*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20+2*1, .y = 30 }, { .x = 1*2,   .y = 6*4   } ),
      Vert( { .x = 22+2*1, .y = 30 }, { .x = 1*2+2, .y = 6*4   } ),
      Vert( { .x = 22+2*1, .y = 34 }, { .x = 1*2+2, .y = 6*4+4 } ),
      // (24, 30): ' '
      Vert( { .x = 20+2*2, .y = 30 }, { .x = 0*2,   .y = 2*4   } ),
      Vert( { .x = 20+2*2, .y = 34 }, { .x = 0*2,   .y = 2*4+4 } ),
      Vert( { .x = 22+2*2, .y = 34 }, { .x = 0*2+2, .y = 2*4+4 } ),
      Vert( { .x = 20+2*2, .y = 30 }, { .x = 0*2,   .y = 2*4   } ),
      Vert( { .x = 22+2*2, .y = 30 }, { .x = 0*2+2, .y = 2*4   } ),
      Vert( { .x = 22+2*2, .y = 34 }, { .x = 0*2+2, .y = 2*4+4 } ),
      // (26, 30): b
      Vert( { .x = 20+2*3, .y = 30 }, { .x = 2*2,   .y = 6*4   } ),
      Vert( { .x = 20+2*3, .y = 34 }, { .x = 2*2,   .y = 6*4+4 } ),
      Vert( { .x = 22+2*3, .y = 34 }, { .x = 2*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20+2*3, .y = 30 }, { .x = 2*2,   .y = 6*4   } ),
      Vert( { .x = 22+2*3, .y = 30 }, { .x = 2*2+2, .y = 6*4   } ),
      Vert( { .x = 22+2*3, .y = 34 }, { .x = 2*2+2, .y = 6*4+4 } ),
      // (28, 30): o
      Vert( { .x = 20+2*4, .y = 30 }, { .x = 15*2,   .y = 6*4   } ),
      Vert( { .x = 20+2*4, .y = 34 }, { .x = 15*2,   .y = 6*4+4 } ),
      Vert( { .x = 22+2*4, .y = 34 }, { .x = 15*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20+2*4, .y = 30 }, { .x = 15*2,   .y = 6*4   } ),
      Vert( { .x = 22+2*4, .y = 30 }, { .x = 15*2+2, .y = 6*4   } ),
      Vert( { .x = 22+2*4, .y = 34 }, { .x = 15*2+2, .y = 6*4+4 } ),
      // (30, 30): b
      Vert( { .x = 20+2*5, .y = 30 }, { .x = 2*2,   .y = 6*4   } ),
      Vert( { .x = 20+2*5, .y = 34 }, { .x = 2*2,   .y = 6*4+4 } ),
      Vert( { .x = 22+2*5, .y = 34 }, { .x = 2*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20+2*5, .y = 30 }, { .x = 2*2,   .y = 6*4   } ),
      Vert( { .x = 22+2*5, .y = 30 }, { .x = 2*2+2, .y = 6*4   } ),
      Vert( { .x = 22+2*5, .y = 34 }, { .x = 2*2+2, .y = 6*4+4 } ),
      // (20, 34): Y
      Vert( { .x = 20+2*0, .y = 34 }, { .x = 9*2,   .y = 5*4   } ),
      Vert( { .x = 20+2*0, .y = 38 }, { .x = 9*2,   .y = 5*4+4 } ),
      Vert( { .x = 22+2*0, .y = 38 }, { .x = 9*2+2, .y = 5*4+4 } ),
      Vert( { .x = 20+2*0, .y = 34 }, { .x = 9*2,   .y = 5*4   } ),
      Vert( { .x = 22+2*0, .y = 34 }, { .x = 9*2+2, .y = 5*4   } ),
      Vert( { .x = 22+2*0, .y = 38 }, { .x = 9*2+2, .y = 5*4+4 } ),
      // (20, 34): e
      Vert( { .x = 20+2*1, .y = 34 }, { .x = 5*2,   .y = 6*4   } ),
      Vert( { .x = 20+2*1, .y = 38 }, { .x = 5*2,   .y = 6*4+4 } ),
      Vert( { .x = 22+2*1, .y = 38 }, { .x = 5*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20+2*1, .y = 34 }, { .x = 5*2,   .y = 6*4   } ),
      Vert( { .x = 22+2*1, .y = 34 }, { .x = 5*2+2, .y = 6*4   } ),
      Vert( { .x = 22+2*1, .y = 38 }, { .x = 5*2+2, .y = 6*4+4 } ),
      // (20, 34): s
      Vert( { .x = 20+2*2, .y = 34 }, { .x = 3*2,   .y = 7*4   } ),
      Vert( { .x = 20+2*2, .y = 38 }, { .x = 3*2,   .y = 7*4+4 } ),
      Vert( { .x = 22+2*2, .y = 38 }, { .x = 3*2+2, .y = 7*4+4 } ),
      Vert( { .x = 20+2*2, .y = 34 }, { .x = 3*2,   .y = 7*4   } ),
      Vert( { .x = 22+2*2, .y = 34 }, { .x = 3*2+2, .y = 7*4   } ),
      Vert( { .x = 22+2*2, .y = 38 }, { .x = 3*2+2, .y = 7*4+4 } ),
  };
  // clang-format on
  REQUIRE( v == expected );
}

TEST_CASE( "[render/typer] write_char scaled" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), { .x = 20, .y = 30 }, B );
  typer.set_scale( size{ .w = 4, .h = 8 } );

  auto Vert = [&]( point p, point atlas_p ) {
    return SilhouetteVertex( p, atlas_p, B ).generic();
  };

  typer.write( 'h' );
  REQUIRE( v.size() == 1 * 6 );
  REQUIRE( typer.position() == point{ .x = 20 + 4, .y = 30 } );
  REQUIRE( typer.color() == B );
  REQUIRE( typer.scale() == size{ .w = 4, .h = 8 } );

  // clang-format off
  expected = {
      // (20, 30): h
      Vert( { .x = 20, .y = 30 }, { .x = 8*2,   .y = 6*4   } ),
      Vert( { .x = 20, .y = 38 }, { .x = 8*2,   .y = 6*4+4 } ),
      Vert( { .x = 24, .y = 38 }, { .x = 8*2+2, .y = 6*4+4 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 8*2,   .y = 6*4   } ),
      Vert( { .x = 24, .y = 30 }, { .x = 8*2+2, .y = 6*4   } ),
      Vert( { .x = 24, .y = 38 }, { .x = 8*2+2, .y = 6*4+4 } ),
  };
  // clang-format on
  REQUIRE( v == expected );
}

TEST_CASE( "[render/typer] dimensions_for_line" ) {
  vector<GenericVertex> v;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), { .x = 20, .y = 30 }, B );

  SECTION( "default scale" ) {
    REQUIRE( typer.dimensions_for_line( "" ) ==
             size{ .w = 2 * 0, .h = 4 } );
    REQUIRE( typer.dimensions_for_line( "h" ) ==
             size{ .w = 2 * 1, .h = 4 } );
    REQUIRE( typer.dimensions_for_line( "hello" ) ==
             size{ .w = 2 * 5, .h = 4 } );
    REQUIRE( typer.dimensions_for_line( "hello\nhello" ) ==
             size{ .w = 2 * 11, .h = 4 } );
  }

  SECTION( "larger scale" ) {
    typer.set_scale( size{ .w = 4, .h = 8 } );
    REQUIRE( typer.dimensions_for_line( "" ) ==
             size{ .w = 4 * 0, .h = 8 } );
    REQUIRE( typer.dimensions_for_line( "h" ) ==
             size{ .w = 4 * 1, .h = 8 } );
    REQUIRE( typer.dimensions_for_line( "hello" ) ==
             size{ .w = 4 * 5, .h = 8 } );
    REQUIRE( typer.dimensions_for_line( "hello\nhello" ) ==
             size{ .w = 4 * 11, .h = 8 } );
  }
}

TEST_CASE( "[render/typer] frame position" ) {
  vector<GenericVertex> v;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );
  Typer typer( painter, ascii_font(), { .x = 20, .y = 30 }, B );

  REQUIRE( typer.position() == point{ .x = 20, .y = 30 } );
  REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

  typer.write( "hello" );
  REQUIRE( typer.position() ==
           point{ .x = 20 + 2 * 5, .y = 30 } );
  REQUIRE( typer.line_start() == point{ .x = 20, .y = 30 } );

  typer.move_frame_by( size{ .w = 10, .h = 30 } );
  REQUIRE( typer.position() ==
           point{ .x = 30 + 2 * 5, .y = 60 } );
  REQUIRE( typer.line_start() == point{ .x = 30, .y = 60 } );

  auto typer2 =
      typer.with_frame_offset( size{ .w = 5, .h = 3 } );
  REQUIRE( typer.position() ==
           point{ .x = 35 + 2 * 5, .y = 63 } );
  REQUIRE( typer.line_start() == point{ .x = 35, .y = 63 } );
}

} // namespace
} // namespace rr
