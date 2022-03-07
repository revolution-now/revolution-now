/****************************************************************
**rect-pack.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-24.
*
* Description: Unit tests for the src/render/rect-pack.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/render/rect-pack.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

TEST_CASE( "[render/rect-pack] empty" ) {
  vector<rect> input;
  size         max_size;

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 0, .h = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 0, .h = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 0, .h = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 0, .h = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 0, .h = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 1x1" ) {
  vector<rect> input = { rect{ .origin = { .x = -1, .y = -1 },
                               .size   = { .w = 1, .h = 1 } } };

  size max_size;

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 1x2" ) {
  vector<rect> input = { rect{ .origin = { .x = -1, .y = -1 },
                               .size   = { .w = 1, .h = 2 } } };
  size         max_size;

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 4 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 4 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 1, .h = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 2x1" ) {
  vector<rect> input = { rect{ .origin = { .x = -1, .y = -1 },
                               .size   = { .w = 2, .h = 1 } } };
  size         max_size;

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 2, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 2, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 4, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 2, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 4, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 2, .h = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 2x2" ) {
  vector<rect> input = { rect{ .origin = { .x = -1, .y = -1 },
                               .size   = { .w = 2, .h = 2 } } };
  size         max_size;

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 2, .h = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 4, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 4 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 4, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             size{ .w = 2, .h = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] multiple 1" ) {
  vector<rect> input;
  auto         add_rect = [&]( size const s ) mutable {
    input.push_back(
                rect{ .origin = { .x = -1, .y = -1 }, .size = s } );
  };

  add_rect( size{ .w = 2, .h = 1 } );
  add_rect( size{ .w = 2, .h = 2 } );
  add_rect( size{ .w = 1, .h = 1 } );
  add_rect( size{ .w = 8, .h = 4 } );
  add_rect( size{ .w = 4, .h = 4 } );
  add_rect( size{ .w = 1, .h = 1 } );
  add_rect( size{ .w = 1, .h = 1 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size      = size{ .w = 100, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[1].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( s == size{ .w = 15, .h = 4 } );
  }

  SECTION( "largest block doesn't have the height" ) {
    max_size      = size{ .w = 100, .h = 3 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "just enough width in first row" ) {
    max_size      = size{ .w = 15, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[1].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( s == size{ .w = 15, .h = 4 } );
  }

  SECTION( "just enough width and height for one row" ) {
    max_size      = size{ .w = 15, .h = 4 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[1].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( s == size{ .w = 15, .h = 4 } );
  }

  SECTION( "enough width in first row - 1" ) {
    max_size      = size{ .w = 14, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[1].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( s == size{ .w = 14, .h = 5 } );
  }

  SECTION(
      "enough width in first row - 1, but not enough height" ) {
    max_size      = size{ .w = 14, .h = 4 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "only large blocks fit in first row" ) {
    max_size      = size{ .w = 13, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 2, .y = 4 } );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 5 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 5 } );
    REQUIRE( input[6].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( s == size{ .w = 12, .h = 6 } );
  }

  SECTION( "only large blocks fit in first row 2" ) {
    max_size      = size{ .w = 12, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 2, .y = 4 } );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 5 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 5 } );
    REQUIRE( input[6].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( s == size{ .w = 12, .h = 6 } );
  }

  SECTION(
      "only large blocks fit in first row, just enough "
      "height" ) {
    max_size      = size{ .w = 12, .h = 6 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 2, .y = 4 } );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 5 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 5 } );
    REQUIRE( input[6].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( s == size{ .w = 12, .h = 6 } );
  }

  SECTION(
      "only large blocks fit in first row, not enough height" ) {
    max_size      = size{ .w = 12, .h = 5 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "only largest block fits in first row" ) {
    max_size      = size{ .w = 11, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 4, .y = 6 } );
    REQUIRE( input[1].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 4, .y = 7 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[5].origin == point{ .x = 5, .y = 7 } );
    REQUIRE( input[6].origin == point{ .x = 6, .y = 4 } );
    REQUIRE( s == size{ .w = 8, .h = 8 } );
  }

  SECTION( "largest block just fits in first row" ) {
    max_size      = size{ .w = 8, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 4, .y = 6 } );
    REQUIRE( input[1].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 4, .y = 7 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[5].origin == point{ .x = 5, .y = 7 } );
    REQUIRE( input[6].origin == point{ .x = 6, .y = 4 } );
    REQUIRE( s == size{ .w = 8, .h = 8 } );
  }

  SECTION( "largest block just fits in first row" ) {
    max_size      = size{ .w = 7, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }
}

TEST_CASE( "[render/rect-pack] multiple 2" ) {
  vector<rect> input;
  auto         add_rect = [&]( size const s ) mutable {
    input.push_back(
                rect{ .origin = { .x = -1, .y = -1 }, .size = s } );
  };

  add_rect( size{ .w = 2, .h = 1 } );
  add_rect( size{ .w = 2, .h = 2 } );
  add_rect( size{ .w = 1, .h = 1 } );
  add_rect( size{ .w = 8, .h = 4 } );
  add_rect( size{ .w = 4, .h = 4 } );
  add_rect( size{ .w = 1, .h = 1 } );
  add_rect( size{ .w = 1, .h = 2 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size      = size{ .w = 100, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 14, .y = 1 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[4].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[5].origin == point{ .x = 15, .y = 1 } );
    REQUIRE( input[6].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( s == size{ .w = 16, .h = 4 } );
  }
}

TEST_CASE( "[render/rect-pack] multiple 3" ) {
  vector<rect> input;
  auto         add_rect = [&]( size const s ) mutable {
    input.push_back(
                rect{ .origin = { .x = -1, .y = -1 }, .size = s } );
  };

  add_rect( size{ .w = 1, .h = 4 } );
  add_rect( size{ .w = 1, .h = 2 } );
  add_rect( size{ .w = 3, .h = 1 } );
  add_rect( size{ .w = 2, .h = 1 } );
  add_rect( size{ .w = 3, .h = 1 } );
  add_rect( size{ .w = 1, .h = 1 } );
  add_rect( size{ .w = 1, .h = 1 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size      = size{ .w = 10, .h = 10 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 1 } );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 2 } );
    REQUIRE( input[5].origin == point{ .x = 2, .y = 3 } );
    REQUIRE( input[6].origin == point{ .x = 3, .y = 3 } );
    REQUIRE( s == size{ .w = 5, .h = 4 } );
  }

  SECTION( "just enough space" ) {
    max_size      = size{ .w = 5, .h = 4 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 1 } );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 2 } );
    REQUIRE( input[5].origin == point{ .x = 2, .y = 3 } );
    REQUIRE( input[6].origin == point{ .x = 3, .y = 3 } );
    REQUIRE( s == size{ .w = 5, .h = 4 } );
  }

  SECTION( "not enough neight" ) {
    max_size      = size{ .w = 5, .h = 3 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "not enough width" ) {
    max_size      = size{ .w = 4, .h = 4 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "short on width" ) {
    max_size      = size{ .w = 4, .h = 8 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 5 } );
    REQUIRE( input[4].origin == point{ .x = 0, .y = 6 } );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 6 } );
    REQUIRE( input[6].origin == point{ .x = 0, .y = 7 } );
    REQUIRE( s == size{ .w = 4, .h = 8 } );
  }

  SECTION( "width 3" ) {
    max_size      = size{ .w = 3, .h = 8 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 5 } );
    REQUIRE( input[4].origin == point{ .x = 0, .y = 6 } );
    REQUIRE( input[5].origin == point{ .x = 0, .y = 7 } );
    REQUIRE( input[6].origin == point{ .x = 1, .y = 7 } );
    REQUIRE( s == size{ .w = 3, .h = 8 } );
  }

  SECTION( "width but not enough height" ) {
    max_size      = size{ .w = 3, .h = 7 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "enough height but not width" ) {
    max_size      = size{ .w = 2, .h = 8 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }
}

TEST_CASE( "[render/rect-pack] some vertical bars" ) {
  vector<rect> input;
  auto         add_rect = [&]( size const s ) mutable {
    input.push_back(
                rect{ .origin = { .x = -1, .y = -1 }, .size = s } );
  };

  add_rect( size{ .w = 1, .h = 4 } );
  add_rect( size{ .w = 1, .h = 4 } );
  add_rect( size{ .w = 1, .h = 4 } );
  add_rect( size{ .w = 1, .h = 4 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size      = size{ .w = 100, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].origin == point{ .x = 3, .y = 0 } );
    REQUIRE( s == size{ .w = 4, .h = 4 } );
  }

  SECTION( "short on width" ) {
    max_size      = size{ .w = 3, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( s == size{ .w = 3, .h = 8 } );
  }

  SECTION( "short on width 2" ) {
    max_size      = size{ .w = 2, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].origin == point{ .x = 1, .y = 4 } );
    REQUIRE( s == size{ .w = 2, .h = 8 } );
  }

  SECTION( "short on width 3" ) {
    max_size      = size{ .w = 1, .h = 100 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 8 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 12 } );
    REQUIRE( s == size{ .w = 1, .h = 16 } );
  }

  SECTION( "short on width, just enough height" ) {
    max_size      = size{ .w = 1, .h = 16 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 8 } );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 12 } );
    REQUIRE( s == size{ .w = 1, .h = 16 } );
  }

  SECTION( "short on width 3, short on height" ) {
    max_size      = size{ .w = 1, .h = 15 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }
}

TEST_CASE( "[render/rect-pack] lots o squares" ) {
  vector<rect> input;
  auto         add_rect = [&]( size const s ) mutable {
    input.push_back(
                rect{ .origin = { .x = -1, .y = -1 }, .size = s } );
  };

  // Generate 16*16 2x2 squares.
  for( int i = 0; i < 16 * 16; ++i )
    add_rect( size{ .w = 2, .h = 2 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size      = size{ .w = 32, .h = 32 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    for( int i = 0; i < 16 * 16; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      REQUIRE( input[i].origin == point{ .x = ( i * 2 ) % 32,
                                         .y = ( i / 16 ) * 2 } );
    }
    REQUIRE( s == size{ .w = 32, .h = 32 } );
  }

  SECTION( "not enough width" ) {
    max_size      = size{ .w = 31, .h = 32 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }

  SECTION( "not enough height" ) {
    max_size      = size{ .w = 32, .h = 31 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s == nothing );
  }
}

TEST_CASE( "[render/rect-pack] lots o bars" ) {
  vector<rect> input;
  auto         add_rect = [&]( size const s ) mutable {
    input.push_back(
                rect{ .origin = { .x = -1, .y = -1 }, .size = s } );
  };

  // Generate 10 10x1 rects.
  for( int i = 0; i < 10; ++i )
    add_rect( size{ .w = 10, .h = 1 } );

  size max_size;

  SECTION(
      "space for one bar per line plus a few empty blocks" ) {
    max_size      = size{ .w = 12, .h = 10 };
    maybe<size> s = pack_rects( input, max_size );
    REQUIRE( s.has_value() );
    for( int i = 0; i < 10; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      REQUIRE( input[i].origin == point{ .x = 0, .y = i } );
    }
    REQUIRE( s == size{ .w = 10, .h = 10 } );
  }
}

} // namespace
} // namespace rr
