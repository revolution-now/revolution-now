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

// Must be last.
#include "test/catch-common.hpp"

namespace rr {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::size;

TEST_CASE( "[render/rect-pack] empty" ) {
  vector<rect_to_pack> input;
  size                 max_size;

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 0, .h = 0 },
                            .area_occupied = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 0 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 0, .h = 0 },
                            .area_occupied = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 0, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 0, .h = 0 },
                            .area_occupied = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 0, .h = 0 },
                            .area_occupied = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 0, .h = 0 },
                            .area_occupied = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 1x1" ) {
  vector<rect_to_pack> input = {
      rect_to_pack{ .size   = { .w = 1, .h = 1 },
                    .origin = { .x = -1, .y = -1 } } };
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
             packing_stats{ .size_used     = { .w = 1, .h = 1 },
                            .area_occupied = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 1 },
                            .area_occupied = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 1 },
                            .area_occupied = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 1 },
                            .area_occupied = 1 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 1x2" ) {
  vector<rect_to_pack> input = {
      rect_to_pack{ .size   = { .w = 1, .h = 2 },
                    .origin = { .x = -1, .y = -1 } } };
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
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 2 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 2 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 4 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 2 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 4 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 1, .h = 2 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 2x1" ) {
  vector<rect_to_pack> input = {
      rect_to_pack{ .size   = { .w = 2, .h = 1 },
                    .origin = { .x = -1, .y = -1 } } };
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
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 1, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) == nothing );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 2, .h = 1 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 2, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 2, .h = 1 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 4, .h = 1 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 2, .h = 1 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }

  SECTION( "" ) {
    max_size = size{ .w = 4, .h = 2 };
    REQUIRE( pack_rects( input, max_size ) ==
             packing_stats{ .size_used     = { .w = 2, .h = 1 },
                            .area_occupied = 2 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] single square 2x2" ) {
  vector<rect_to_pack> input = {
      rect_to_pack{ .size   = { .w = 2, .h = 2 },
                    .origin = { .x = -1, .y = -1 } } };
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
             packing_stats{ .size_used     = { .w = 2, .h = 2 },
                            .area_occupied = 4 } );
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
             packing_stats{ .size_used     = { .w = 2, .h = 2 },
                            .area_occupied = 4 } );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
  }
}

TEST_CASE( "[render/rect-pack] multiple 1" ) {
  vector<rect_to_pack> input;
  auto add_rect = [&]( int id, size const& s ) mutable {
    input.push_back( rect_to_pack{
        .id = id, .size = s, .origin = { .x = -1, .y = -1 } } );
  };

  add_rect( /*id=*/0, size{ .w = 2, .h = 1 } );
  add_rect( /*id=*/1, size{ .w = 2, .h = 2 } );
  add_rect( /*id=*/2, size{ .w = 1, .h = 1 } );
  add_rect( /*id=*/3, size{ .w = 8, .h = 4 } );
  add_rect( /*id=*/4, size{ .w = 4, .h = 4 } );
  add_rect( /*id=*/5, size{ .w = 1, .h = 1 } );
  add_rect( /*id=*/6, size{ .w = 1, .h = 1 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size                   = size{ .w = 100, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 15, .h = 4 },
                            .area_occupied = 57 } );
  }

  SECTION( "largest block doesn't have the height" ) {
    max_size                   = size{ .w = 100, .h = 3 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "just enough width in first row" ) {
    max_size                   = size{ .w = 15, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 15, .h = 4 },
                            .area_occupied = 57 } );
  }

  SECTION( "just enough width and height for one row" ) {
    max_size                   = size{ .w = 15, .h = 4 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 15, .h = 4 },
                            .area_occupied = 57 } );
  }

  SECTION( "enough width in first row - 1" ) {
    max_size                   = size{ .w = 14, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 12, .y = 3 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 13, .y = 3 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 14, .h = 5 },
                            .area_occupied = 57 } );
  }

  SECTION(
      "enough width in first row - 1, but not enough height" ) {
    max_size                   = size{ .w = 14, .h = 4 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "only large blocks fit in first row" ) {
    max_size                   = size{ .w = 13, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 4 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 5 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 5 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 12, .h = 6 },
                            .area_occupied = 57 } );
  }

  SECTION( "only large blocks fit in first row 2" ) {
    max_size                   = size{ .w = 12, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 4 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 5 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 5 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 12, .h = 6 },
                            .area_occupied = 57 } );
  }

  SECTION(
      "only large blocks fit in first row, just enough "
      "height" ) {
    max_size                   = size{ .w = 12, .h = 6 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 4 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 5 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 5 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 12, .h = 6 },
                            .area_occupied = 57 } );
  }

  SECTION(
      "only large blocks fit in first row, not enough height" ) {
    max_size                   = size{ .w = 12, .h = 5 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "only largest block fits in first row" ) {
    max_size                   = size{ .w = 11, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 4, .y = 6 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 4, .y = 7 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 5, .y = 7 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 6, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 8, .h = 8 },
                            .area_occupied = 57 } );
  }

  SECTION( "largest block just fits in first row" ) {
    max_size                   = size{ .w = 8, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 4, .y = 4 } );
    REQUIRE( input[3].id == 0 );
    REQUIRE( input[3].origin == point{ .x = 4, .y = 6 } );
    REQUIRE( input[4].id == 2 );
    REQUIRE( input[4].origin == point{ .x = 4, .y = 7 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 5, .y = 7 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 6, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 8, .h = 8 },
                            .area_occupied = 57 } );
  }

  SECTION( "largest block just fits in first row" ) {
    max_size                   = size{ .w = 7, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }
}

