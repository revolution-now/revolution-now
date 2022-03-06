/****************************************************************
**cartesian.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-02-26.
*
* Description: Unit tests for the src/gfx/cartesian.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/cartesian.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace gfx {
namespace {

using namespace std;

using ::base::nothing;

TEST_CASE( "[gfx/cartesian] size::max_with" ) {
  size s1{ .w = 4, .h = 2 };
  size s2{ .w = 2, .h = 8 };
  REQUIRE( s1.max_with( s2 ) == size{ .w = 4, .h = 8 } );
}

TEST_CASE( "[gfx/cartesian] coord + size" ) {
  point p{ .x = 4, .y = 2 };
  size  s{ .w = 2, .h = 8 };
  REQUIRE( p + s == point{ .x = 6, .y = 10 } );
  REQUIRE( s + p == point{ .x = 6, .y = 10 } );
}

TEST_CASE( "[gfx/cartesian] rect::nw, rect::se, etc." ) {
  rect r;

  REQUIRE( r.nw() == point{} );
  REQUIRE( r.ne() == point{} );
  REQUIRE( r.se() == point{} );
  REQUIRE( r.sw() == point{} );
  REQUIRE( r.top() == 0 );
  REQUIRE( r.bottom() == 0 );
  REQUIRE( r.right() == 0 );
  REQUIRE( r.left() == 0 );

  r = rect{ .origin = { .x = 3, .y = 4 },
            .size   = { .w = 1, .h = 3 } };
  REQUIRE( r.nw() == point{ .x = 3, .y = 4 } );
  REQUIRE( r.ne() == point{ .x = 4, .y = 4 } );
  REQUIRE( r.se() == point{ .x = 4, .y = 7 } );
  REQUIRE( r.sw() == point{ .x = 3, .y = 7 } );
  REQUIRE( r.top() == 4 );
  REQUIRE( r.bottom() == 7 );
  REQUIRE( r.right() == 4 );
  REQUIRE( r.left() == 3 );
}

TEST_CASE(
    "[gfx/cartesian] negative rect::nw, rect::se, etc." ) {
  rect r;

  r = rect{ .origin = { .x = 3, .y = 4 },
            .size   = { .w = -1, .h = -3 } };
  REQUIRE( r.nw() == point{ .x = 2, .y = 1 } );
  REQUIRE( r.ne() == point{ .x = 3, .y = 1 } );
  REQUIRE( r.se() == point{ .x = 3, .y = 4 } );
  REQUIRE( r.sw() == point{ .x = 2, .y = 4 } );
  REQUIRE( r.top() == 1 );
  REQUIRE( r.bottom() == 4 );
  REQUIRE( r.right() == 3 );
  REQUIRE( r.left() == 2 );
}

TEST_CASE( "[gfx/cartesian] rect::is_inside" ) {
  rect r1;
  rect r2{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = 0, .h = 0 } };
  rect r3{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = 1, .h = 0 } };
  rect r4{ .origin = { .x = 2, .y = 2 },
           .size   = { .w = 0, .h = 1 } };
  rect r5{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = 3, .h = 3 } };
  rect r6{ .origin = { .x = 0, .y = 2 },
           .size   = { .w = 4, .h = 3 } };
  rect r7{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = 2, .h = 1 } };
  rect r8{ .origin = { .x = 2, .y = 1 },
           .size   = { .w = 2, .h = 1 } };

  REQUIRE( r1.is_inside( r1 ) == true );
  REQUIRE( r1.is_inside( r2 ) == false );
  REQUIRE( r1.is_inside( r3 ) == false );
  REQUIRE( r1.is_inside( r4 ) == false );
  REQUIRE( r1.is_inside( r5 ) == false );
  REQUIRE( r1.is_inside( r6 ) == false );
  REQUIRE( r1.is_inside( r7 ) == false );
  REQUIRE( r1.is_inside( r8 ) == false );

  REQUIRE( r2.is_inside( r1 ) == false );
  REQUIRE( r2.is_inside( r2 ) == true );
  REQUIRE( r2.is_inside( r3 ) == false );
  REQUIRE( r2.is_inside( r4 ) == false );
  REQUIRE( r2.is_inside( r5 ) == false );
  REQUIRE( r2.is_inside( r6 ) == false );
  REQUIRE( r2.is_inside( r7 ) == true );
  REQUIRE( r2.is_inside( r8 ) == false );

  REQUIRE( r3.is_inside( r1 ) == false );
  REQUIRE( r3.is_inside( r2 ) == false );
  REQUIRE( r3.is_inside( r3 ) == true );
  REQUIRE( r3.is_inside( r4 ) == false );
  REQUIRE( r3.is_inside( r5 ) == true );
  REQUIRE( r3.is_inside( r6 ) == true );
  REQUIRE( r3.is_inside( r7 ) == true );
  REQUIRE( r3.is_inside( r8 ) == false );

  REQUIRE( r4.is_inside( r1 ) == false );
  REQUIRE( r4.is_inside( r2 ) == false );
  REQUIRE( r4.is_inside( r3 ) == false );
  REQUIRE( r4.is_inside( r4 ) == true );
  REQUIRE( r4.is_inside( r5 ) == true );
  REQUIRE( r4.is_inside( r6 ) == true );
  REQUIRE( r4.is_inside( r7 ) == false );
  REQUIRE( r4.is_inside( r8 ) == false );

  REQUIRE( r5.is_inside( r1 ) == false );
  REQUIRE( r5.is_inside( r2 ) == false );
  REQUIRE( r5.is_inside( r3 ) == false );
  REQUIRE( r5.is_inside( r4 ) == false );
  REQUIRE( r5.is_inside( r5 ) == true );
  REQUIRE( r5.is_inside( r6 ) == true );
  REQUIRE( r5.is_inside( r7 ) == false );
  REQUIRE( r5.is_inside( r8 ) == false );

  REQUIRE( r6.is_inside( r1 ) == false );
  REQUIRE( r6.is_inside( r2 ) == false );
  REQUIRE( r6.is_inside( r3 ) == false );
  REQUIRE( r6.is_inside( r4 ) == false );
  REQUIRE( r6.is_inside( r5 ) == false );
  REQUIRE( r6.is_inside( r6 ) == true );
  REQUIRE( r6.is_inside( r7 ) == false );
  REQUIRE( r6.is_inside( r8 ) == false );

  REQUIRE( r7.is_inside( r1 ) == false );
  REQUIRE( r7.is_inside( r2 ) == false );
  REQUIRE( r7.is_inside( r3 ) == false );
  REQUIRE( r7.is_inside( r4 ) == false );
  REQUIRE( r7.is_inside( r5 ) == false );
  REQUIRE( r7.is_inside( r6 ) == false );
  REQUIRE( r7.is_inside( r7 ) == true );
  REQUIRE( r7.is_inside( r8 ) == false );

  REQUIRE( r8.is_inside( r1 ) == false );
  REQUIRE( r8.is_inside( r2 ) == false );
  REQUIRE( r8.is_inside( r3 ) == false );
  REQUIRE( r8.is_inside( r4 ) == false );
  REQUIRE( r8.is_inside( r5 ) == false );
  REQUIRE( r8.is_inside( r6 ) == false );
  REQUIRE( r8.is_inside( r7 ) == false );
  REQUIRE( r8.is_inside( r8 ) == true );
}

TEST_CASE( "[gfx/cartesian] negaitve rect::is_inside" ) {
  rect r1;
  rect r2{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = -0, .h = -0 } };
  rect r3{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = -1, .h = -0 } };
  rect r4{ .origin = { .x = 2, .y = 2 },
           .size   = { .w = -0, .h = -1 } };
  rect r5{ .origin = { .x = 1, .y = 2 },
           .size   = { .w = -3, .h = -3 } };
  rect r6{ .origin = { .x = 0, .y = 2 },
           .size   = { .w = -4, .h = -3 } };
  rect r7{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = -2, .h = -1 } };
  rect r8{ .origin = { .x = 2, .y = 1 },
           .size   = { .w = -2, .h = -1 } };

  REQUIRE( r1.is_inside( r1 ) == true );
  REQUIRE( r1.is_inside( r2 ) == false );
  REQUIRE( r1.is_inside( r3 ) == false );
  REQUIRE( r1.is_inside( r4 ) == false );
  REQUIRE( r1.is_inside( r5 ) == true );
  REQUIRE( r1.is_inside( r6 ) == true );
  REQUIRE( r1.is_inside( r7 ) == true );
  REQUIRE( r1.is_inside( r8 ) == true );

  REQUIRE( r2.is_inside( r1 ) == false );
  REQUIRE( r2.is_inside( r2 ) == true );
  REQUIRE( r2.is_inside( r3 ) == false );
  REQUIRE( r2.is_inside( r4 ) == false );
  REQUIRE( r2.is_inside( r5 ) == true );
  REQUIRE( r2.is_inside( r6 ) == false );
  REQUIRE( r2.is_inside( r7 ) == true );
  REQUIRE( r2.is_inside( r8 ) == true );

  REQUIRE( r3.is_inside( r1 ) == false );
  REQUIRE( r3.is_inside( r2 ) == false );
  REQUIRE( r3.is_inside( r3 ) == true );
  REQUIRE( r3.is_inside( r4 ) == false );
  REQUIRE( r3.is_inside( r5 ) == true );
  REQUIRE( r3.is_inside( r6 ) == false );
  REQUIRE( r3.is_inside( r7 ) == false );
  REQUIRE( r3.is_inside( r8 ) == false );

  REQUIRE( r4.is_inside( r1 ) == false );
  REQUIRE( r4.is_inside( r2 ) == false );
  REQUIRE( r4.is_inside( r3 ) == false );
  REQUIRE( r4.is_inside( r4 ) == true );
  REQUIRE( r4.is_inside( r5 ) == false );
  REQUIRE( r4.is_inside( r6 ) == false );
  REQUIRE( r4.is_inside( r7 ) == false );
  REQUIRE( r4.is_inside( r8 ) == false );

  REQUIRE( r5.is_inside( r1 ) == false );
  REQUIRE( r5.is_inside( r2 ) == false );
  REQUIRE( r5.is_inside( r3 ) == false );
  REQUIRE( r5.is_inside( r4 ) == false );
  REQUIRE( r5.is_inside( r5 ) == true );
  REQUIRE( r5.is_inside( r6 ) == false );
  REQUIRE( r5.is_inside( r7 ) == false );
  REQUIRE( r5.is_inside( r8 ) == false );

  REQUIRE( r6.is_inside( r1 ) == false );
  REQUIRE( r6.is_inside( r2 ) == false );
  REQUIRE( r6.is_inside( r3 ) == false );
  REQUIRE( r6.is_inside( r4 ) == false );
  REQUIRE( r6.is_inside( r5 ) == false );
  REQUIRE( r6.is_inside( r6 ) == true );
  REQUIRE( r6.is_inside( r7 ) == false );
  REQUIRE( r6.is_inside( r8 ) == false );

  REQUIRE( r7.is_inside( r1 ) == false );
  REQUIRE( r7.is_inside( r2 ) == false );
  REQUIRE( r7.is_inside( r3 ) == false );
  REQUIRE( r7.is_inside( r4 ) == false );
  REQUIRE( r7.is_inside( r5 ) == true );
  REQUIRE( r7.is_inside( r6 ) == false );
  REQUIRE( r7.is_inside( r7 ) == true );
  REQUIRE( r7.is_inside( r8 ) == false );

  REQUIRE( r8.is_inside( r1 ) == false );
  REQUIRE( r8.is_inside( r2 ) == false );
  REQUIRE( r8.is_inside( r3 ) == false );
  REQUIRE( r8.is_inside( r4 ) == false );
  REQUIRE( r8.is_inside( r5 ) == false );
  REQUIRE( r8.is_inside( r6 ) == false );
  REQUIRE( r8.is_inside( r7 ) == false );
  REQUIRE( r8.is_inside( r8 ) == true );
}

TEST_CASE( "[gfx/cartesian] rect::contains" ) {
  rect r1;
  rect r2{ .origin = { .x = 1, .y = 1 },
           .size   = { .w = 2, .h = 1 } };

  REQUIRE( r1.contains( point{} ) == true );
  REQUIRE( r1.contains( point{ .x = 1, .y = 0 } ) == false );
  REQUIRE( r1.contains( point{ .x = 0, .y = 1 } ) == false );
  REQUIRE( r1.contains( point{ .x = 1, .y = 1 } ) == false );

  REQUIRE( r2.contains( point{ .x = 0, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 2, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 3, .y = 0 } ) == false );
  REQUIRE( r2.contains( point{ .x = 0, .y = 1 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 1 } ) == true );
  REQUIRE( r2.contains( point{ .x = 2, .y = 1 } ) == true );
  REQUIRE( r2.contains( point{ .x = 3, .y = 1 } ) == true );
  REQUIRE( r2.contains( point{ .x = 0, .y = 2 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 2 } ) == true );
  REQUIRE( r2.contains( point{ .x = 2, .y = 2 } ) == true );
  REQUIRE( r2.contains( point{ .x = 3, .y = 2 } ) == true );
  REQUIRE( r2.contains( point{ .x = 0, .y = 3 } ) == false );
  REQUIRE( r2.contains( point{ .x = 1, .y = 3 } ) == false );
  REQUIRE( r2.contains( point{ .x = 2, .y = 3 } ) == false );
  REQUIRE( r2.contains( point{ .x = 3, .y = 3 } ) == false );
}

TEST_CASE( "[gfx/cartesian] rect::clipped_by" ) {
  rect expected, r1, r2;

  r1       = rect{ .origin = { .x = -1, .y = 0 },
                   .size   = { .w = 3, .h = 3 } };
  r2       = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 5, .h = 7 } };
  expected = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 3 } };
  r2       = rect{ .origin = { .x = 0, .y = 1 },
                   .size   = { .w = 5, .h = 7 } };
  expected = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 3 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1 = rect{ .origin = { .x = 2, .y = 2 },
             .size   = { .w = 2, .h = 2 } };
  r2 = rect{ .origin = { .x = 4, .y = 2 },
             .size   = { .w = 2, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == nothing );

  r1 = rect{ .origin = { .x = 2, .y = 2 },
             .size   = { .w = 2, .h = 2 } };
  r2 = rect{ .origin = { .x = 6, .y = 2 },
             .size   = { .w = 2, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == nothing );

  r1       = rect{ .origin = { .x = 2, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  r2       = rect{ .origin = { .x = 3, .y = 2 },
                   .size   = { .w = 2, .h = 2 } };
  expected = rect{ .origin = { .x = 3, .y = 2 },
                   .size   = { .w = 1, .h = 2 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );

  r1       = rect{ .origin = { .x = 0, .y = 0 },
                   .size   = { .w = 5, .h = 10 } };
  r2       = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 1, .h = 1 } };
  expected = rect{ .origin = { .x = 1, .y = 1 },
                   .size   = { .w = 1, .h = 1 } };
  REQUIRE( r1.clipped_by( r2 ) == expected );
}

} // namespace
} // namespace gfx
