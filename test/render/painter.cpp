/****************************************************************
**painter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Unit tests for the src/render/painter.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/painter.hpp"

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

using ::base::nothing;
using ::gfx::dsize;
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
      /*id=6*/
      { .origin = { .x = 7, .y = 10 },
        .size   = { .w = 8, .h = 9 } },
  } );
  return m;
}

constexpr pixel R{ .r = 255, .g = 0, .b = 0, .a = 255 };
constexpr pixel G{ .r = 0, .g = 255, .b = 0, .a = 128 };
constexpr pixel B{ .r = 0, .g = 0, .b = 255, .a = 10 };
constexpr pixel W{ .r = 255, .g = 255, .b = 255, .a = 255 };

rect get_atlas_rect( int id ) {
  return atlas_map().lookup( id );
}

TEST_CASE( "[render/painter] draw_solid_rect" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

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

TEST_CASE( "[render/painter] draw_horizontal_line" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

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

TEST_CASE( "[render/painter] draw_vertical_line" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

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

TEST_CASE( "[render/painter] draw_point" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

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

TEST_CASE( "[render/painter] draw_empty_rect inner" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

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

TEST_CASE( "[render/painter] draw_empty_rect in_out" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  rect                   r;
  Painter::e_border_mode mode = Painter::e_border_mode::in_out;

  auto Vert = [&]( point p ) {
    return SolidVertex( p, B ).generic();
  };

  SECTION( "10x10 square" ) {
    r = rect{ .origin = { .x = 20, .y = 30 },
              .size   = { .w = 10, .h = 10 } };
    painter.draw_empty_rect( r, mode, B );
    expected = {
        // Horizontal line from (20,30) with length 10.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 31 } ),
        Vert( { .x = 30, .y = 31 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 30, .y = 30 } ),
        Vert( { .x = 30, .y = 31 } ),
        // Vertical line from (30,30) with length 11.
        Vert( { .x = 30, .y = 30 } ),
        Vert( { .x = 30, .y = 41 } ),
        Vert( { .x = 31, .y = 41 } ),
        Vert( { .x = 30, .y = 30 } ),
        Vert( { .x = 31, .y = 30 } ),
        Vert( { .x = 31, .y = 41 } ),
        // Horizontal line from (20,40) with length 10.
        Vert( { .x = 20, .y = 40 } ),
        Vert( { .x = 20, .y = 41 } ),
        Vert( { .x = 30, .y = 41 } ),
        Vert( { .x = 20, .y = 40 } ),
        Vert( { .x = 30, .y = 40 } ),
        Vert( { .x = 30, .y = 41 } ),
        // Vertical line from (20,30) with length 11.
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 20, .y = 41 } ),
        Vert( { .x = 21, .y = 41 } ),
        Vert( { .x = 20, .y = 30 } ),
        Vert( { .x = 21, .y = 30 } ),
        Vert( { .x = 21, .y = 41 } ),
    };
    REQUIRE( v == expected );
  }
}

TEST_CASE( "[render/painter] draw_empty_rect outter" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

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

TEST_CASE( "[render/painter] draw_sprite" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  point p;

  rect const atlas_rect = get_atlas_rect( 1 );

  auto Vert = [&]( point p, point atlas_p ) {
    return SpriteVertex( p, atlas_p, atlas_rect ).generic();
  };

  p            = { .x = 20, .y = 30 };
  int atlas_id = 1;
  painter.draw_sprite( atlas_id, p );
  // atlas: { .origin = { .x = 2, .y = 3 },
  //          .size   = { .w = 4, .h = 5 } },
  expected = {
      Vert( { .x = 20, .y = 30 }, { .x = 2, .y = 3 } ),
      Vert( { .x = 20, .y = 35 }, { .x = 2, .y = 8 } ),
      Vert( { .x = 24, .y = 35 }, { .x = 6, .y = 8 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 2, .y = 3 } ),
      Vert( { .x = 24, .y = 30 }, { .x = 6, .y = 3 } ),
      Vert( { .x = 24, .y = 35 }, { .x = 6, .y = 8 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] draw_stencil" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  point p;
  // This is the offset between the origins of the point and re-
  // placement point.
  gfx::size offset{ .w = 1, .h = 1 };

  rect const atlas_rect = get_atlas_rect( 2 );

  auto Vert = [&]( point p, point atlas_p ) {
    return StencilVertex( p, atlas_p, atlas_rect, offset, R )
        .generic();
  };

  p                  = { .x = 20, .y = 30 };
  int atlas_id       = 2;
  int replacement_id = 3;
  painter.draw_stencil( atlas_id, replacement_id, p, R );
  // atlas: { .origin = { .x = 3, .y = 4 },
  //          .size   = { .w = 5, .h = 6 } },
  // replacement:
  //        { .origin = { .x = 4, .y = 5 },
  //          .size   = { .w = 6, .h = 7 } },
  expected = {
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 4 } ),
      Vert( { .x = 20, .y = 36 }, { .x = 3, .y = 10 } ),
      Vert( { .x = 25, .y = 36 }, { .x = 8, .y = 10 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 4 } ),
      Vert( { .x = 25, .y = 30 }, { .x = 8, .y = 4 } ),
      Vert( { .x = 25, .y = 36 }, { .x = 8, .y = 10 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] draw_sprite_scale" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  rect r;

  rect const atlas_rect = get_atlas_rect( 1 );

  auto Vert = [&]( point p, point atlas_p ) {
    return SpriteVertex( p, atlas_p, atlas_rect ).generic();
  };

  r            = rect{ .origin = { .x = 20, .y = 30 },
                       .size   = { .w = 8, .h = 10 } };
  int atlas_id = 1;
  painter.draw_sprite_scale( atlas_id, r );
  // atlas: { .origin = { .x = 2, .y = 3 },
  //          .size   = { .w = 4, .h = 5 } },
  expected = {
      Vert( { .x = 20, .y = 30 }, { .x = 2, .y = 3 } ),
      Vert( { .x = 20, .y = 40 }, { .x = 2, .y = 8 } ),
      Vert( { .x = 28, .y = 40 }, { .x = 6, .y = 8 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 2, .y = 3 } ),
      Vert( { .x = 28, .y = 30 }, { .x = 6, .y = 3 } ),
      Vert( { .x = 28, .y = 40 }, { .x = 6, .y = 8 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] draw_sprite_section" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter painter( atlas_map(), emitter );

  rect const atlas_rect = rect{ .origin = { .x = 3, .y = 5 },
                                .size   = { .w = 3, .h = 3 } };

  auto Vert = [&]( point p, point atlas_p ) {
    return SpriteVertex( p, atlas_p, atlas_rect ).generic();
  };

  point p{ .x = 20, .y = 30 };
  rect  section{ .origin = { .x = 1, .y = 2 },
                 .size   = { .w = 10, .h = 10 } };

  int atlas_id = 1;
  painter.draw_sprite_section( atlas_id, p, section );
  // atlas: { .origin = { .x = 2, .y = 3 },
  //          .size   = { .w = 4, .h = 5 } },
  expected = {
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 5 } ),
      Vert( { .x = 20, .y = 33 }, { .x = 3, .y = 8 } ),
      Vert( { .x = 23, .y = 33 }, { .x = 6, .y = 8 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 5 } ),
      Vert( { .x = 23, .y = 30 }, { .x = 6, .y = 5 } ),
      Vert( { .x = 23, .y = 33 }, { .x = 6, .y = 8 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod depixelate to blank" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );

  Painter painter = unmodded_painter.with_mods(
      { .depixelate =
            DepixelateInfo{
                .stage       = .7,
                .inverted    = {},
                .hash_anchor = point{ .x = 1, .y = 2 } },
        .alpha       = nothing,
        .repos       = {},
        .desaturate  = nothing,
        .fixed_color = nothing } );

  point p;

  rect const atlas_rect = get_atlas_rect( 2 );

  auto Vert = [&]( point p, point atlas_p ) {
    auto vert = SpriteVertex( p, atlas_p, atlas_rect );
    vert.set_depixelation_stage( .7 );
    vert.set_depixelation_hash_anchor( { .x = 1, .y = 2 } );
    return vert.generic();
  };

  p            = { .x = 20, .y = 30 };
  int atlas_id = 2;
  painter.draw_sprite( atlas_id, p );
  // atlas: { .origin = { .x = 3, .y = 4 },
  //          .size   = { .w = 5, .h = 6 } },
  expected = {
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 4 } ),
      Vert( { .x = 20, .y = 36 }, { .x = 3, .y = 10 } ),
      Vert( { .x = 25, .y = 36 }, { .x = 8, .y = 10 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 4 } ),
      Vert( { .x = 25, .y = 30 }, { .x = 8, .y = 4 } ),
      Vert( { .x = 25, .y = 36 }, { .x = 8, .y = 10 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod depixelate with inversion" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );

  Painter painter = unmodded_painter.with_mods(
      { .depixelate =
            DepixelateInfo{
                .stage       = .7,
                .inverted    = true,
                .hash_anchor = point{ .x = 1, .y = 2 } },
        .alpha       = nothing,
        .repos       = {},
        .desaturate  = nothing,
        .fixed_color = nothing } );

  point p;

  rect const atlas_rect = get_atlas_rect( 2 );

  auto Vert = [&]( point p, point atlas_p ) {
    auto vert = SpriteVertex( p, atlas_p, atlas_rect );
    vert.set_depixelation_stage( .7 );
    vert.set_depixelation_inversion( true );
    vert.set_depixelation_hash_anchor( { .x = 1, .y = 2 } );
    return vert.generic();
  };

  p            = { .x = 20, .y = 30 };
  int atlas_id = 2;
  painter.draw_sprite( atlas_id, p );
  // atlas: { .origin = { .x = 3, .y = 4 },
  //          .size   = { .w = 5, .h = 6 } },
  expected = {
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 4 } ),
      Vert( { .x = 20, .y = 36 }, { .x = 3, .y = 10 } ),
      Vert( { .x = 25, .y = 36 }, { .x = 8, .y = 10 } ),
      Vert( { .x = 20, .y = 30 }, { .x = 3, .y = 4 } ),
      Vert( { .x = 25, .y = 30 }, { .x = 8, .y = 4 } ),
      Vert( { .x = 25, .y = 36 }, { .x = 8, .y = 10 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod alpha" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );
  Painter painter =
      unmodded_painter.with_mods( { .depixelate  = {},
                                    .alpha       = .7,
                                    .repos       = {},
                                    .desaturate  = nothing,
                                    .fixed_color = nothing } );

  rect r;

  auto Vert = [&]( point p ) {
    auto vert = SolidVertex( p, G );
    vert.set_alpha( .7 );
    return vert.generic();
  };

  r = rect{ .origin = { .x = 20, .y = 30 },
            .size   = { .w = 100, .h = 200 } };
  painter.draw_solid_rect( r, G );
  expected = {
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 20, .y = 230 } ),
      Vert( { .x = 120, .y = 230 } ),
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 120, .y = 30 } ),
      Vert( { .x = 120, .y = 230 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod desaturate" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );
  Painter painter =
      unmodded_painter.with_mods( { .depixelate  = {},
                                    .alpha       = {},
                                    .repos       = {},
                                    .desaturate  = true,
                                    .fixed_color = nothing } );

  rect r;

  auto Vert = [&]( point p ) {
    auto vert = SolidVertex( p, G );
    vert.set_desaturate( true );
    return vert.generic();
  };

  r = rect{ .origin = { .x = 20, .y = 30 },
            .size   = { .w = 100, .h = 200 } };
  painter.draw_solid_rect( r, G );
  expected = {
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 20, .y = 230 } ),
      Vert( { .x = 120, .y = 230 } ),
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 120, .y = 30 } ),
      Vert( { .x = 120, .y = 230 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod fixed_color" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );
  Painter painter = unmodded_painter.with_mods(
      { .depixelate  = {},
        .alpha       = {},
        .repos       = {},
        .desaturate  = {},
        .fixed_color = gfx::pixel::red() } );

  rect r;

  auto Vert = [&]( point p ) {
    auto vert = SolidVertex( p, G );
    vert.set_fixed_color(
        gfx::pixel{ .r = 255, .g = 0, .b = 0, .a = 255 } );
    return vert.generic();
  };

  r = rect{ .origin = { .x = 20, .y = 30 },
            .size   = { .w = 100, .h = 200 } };
  painter.draw_solid_rect( r, G );
  expected = {
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 20, .y = 230 } ),
      Vert( { .x = 120, .y = 230 } ),
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 120, .y = 30 } ),
      Vert( { .x = 120, .y = 230 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod cycling" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );
  Painter painter = unmodded_painter.with_mods(
      { .depixelate  = {},
        .alpha       = {},
        .repos       = {},
        .cycling     = { .enabled = true },
        .desaturate  = nothing,
        .fixed_color = nothing } );

  rect r;

  auto Vert = [&]( point p ) {
    auto vert = SolidVertex( p, G );
    vert.set_color_cycle( true );
    return vert.generic();
  };

  r = rect{ .origin = { .x = 20, .y = 30 },
            .size   = { .w = 100, .h = 200 } };
  painter.draw_solid_rect( r, G );
  expected = {
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 20, .y = 230 } ),
      Vert( { .x = 120, .y = 230 } ),
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 120, .y = 30 } ),
      Vert( { .x = 120, .y = 230 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod reposition" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );
  Painter painter = unmodded_painter.with_mods(
      { .depixelate = {},
        .alpha      = {},
        .repos      = RepositionInfo{
                 .scale       = 2.0,
                 .translation = dsize{ .w = 5, .h = 3 },
                 .use_camera  = false,
        } } );

  auto Vert = [&]( point p ) {
    auto vert = SolidVertex( p, G );
    vert.set_scaling( 2.0 );
    vert.set_translation( dsize{ .w = 5, .h = 3 } );
    return vert.generic();
  };

  rect r = rect{ .origin = { .x = 20, .y = 30 },
                 .size   = { .w = 100, .h = 200 } };
  painter.draw_solid_rect( r, G );
  expected = {
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 20, .y = 230 } ),
      Vert( { .x = 120, .y = 230 } ),
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 120, .y = 30 } ),
      Vert( { .x = 120, .y = 230 } ),
  };
  REQUIRE( v == expected );
}

TEST_CASE( "[render/painter] mod use_camera" ) {
  vector<GenericVertex> v, expected;

  Emitter emitter( v );
  Painter unmodded_painter( atlas_map(), emitter );
  Painter painter = unmodded_painter.with_mods(
      { .depixelate = {},
        .alpha      = {},
        .repos =
            RepositionInfo{
                .scale       = 2.0,
                .translation = dsize{ .w = 5.3, .h = 3 },
                .use_camera  = true,
            },
        .desaturate  = nothing,
        .fixed_color = nothing } );

  auto Vert = [&]( point p ) {
    auto vert = SolidVertex( p, G );
    vert.set_scaling( 2.0 );
    vert.set_translation( dsize{ .w = 5.3, .h = 3 } );
    vert.set_use_camera( true );
    return vert.generic();
  };

  rect r = rect{ .origin = { .x = 20, .y = 30 },
                 .size   = { .w = 100, .h = 200 } };
  painter.draw_solid_rect( r, G );
  expected = {
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 20, .y = 230 } ),
      Vert( { .x = 120, .y = 230 } ),
      Vert( { .x = 20, .y = 30 } ),
      Vert( { .x = 120, .y = 30 } ),
      Vert( { .x = 120, .y = 230 } ),
  };
  REQUIRE( v == expected );
}

} // namespace
} // namespace rr