TEST_CASE( "[render/rect-pack] multiple 2" ) {
  vector<rect_to_pack> input;
  auto add_rect = [&]( int id, size const& s ) mutable {
    input.push_back( rect_to_pack{
        .id = id, .size = s, .origin = { .x = -1, .y = -1 } } );
  };

  add_rect( /*id=*/0, size{ .w = 2, .h = 1 } );
  add_rect( /*id=*/1, size{ .w = 2, .h = 2 } );
  add_rect( /*id=*/2, size{ .w = 1, .h = 1 } );
  add_rect( /*id=*/3, size{ .w = 8, .h = 4 } );
  add_rect( /*id=*/4, size{ .w = 4, .h = 4 } );
  add_rect( /*id=*/5, size{ .w = 1, .h = 1 } );
  add_rect( /*id=*/6, size{ .w = 1, .h = 2 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size                   = size{ .w = 100, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 3 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 4 );
    REQUIRE( input[1].origin == point{ .x = 8, .y = 0 } );
    REQUIRE( input[2].id == 1 );
    REQUIRE( input[2].origin == point{ .x = 12, .y = 0 } );
    REQUIRE( input[3].id == 6 );
    REQUIRE( input[3].origin == point{ .x = 12, .y = 2 } );
    REQUIRE( input[4].id == 0 );
    REQUIRE( input[4].origin == point{ .x = 14, .y = 0 } );
    REQUIRE( input[5].id == 2 );
    REQUIRE( input[5].origin == point{ .x = 14, .y = 1 } );
    REQUIRE( input[6].id == 5 );
    REQUIRE( input[6].origin == point{ .x = 15, .y = 1 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 16, .h = 4 },
                            .area_occupied = 58 } );
  }
}

