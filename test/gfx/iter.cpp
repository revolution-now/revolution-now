/****************************************************************
**iter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-16.
*
* Description: Unit tests for the src/gfx/iter.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/iter.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

TEST_CASE( "[gfx/iter] subrects even multiple" ) {
  rect                  r = rect{ .origin = { .x = 64, .y = 32 },
                                  .size = { .w = 32 * 3, .h = 32 * 2 } };
  rect                  expected;
  base::generator<rect> iterable =
      subrects( r, size{ .w = 32, .h = 32 } );

  auto it = iterable.begin();
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64, .y = 32 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 1, .y = 32 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 2, .y = 32 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 0, .y = 64 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 1, .y = 64 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 2, .y = 64 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it == iterable.end() );
}

TEST_CASE( "[gfx/iter] subrects non-even multiple" ) {
  // Add a little bit extra in each dimension to make sure that
  // this works for the case that it is not an even multiple of
  // the chunk size.
  rect                  r = rect{ .origin = { .x = 64, .y = 32 },
                                  .size = { .w = 32 * 3 + 3, .h = 32 * 2 + 4 } };
  rect                  expected;
  base::generator<rect> iterable =
      subrects( r, size{ .w = 32, .h = 32 } );

  auto it = iterable.begin();
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64, .y = 32 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 1, .y = 32 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 2, .y = 32 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 3, .y = 32 },
               .size   = { .w = 3, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 0, .y = 64 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 1, .y = 64 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 2, .y = 64 },
               .size   = { .w = 32, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 3, .y = 64 },
               .size   = { .w = 3, .h = 32 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 0, .y = 96 },
               .size   = { .w = 32, .h = 4 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 1, .y = 96 },
               .size   = { .w = 32, .h = 4 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 2, .y = 96 },
               .size   = { .w = 32, .h = 4 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it != iterable.end() );
  expected = { .origin = { .x = 64 + 32 * 3, .y = 96 },
               .size   = { .w = 3, .h = 4 } };
  REQUIRE( *it == expected );

  ++it;
  REQUIRE( it == iterable.end() );
}

TEST_CASE( "[gfx/iter] rect_iterator" ) {
  SECTION( "0x0" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 0, .h = 0 } };
    rect_iterator ri( r );

    auto it = ri.begin();
    REQUIRE( it == ri.end() );
  }

  SECTION( "0x1" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 0, .h = 1 } };
    rect_iterator ri( r );

    auto it = ri.begin();
    REQUIRE( it == ri.end() );
  }

  SECTION( "0x2" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 0, .h = 2 } };
    rect_iterator ri( r );

    auto it = ri.begin();
    REQUIRE( it == ri.end() );
  }

  SECTION( "1x0" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 1, .h = 0 } };
    rect_iterator ri( r );

    auto it = ri.begin();
    REQUIRE( it == ri.end() );
  }

  SECTION( "2x0" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 2, .h = 0 } };
    rect_iterator ri( r );

    auto it = ri.begin();
    REQUIRE( it == ri.end() );
  }

  SECTION( "1x1" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 1, .h = 1 } };
    rect_iterator ri( r );
    using C = rn::Coord;

    auto it = ri.begin();
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 3 } );
    ++it;
    REQUIRE( it == ri.end() );
  }

  SECTION( "2x2" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 2, .h = 2 } };
    rect_iterator ri( r );
    using C = rn::Coord;

    auto it = ri.begin();
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 4 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 4 } );
    ++it;
    REQUIRE( it == ri.end() );
  }

  SECTION( "4x2" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 4, .h = 2 } };
    rect_iterator ri( r );
    using C = rn::Coord;

    auto it = ri.begin();
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 4, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 5, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 4 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 4 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 4, .y = 4 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 5, .y = 4 } );
    ++it;
    REQUIRE( it == ri.end() );
  }

  SECTION( "2x4" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 2, .h = 4 } };
    rect_iterator ri( r );
    using C = rn::Coord;

    auto it = ri.begin();
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 3 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 4 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 4 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 5 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 5 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 6 } );
    ++it;
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 6 } );
    ++it;
    REQUIRE( it == ri.end() );
  }

  SECTION( "postfix++" ) {
    rect const    r = { .origin = { .x = 2, .y = 3 },
                        .size   = { .w = 2, .h = 2 } };
    rect_iterator ri( r );
    using C = rn::Coord;

    auto it = ri.begin();
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 3 } );
    REQUIRE( it++ != ri.end() );
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 3 } );
    REQUIRE( it++ != ri.end() );
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 2, .y = 4 } );
    REQUIRE( it++ != ri.end() );
    REQUIRE( it != ri.end() );
    REQUIRE( *it == C{ .x = 3, .y = 4 } );
    REQUIRE( it++ != ri.end() );
    REQUIRE( it == ri.end() );
  }
}

} // namespace
} // namespace gfx
