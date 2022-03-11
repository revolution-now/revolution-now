/****************************************************************
**draw.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Unit tests for the src/render/draw.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/draw.hpp"

// render
#include "atlas.hpp"
#include "emitter.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

AtlasMap const& atlas_map() {
  static const auto m = AtlasMap( vector<rect>{
      /*id=0*/ { .origin = { .x = 0, .y = 1 },
                 .size   = { .w = 2, .h = 3 } },
      /*id=1*/
      { .origin = { .x = 2, .y = 3 },
        .size   = { .w = 4, .h = 5 } },
      /*id=2*/
      { .origin = { .x = 3, .y = 4 },
        .size   = { .w = 5, .h = 6 } },
      /*id=3*/
      { .origin = { .x = 4, .y = 5 },
        .size   = { .w = 6, .h = 7 } },
      /*id=4*/
      { .origin = { .x = 5, .y = 6 },
        .size   = { .w = 7, .h = 8 } },
      /*id=5*/
      { .origin = { .x = 6, .y = 7 },
        .size   = { .w = 8, .h = 9 } },
  } );
  return m;
}

constexpr pixel R{ .r = 255, .g = 0, .b = 0, .a = 255 };
constexpr pixel G{ .r = 0, .g = 255, .b = 0, .a = 128 };
constexpr pixel B{ .r = 0, .g = 0, .b = 255, .a = 10 };
constexpr pixel W{ .r = 255, .g = 255, .b = 255, .a = 255 };