TEST_CASE( "[render/rect-pack] multiple 3" ) {
  vector<rect_to_pack> input;
  auto add_rect = [&]( int id, size const& s ) mutable {
    input.push_back( rect_to_pack{
        .id = id, .size = s, .origin = { .x = -1, .y = -1 } } );
  };

  add_rect( /*id=*/0, size{ .w = 1, .h = 4 } );
  add_rect( /*id=*/1, size{ .w = 1, .h = 2 } );
  add_rect( /*id=*/2, size{ .w = 3, .h = 1 } );
  add_rect( /*id=*/3, size{ .w = 2, .h = 1 } );
  add_rect( /*id=*/4, size{ .w = 3, .h = 1 } );
  add_rect( /*id=*/5, size{ .w = 1, .h = 1 } );
  add_rect( /*id=*/6, size{ .w = 1, .h = 1 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size                   = size{ .w = 10, .h = 10 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 1 } );
    REQUIRE( input[4].id == 4 );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 2 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 2, .y = 3 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 3, .y = 3 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 5, .h = 4 },
                            .area_occupied = 16 } );
  }

  SECTION( "just enough space" ) {
    max_size                   = size{ .w = 5, .h = 4 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 2, .y = 1 } );
    REQUIRE( input[4].id == 4 );
    REQUIRE( input[4].origin == point{ .x = 2, .y = 2 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 2, .y = 3 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 3, .y = 3 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 5, .h = 4 },
                            .area_occupied = 16 } );
  }

  SECTION( "not enough neight" ) {
    max_size                   = size{ .w = 5, .h = 3 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "not enough width" ) {
    max_size                   = size{ .w = 4, .h = 4 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "short on width" ) {
    max_size                   = size{ .w = 4, .h = 8 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 5 } );
    REQUIRE( input[4].id == 4 );
    REQUIRE( input[4].origin == point{ .x = 0, .y = 6 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 3, .y = 6 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 0, .y = 7 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 4, .h = 8 },
                            .area_occupied = 16 } );
  }

  SECTION( "width 3" ) {
    max_size                   = size{ .w = 3, .h = 8 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 5 } );
    REQUIRE( input[4].id == 4 );
    REQUIRE( input[4].origin == point{ .x = 0, .y = 6 } );
    REQUIRE( input[5].id == 5 );
    REQUIRE( input[5].origin == point{ .x = 0, .y = 7 } );
    REQUIRE( input[6].id == 6 );
    REQUIRE( input[6].origin == point{ .x = 1, .y = 7 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 3, .h = 8 },
                            .area_occupied = 16 } );
  }

  SECTION( "width but not enough height" ) {
    max_size                   = size{ .w = 3, .h = 7 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "enough height but not width" ) {
    max_size                   = size{ .w = 2, .h = 8 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }
}

TEST_CASE( "[render/rect-pack] some vertical bars" ) {
  vector<rect_to_pack> input;
  auto add_rect = [&]( int id, size const& s ) mutable {
    input.push_back( rect_to_pack{
        .id = id, .size = s, .origin = { .x = -1, .y = -1 } } );
  };

  add_rect( /*id=*/0, size{ .w = 1, .h = 4 } );
  add_rect( /*id=*/1, size{ .w = 1, .h = 4 } );
  add_rect( /*id=*/2, size{ .w = 1, .h = 4 } );
  add_rect( /*id=*/3, size{ .w = 1, .h = 4 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size                   = size{ .w = 100, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 3, .y = 0 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 4, .h = 4 },
                            .area_occupied = 16 } );
  }

  SECTION( "short on width" ) {
    max_size                   = size{ .w = 3, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 2, .y = 0 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 3, .h = 8 },
                            .area_occupied = 16 } );
  }

  SECTION( "short on width 2" ) {
    max_size                   = size{ .w = 2, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 1, .y = 0 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 1, .y = 4 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 2, .h = 8 },
                            .area_occupied = 16 } );
  }

  SECTION( "short on width 3" ) {
    max_size                   = size{ .w = 1, .h = 100 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 8 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 12 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 1, .h = 16 },
                            .area_occupied = 16 } );
  }

  SECTION( "short on width, just enough height" ) {
    max_size                   = size{ .w = 1, .h = 16 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    REQUIRE( input[0].id == 0 );
    REQUIRE( input[0].origin == point{ .x = 0, .y = 0 } );
    REQUIRE( input[1].id == 1 );
    REQUIRE( input[1].origin == point{ .x = 0, .y = 4 } );
    REQUIRE( input[2].id == 2 );
    REQUIRE( input[2].origin == point{ .x = 0, .y = 8 } );
    REQUIRE( input[3].id == 3 );
    REQUIRE( input[3].origin == point{ .x = 0, .y = 12 } );
    REQUIRE( stats ==
             packing_stats{ .size_used     = { .w = 1, .h = 16 },
                            .area_occupied = 16 } );
  }

  SECTION( "short on width 3, short on height" ) {
    max_size                   = size{ .w = 1, .h = 15 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }
}

TEST_CASE( "[render/rect-pack] lots o squares" ) {
  vector<rect_to_pack> input;
  auto add_rect = [&]( int id, size const& s ) mutable {
    input.push_back( rect_to_pack{
        .id = id, .size = s, .origin = { .x = -1, .y = -1 } } );
  };

  // Generate 16*16 2x2 squares.
  for( int i = 0; i < 16 * 16; ++i )
    add_rect( /*id=*/i, size{ .w = 2, .h = 2 } );

  size max_size;

  SECTION( "ample space" ) {
    max_size                   = size{ .w = 32, .h = 32 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    for( int i = 0; i < 16 * 16; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      REQUIRE( input[i].id == i );
      REQUIRE( input[i].origin == point{ .x = ( i * 2 ) % 32,
                                         .y = ( i / 16 ) * 2 } );
    }
    REQUIRE( stats ==
             packing_stats{ .size_used = { .w = 32, .h = 32 },
                            .area_occupied = 16 * 16 * 2 * 2 } );
  }

  SECTION( "not enough width" ) {
    max_size                   = size{ .w = 31, .h = 32 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }

  SECTION( "not enough height" ) {
    max_size                   = size{ .w = 32, .h = 31 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats == nothing );
  }
}

TEST_CASE( "[render/rect-pack] lots o bars" ) {
  vector<rect_to_pack> input;
  auto add_rect = [&]( int id, size const& s ) mutable {
    input.push_back( rect_to_pack{
        .id = id, .size = s, .origin = { .x = -1, .y = -1 } } );
  };

  // Generate 10 10x1 rects.
  for( int i = 0; i < 10; ++i )
    add_rect( /*id=*/i, size{ .w = 10, .h = 1 } );

  size max_size;

  SECTION(
      "space for one bar per line plus a few empty blocks" ) {
    max_size                   = size{ .w = 12, .h = 10 };
    maybe<packing_stats> stats = pack_rects( input, max_size );
    REQUIRE( stats.has_value() );
    for( int i = 0; i < 10; ++i ) {
      INFO( fmt::format( "i={}", i ) );
      REQUIRE( input[i].id == i );
      REQUIRE( input[i].origin == point{ .x = 0, .y = i } );
    }
    REQUIRE( stats ==
             packing_stats{ .size_used = { .w = 10, .h = 10 },
                            .area_occupied = 100 } );
  }
}

} // namespace
} // namespace rr
