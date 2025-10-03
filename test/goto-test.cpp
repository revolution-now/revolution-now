/****************************************************************
**goto-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-26.
*
* Description: Unit tests for the goto module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/goto.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/goto-viewer.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/unit-composition.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::gfx::point;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    // No map creation here by default.
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[goto] compute_goto_path" ) {
  world w;
  point src, dst;
  GotoPath expected;

  VisibilityEntire const viz_entire( w.ss() );
  VisibilityForPlayer const viz_player(
      w.ss(), w.default_player_type() );

  using enum e_surface;
  using enum e_ground_terrain;
  using enum e_land_overlay;
  using enum e_river;

  using MS = MapSquare;

  auto const f =
      [&] [[clang::noinline]] ( IGotoMapViewer const& viewer ) {
        return compute_goto_path( viewer, src, dst );
      };

  SECTION( "map 1" ) {
    // NOTE: the below was generated using lua/capture/map.lua
    // clang-format off
    MS const A{.surface=land,.ground=arctic,.road=true};
    MS const H{.surface=land,.ground=grassland,.overlay=hills,.road=true};
    MS const M{.surface=land,.ground=grassland,.overlay=mountains,.road=true};
    MS const _{.surface=water,.ground=arctic};
    MS const a{.surface=land,.ground=arctic};
    MS const b{.surface=land,.ground=prairie,.overlay=forest};
    MS const c{.surface=land,.ground=plains};
    MS const d{.surface=land,.ground=desert,.overlay=forest,.road=true};
    MS const e{.surface=land,.ground=marsh,.overlay=forest,.road=true};
    MS const i{.surface=land,.ground=prairie,.overlay=forest,.road=true};
    MS const j{.surface=land,.ground=marsh,.overlay=forest};
    MS const k{.surface=land,.ground=plains,.overlay=forest};
    MS const l{.surface=land,.ground=plains,.overlay=forest,.road=true};
    MS const m{.surface=land,.ground=grassland,.overlay=mountains};
    MS const s{.surface=land,.ground=savannah};
    MS const x{.surface=water,.ground=arctic,.sea_lane=true};
    // clang-format on

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ x,a,a,_,_,a,_,_,_,x, /*0
      1*/ x,_,_,_,_,_,_,_,_,x, /*1
      2*/ x,_,_,_,b,b,c,_,_,x, /*2
      3*/ x,_,_,_,H,d,_,_,_,x, /*3
      4*/ x,_,e,i,j,i,_,s,_,x, /*4
      5*/ x,_,i,m,j,s,s,s,s,x, /*5
      6*/ x,_,M,s,s,s,s,s,s,x, /*6
      7*/ x,k,M,s,s,s,s,s,s,x, /*7
      8*/ x,_,l,_,_,_,_,_,_,x, /*8
      9*/ x,_,A,a,a,_,_,_,a,x, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );
    // NOTE: end generated code.

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::free_colonist );

    // path: island
    src      = { .x = 8, .y = 9 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta         = { .tiles_touched     = 1,
                                   .iterations        = 1,
                                   .queue_size_at_end = 0 },
                 .reverse_path = {} };
    REQUIRE( f( viewer ) == expected );

    // path: top to bottom
    src      = { .x = 6, .y = 2 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 36,
                           .iterations        = 30,
                           .queue_size_at_end = 6 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 3, .y = 9 },
                   { .x = 2, .y = 8 },
                   { .x = 2, .y = 7 },
                   { .x = 2, .y = 6 },
                   { .x = 2, .y = 5 },
                   { .x = 3, .y = 4 },
                   { .x = 4, .y = 3 },
                   { .x = 5, .y = 3 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // path: bottom to top
    src      = { .x = 4, .y = 9 };
    dst      = { .x = 6, .y = 2 };
    expected = { .meta = { .tiles_touched     = 25,
                           .iterations        = 18,
                           .queue_size_at_end = 8 },

                 .reverse_path = {
                   { .x = 6, .y = 2 },
                   { .x = 5, .y = 3 },
                   { .x = 4, .y = 3 },
                   { .x = 3, .y = 4 },
                   { .x = 2, .y = 5 },
                   { .x = 2, .y = 6 },
                   { .x = 2, .y = 7 },
                   { .x = 2, .y = 8 },
                   { .x = 3, .y = 9 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // path: bottom right to left.
    src      = { .x = 8, .y = 7 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 36,
                           .iterations        = 38,
                           .queue_size_at_end = 0 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 3, .y = 9 },
                   { .x = 2, .y = 8 },
                   { .x = 2, .y = 7 },
                   { .x = 2, .y = 6 },
                   { .x = 2, .y = 5 },
                   { .x = 3, .y = 4 },
                   { .x = 4, .y = 3 },
                   { .x = 5, .y = 4 },
                   { .x = 6, .y = 5 },
                   { .x = 7, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // path: left to bottom right.
    src      = { .x = 4, .y = 9 };
    dst      = { .x = 8, .y = 7 };
    expected = { .meta = { .tiles_touched     = 36,
                           .iterations        = 32,
                           .queue_size_at_end = 4 },

                 .reverse_path = {
                   { .x = 8, .y = 7 },
                   { .x = 7, .y = 6 },
                   { .x = 6, .y = 5 },
                   { .x = 5, .y = 4 },
                   { .x = 4, .y = 3 },
                   { .x = 3, .y = 4 },
                   { .x = 2, .y = 5 },
                   { .x = 2, .y = 6 },
                   { .x = 2, .y = 7 },
                   { .x = 2, .y = 8 },
                   { .x = 3, .y = 9 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "map 2" ) {
    // NOTE: the below was generated using lua/capture/map.lua
    // clang-format off
    MS const M{.surface=land,.ground=grassland,.overlay=mountains,.road=true};
    MS const _{.surface=water,.ground=arctic};
    MS const b{.surface=land,.ground=prairie,.overlay=forest};
    MS const c{.surface=land,.ground=tundra,.overlay=forest};
    MS const d{.surface=land,.ground=plains,.overlay=forest};
    MS const e{.surface=water,.ground=grassland};
    MS const h{.surface=land,.ground=grassland,.overlay=hills};
    MS const i{.surface=land,.ground=tundra,.overlay=forest,.road=true};
    MS const j{.surface=land,.ground=grassland,.overlay=forest};
    MS const k{.surface=land,.ground=plains,.overlay=forest,.road=true};
    MS const l{.surface=water,.ground=arctic,.river=minor};
    MS const m{.surface=land,.ground=grassland,.overlay=mountains};
    MS const n{.surface=land,.ground=prairie,.overlay=forest,.river=minor};
    MS const o{.surface=land,.ground=desert,.overlay=forest};
    MS const p{.surface=land,.ground=prairie};
    // clang-format on

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9 a b
      0*/ b,c,_,_,_,_,_,_,_,_,_,_, /*0
      1*/ m,m,_,_,_,_,_,_,d,_,_,_, /*1
      2*/ _,_,e,_,_,_,M,b,i,d,_,_, /*2
      3*/ _,j,c,b,k,m,m,_,b,l,n,_, /*3
      4*/ _,c,d,b,c,m,m,h,b,_,_,_, /*4
      5*/ _,_,b,o,p,_,h,m,c,_,_,_, /*5
      6*/ _,_,_,d,j,_,c,_,_,_,_,_, /*6
      7*/ _,_,_,_,_,_,_,_,_,_,_,_, /*7
          0 1 2 3 4 5 6 7 8 9 a b
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 12 );
    // NOTE: end generated code.

    w.add_colony( { .x = 4, .y = 3 } );

    SECTION( "movement 1 unit" ) {
      GotoMapViewer const viewer( w.ss(), viz_entire,
                                  w.default_player_type(),
                                  e_unit_type::free_colonist );

      src      = { .x = 4, .y = 3 };
      dst      = { .x = 10, .y = 3 };
      expected = { .meta = { .tiles_touched     = 30,
                             .iterations        = 30,
                             .queue_size_at_end = 0 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 3 },
                     { .x = 7, .y = 2 },
                     { .x = 6, .y = 3 },
                     { .x = 5, .y = 3 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 20,
                             .iterations        = 17,
                             .queue_size_at_end = 3 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 3 },
                     { .x = 6, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 8, .y = 1 },
                     { .x = 9, .y = 2 },
                   } };
      REQUIRE( f( viewer ) == expected );
    }

    SECTION( "movement 4 unit" ) {
      GotoMapViewer const viewer( w.ss(), viz_entire,
                                  w.default_player_type(),
                                  e_unit_type::dragoon );

      src      = { .x = 4, .y = 3 };
      dst      = { .x = 10, .y = 3 };
      expected = { .meta = { .tiles_touched     = 30,
                             .iterations        = 30,
                             .queue_size_at_end = 0 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 3 },
                     { .x = 7, .y = 4 },
                     { .x = 6, .y = 5 },
                     { .x = 5, .y = 4 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 24,
                             .iterations        = 19,
                             .queue_size_at_end = 5 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 4 },
                     { .x = 6, .y = 5 },
                     { .x = 7, .y = 4 },
                     { .x = 8, .y = 3 },
                     { .x = 9, .y = 2 },
                   } };
      REQUIRE( f( viewer ) == expected );
    }
  }

  // This one is essentially the same as map 2 except with a few
  // extra roads placed at specific places to alter the path.
  SECTION( "map 3" ) {
    // NOTE: the below was generated using lua/capture/map.lua
    // clang-format off
    MS const _{.surface=water,.ground=arctic};
    MS const b{.surface=land,.ground=prairie,.overlay=forest};
    MS const c{.surface=land,.ground=tundra,.overlay=forest};
    MS const d{.surface=land,.ground=plains,.overlay=forest};
    MS const e{.surface=water,.ground=grassland};
    MS const h{.surface=land,.ground=grassland,.overlay=hills};
    MS const i{.surface=land,.ground=prairie,.overlay=forest,.road=true};
    MS const j{.surface=land,.ground=tundra,.overlay=forest,.road=true};
    MS const k{.surface=land,.ground=grassland,.overlay=forest};
    MS const l{.surface=land,.ground=plains,.overlay=forest,.road=true};
    MS const m{.surface=land,.ground=grassland,.overlay=mountains};
    MS const n{.surface=water,.ground=arctic,.river=minor};
    MS const o{.surface=land,.ground=prairie,.overlay=forest,.river=minor};
    MS const p{.surface=land,.ground=desert,.overlay=forest};
    MS const q{.surface=land,.ground=prairie};
    // clang-format on

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9 a b
      0*/ b,c,_,_,_,_,_,_,_,_,_,_, /*0
      1*/ m,m,_,_,_,_,_,_,d,_,_,_, /*1
      2*/ _,_,e,_,_,_,m,i,j,d,_,_, /*2
      3*/ _,k,c,b,l,m,m,_,b,n,o,_, /*3
      4*/ _,c,d,b,c,m,m,h,b,_,_,_, /*4
      5*/ _,_,b,p,q,_,h,m,c,_,_,_, /*5
      6*/ _,_,_,d,k,_,c,_,_,_,_,_, /*6
      7*/ _,_,_,_,_,_,_,_,_,_,_,_, /*7
          0 1 2 3 4 5 6 7 8 9 a b
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 12 );
    // NOTE: end generated code.

    w.add_colony( { .x = 4, .y = 3 } );

    SECTION( "movement 1 unit" ) {
      GotoMapViewer const viewer( w.ss(), viz_entire,
                                  w.default_player_type(),
                                  e_unit_type::free_colonist );

      src      = { .x = 4, .y = 3 };
      dst      = { .x = 10, .y = 3 };
      expected = { .meta = { .tiles_touched     = 30,
                             .iterations        = 27,
                             .queue_size_at_end = 3 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 6, .y = 3 },
                     { .x = 5, .y = 3 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 20,
                             .iterations        = 17,
                             .queue_size_at_end = 4 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 3 },
                     { .x = 6, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 9, .y = 2 },
                   } };
      REQUIRE( f( viewer ) == expected );
    }

    SECTION( "movement 4 unit" ) {
      GotoMapViewer const viewer( w.ss(), viz_entire,
                                  w.default_player_type(),
                                  e_unit_type::dragoon );

      src      = { .x = 4, .y = 3 };
      dst      = { .x = 10, .y = 3 };
      expected = { .meta = { .tiles_touched     = 30,
                             .iterations        = 30,
                             .queue_size_at_end = 0 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 6, .y = 3 },
                     { .x = 5, .y = 4 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 24,
                             .iterations        = 20,
                             .queue_size_at_end = 5 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 3 },
                     { .x = 6, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 9, .y = 2 },
                   } };
      REQUIRE( f( viewer ) == expected );
    }
  }
}

TEST_CASE( "[goto] compute_harbor_goto_path" ) {
  world w;
}

TEST_CASE( "[goto] unit_has_reached_goto_target" ) {
  world w;
}

TEST_CASE( "[goto] find_goto_port" ) {
  world w;
}

TEST_CASE( "[goto] ask_goto_port" ) {
  world w;
}

} // namespace
} // namespace rn