TEST_CASE( "[render/draw] draw_solid_rect" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );

  rect r;

  auto VertR = [&]( point p ) {
    return SolidVertex( p, R ).generic();
  };
  auto VertG = [&]( point p ) {
    return SolidVertex( p, G ).generic();
  };
  auto VertB = [&]( point p ) {
    return SolidVertex( p, B ).generic();
  };

  SECTION( "empty" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = 0, .h = 0 } };
    painter.draw_solid_rect( r, R );
    expected = {
        VertR( { .x = 20, .y = 30 } ),
        VertR( { .x = 20, .y = 30 } ),
        VertR( { .x = 20, .y = 30 } ),
        VertR( { .x = 20, .y = 30 } ),
        VertR( { .x = 20, .y = 30 } ),
        VertR( { .x = 20, .y = 30 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "normal" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = 100, .h = 200 } };
    painter.draw_solid_rect( r, G );
    expected = {
        VertG( { .x = 20, .y = 30 } ),
        VertG( { .x = 20, .y = 230 } ),
        VertG( { .x = 120, .y = 230 } ),
        VertG( { .x = 20, .y = 30 } ),
        VertG( { .x = 120, .y = 30 } ),
        VertG( { .x = 120, .y = 230 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "non-normalized" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = -100, .h = 200 } };
    painter.draw_solid_rect( r, B );
    expected = {
        VertB( { .x = -80, .y = 30 } ),
        VertB( { .x = -80, .y = 230 } ),
        VertB( { .x = 20, .y = 230 } ),
        VertB( { .x = -80, .y = 30 } ),
        VertB( { .x = 20, .y = 30 } ),
        VertB( { .x = 20, .y = 230 } ),
    };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[render/draw] draw_horizontal_line" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );

  point p;
  int   length = 0;

  auto Vert = [&]( point p ) {
    return SolidVertex( p, G ).generic();
  };

  SECTION( "empty" ) {
    p      = point{ .x = 20, .y = 30 };
    length = 0;
    painter.draw_horizontal_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "length 1" ) {
    p      = point{ .x = 20, .y = 30 };
    length = 1;
    painter.draw_horizontal_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 31 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "length 10" ) {
    p      = point{ .x = 20, .y = 30 };
    length = 10;
    painter.draw_horizontal_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 30, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 30, .y = 30 } ),
        Vert( { .x = 30, .y = 31 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "non-normalized" ) {
    p      = point{ .x = 20, .y = 30 };
    length = -5;
    painter.draw_horizontal_line( p, length, G );
    expected = {
        Vert( { .x = 15, .y = 30 } ),
        Vert( { .x = 15, .y = 31 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 15, .y = 30 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
    };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[render/draw] draw_vertical_line" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );

  point p;
  int   length = 0;

  auto Vert = [&]( point p ) {
    return SolidVertex( p, G ).generic();
  };

  SECTION( "empty" ) {
    p      = point{ .x = 20, .y = 30 };
    length = 0;
    painter.draw_vertical_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "length 1" ) {
    p      = point{ .x = 20, .y = 30 };
    length = 1;
    painter.draw_vertical_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 31 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "length 10" ) {
    p      = point{ .x = 20, .y = 30 };
    length = 10;
    painter.draw_vertical_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 40 } ),
        Vert( { .x = 21, .y = 40 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 40 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "non-normalized" ) {
    p      = point{ .x = 20, .y = 30 };
    length = -5;
    painter.draw_vertical_line( p, length, G );
    expected = {
        Vert( { .x = 20, .y = 25 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 20, .y = 25 } ),
        Vert( { .x = 21, .y = 25 } ),
        Vert( { .x = 21, .y = 30 } ),
    };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[render/draw] draw_point" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );

  point p;

  auto Vert = [&]( point p ) {
    return SolidVertex( p, W ).generic();
  };

  p = point{ .x = 20, .y = 30 };
  painter.draw_point( p, W );
  expected = {
      Vert( { .x = 20, .y = 30 } ), Vert( { .x = 20, .y = 31 } ),
      Vert( { .x = 21, .y = 31 } ), Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 21, .y = 30 } ), Vert( { .x = 21, .y = 31 } ),
  };
  REQUIRE( v == expected );

  p = point{ .x = -1, .y = 0 };
  painter.draw_point( p, W );
  expected = {
      Vert( { .x = 20, .y = 30 } ), Vert( { .x = 20, .y = 31 } ),
      Vert( { .x = 21, .y = 31 } ), Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 21, .y = 30 } ), Vert( { .x = 21, .y = 31 } ),
      Vert( { .x = -1, .y = 0 } ),  Vert( { .x = -1, .y = 1 } ),
      Vert( { .x = 0, .y = 1 } ),   Vert( { .x = -1, .y = 0 } ),
      Vert( { .x = 0, .y = 0 } ),   Vert( { .x = 0, .y = 1 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/draw] draw_empty_rect inner" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );

  rect                   r;
  Painter::e_border_mode mode = Painter::e_border_mode::inside;

  auto Vert = [&]( point p ) {
    return SolidVertex( p, B ).generic();
  };

  SECTION( "empty" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = 0, .h = 0 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {};
    REQUIRE( v == expected );
  }

  SECTION( "1x1 square" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = 1, .h = 1 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 0.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        // Vertical line from (20,30) with length 1.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 31 } ),
        // Horizontal line from (20,30) with length 0.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        // Vertical line from (20,30) with length 1.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 31 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "10x10 square" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = 10, .h = 10 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 9.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 29, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 29, .y = 30 } ),
        Vert( { .x = 29, .y = 31 } ),
        // Vertical line from (29,30) with length 10.
        Vert( { .x = 29, .y = 30 } ),
        Vert( { .x = 29, .y = 40 } ),
        Vert( { .x = 30, .y = 40 } ),
        Vert( { .x = 29, .y = 30 } ),
        Vert( { .x = 30, .y = 30 } ),
        Vert( { .x = 30, .y = 40 } ),
        // Horizontal line from (20,39) with length 9.
        Vert( { .x = 20, .y = 39 } ),
        Vert( { .x = 20, .y = 40 } ),
        Vert( { .x = 29, .y = 40 } ),
        Vert( { .x = 20, .y = 39 } ),
        Vert( { .x = 29, .y = 39 } ),
        Vert( { .x = 29, .y = 40 } ),
        // Vertical line from (20,30) with length 10.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 40 } ),
        Vert( { .x = 21, .y = 40 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 40 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "-2 x -2 square" ) {
    r = rect{ .origin = { .x = 22, .y = 32 },
              .size   = { .w = -2, .h = -2 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 1.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 31 } ),
        // Vertical line from (21,30) with length 2.
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 32 } ),
        Vert( { .x = 22, .y = 32 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 22, .y = 30 } ),
        Vert( { .x = 22, .y = 32 } ),
        // Horizontal line from (20,31) with length 1.
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 32 } ),
        Vert( { .x = 21, .y = 32 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 21, .y = 32 } ),
        // Vertical line from (20,30) with length 2.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 32 } ),
        Vert( { .x = 21, .y = 32 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 32 } ),
    };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[render/draw] draw_empty_rect outter" ) {
  vector<GenericVertex> v, expected;
  Emitter               emitter( v );
  Painter               painter( atlas_map(), emitter );

  rect                   r;
  Painter::e_border_mode mode = Painter::e_border_mode::outside;

  auto Vert = [&]( point p ) {
    return SolidVertex( p, B ).generic();
  };

  SECTION( "empty" ) {
    r = rect{ .origin = { .x = 21, .y = 31 },
              .size   = { .w = 0, .h = 0 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 1.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 31 } ),
        // Vertical line from (21,30) with length 2.
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 32 } ),
        Vert( { .x = 22, .y = 32 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 22, .y = 30 } ),
        Vert( { .x = 22, .y = 32 } ),
        // Horizontal line from (20,31) with length 1.
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 20, .y = 32 } ),
        Vert( { .x = 21, .y = 32 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 21, .y = 31 } ),
        Vert( { .x = 21, .y = 32 } ),
        // Vertical line from (20,30) with length 2.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 32 } ),
        Vert( { .x = 21, .y = 32 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 32 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "1x1 square" ) {
    r = rect{ .origin = { .x = 21, .y = 31 },
              .size   = { .w = 1, .h = 1 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 2.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 22, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 22, .y = 30 } ),
        Vert( { .x = 22, .y = 31 } ),
        // Vertical line from (22,30) with length 3.
        Vert( { .x = 22, .y = 30 } ),
        Vert( { .x = 22, .y = 33 } ),
        Vert( { .x = 23, .y = 33 } ),
        Vert( { .x = 22, .y = 30 } ),
        Vert( { .x = 23, .y = 30 } ),
        Vert( { .x = 23, .y = 33 } ),
        // Horizontal line from (20,32) with length 2.
        Vert( { .x = 20, .y = 32 } ),
        Vert( { .x = 20, .y = 33 } ),
        Vert( { .x = 22, .y = 33 } ),
        Vert( { .x = 20, .y = 32 } ),
        Vert( { .x = 22, .y = 32 } ),
        Vert( { .x = 22, .y = 33 } ),
        // Vertical line from (20,30) with length 3.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 33 } ),
        Vert( { .x = 21, .y = 33 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 33 } ),
    };
    REQUIRE( v == expected );
  }

  SECTION( "10x10 square" ) {
    r = rect{ .origin = { .x = 21, .y = 31 },
              .size   = { .w = 10, .h = 10 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 11.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 31, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 31, .y = 30 } ),
        Vert( { .x = 31, .y = 31 } ),
        // Vertical line from (31,30) with length 12.
        Vert( { .x = 31, .y = 30 } ),
        Vert( { .x = 31, .y = 42 } ),
        Vert( { .x = 32, .y = 42 } ),
        Vert( { .x = 31, .y = 30 } ),
        Vert( { .x = 32, .y = 30 } ),
        Vert( { .x = 32, .y = 42 } ),
        // Horizontal line from (20,41) with length 11.
        Vert( { .x = 20, .y = 41 } ),
        Vert( { .x = 20, .y = 42 } ),
        Vert( { .x = 31, .y = 42 } ),
        Vert( { .x = 20, .y = 41 } ),
        Vert( { .x = 31, .y = 41 } ),
        Vert( { .x = 31, .y = 42 } ),
        // Vertical line from (20,30) with length 12.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 42 } ),
        Vert( { .x = 21, .y = 42 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 42 } ),
    };
    REQUIRE( v == expected );
  }
}

} // namespace
} // namespace rr
