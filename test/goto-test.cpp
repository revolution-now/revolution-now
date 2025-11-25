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
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/goto-registry.hpp"
#include "src/goto-viewer.hpp"
#include "src/harbor-units.hpp"
#include "src/imap-updater.hpp"
#include "src/tribe-mgr.hpp"
#include "src/unit-ownership.hpp"
#include "src/visibility.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/goto.rds.hpp"
#include "src/ss/land-view.rds.hpp"
#include "src/ss/map-square.rds.hpp"
#include "src/ss/player.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/units.hpp"

// gfx
#include "src/gfx/iter.hpp"

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
using ::gfx::rect_iterator;
using ::mock::matchers::_;
using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::dutch );
    set_default_player_type( e_player::dutch );
    add_player( e_player::french );
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
                           .iterations        = 34,
                           .queue_size_at_end = 9 },

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
                           .iterations        = 19,
                           .queue_size_at_end = 9 },

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
                           .iterations        = 48,
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
                           .iterations        = 35,
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
                             .iterations        = 39,
                             .queue_size_at_end = 1 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 6, .y = 2 },
                     { .x = 5, .y = 3 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 20,
                             .iterations        = 19,
                             .queue_size_at_end = 4 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 3 },
                     { .x = 6, .y = 3 },
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
                             .iterations        = 36,
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
                             .iterations        = 21,
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
                             .iterations        = 36,
                             .queue_size_at_end = 4 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 6, .y = 2 },
                     { .x = 5, .y = 3 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 20,
                             .iterations        = 18,
                             .queue_size_at_end = 6 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 3 },
                     { .x = 6, .y = 3 },
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
                             .iterations        = 36,
                             .queue_size_at_end = 0 },

                   .reverse_path = {
                     { .x = 10, .y = 3 },
                     { .x = 9, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 7, .y = 2 },
                     { .x = 6, .y = 2 },
                     { .x = 5, .y = 3 },
                   } };
      REQUIRE( f( viewer ) == expected );

      src      = { .x = 10, .y = 3 };
      dst      = { .x = 4, .y = 3 };
      expected = { .meta = { .tiles_touched     = 24,
                             .iterations        = 21,
                             .queue_size_at_end = 6 },

                   .reverse_path = {
                     { .x = 4, .y = 3 },
                     { .x = 5, .y = 3 },
                     { .x = 6, .y = 3 },
                     { .x = 7, .y = 2 },
                     { .x = 8, .y = 2 },
                     { .x = 9, .y = 2 },
                   } };
      REQUIRE( f( viewer ) == expected );
    }
  }

  SECTION( "map 1 with hidden tiles" ) {
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

    GotoMapViewer const viewer( w.ss(), viz_player,
                                w.default_player_type(),
                                e_unit_type::free_colonist );

    // path: island
    src      = { .x = 8, .y = 9 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 28,
                           .iterations        = 23,
                           .queue_size_at_end = 16 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 5, .y = 9 },
                   { .x = 6, .y = 9 },
                   { .x = 7, .y = 9 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // With some visibility.
    w.make_clear( { .x = 7, .y = 8 } );
    w.make_clear( { .x = 8, .y = 8 } );
    src      = { .x = 8, .y = 9 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 22,
                           .iterations        = 15,
                           .queue_size_at_end = 13 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 5, .y = 9 },
                   { .x = 6, .y = 9 },
                   { .x = 7, .y = 9 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // path: top to bottom
    src      = { .x = 5, .y = 3 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 94,
                           .iterations        = 105,
                           .queue_size_at_end = 44 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 4, .y = 8 },
                   { .x = 4, .y = 7 },
                   { .x = 4, .y = 6 },
                   { .x = 4, .y = 5 },
                   { .x = 4, .y = 4 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // With a bit of visibility but not enough to change to the
    // optimal path, though the path does improve a bit.
    w.make_clear( { .x = 5, .y = 3 } );
    w.make_clear( { .x = 5, .y = 4 } );
    w.make_clear( { .x = 4, .y = 3 } );
    w.make_clear( { .x = 4, .y = 4 } );
    w.make_clear( { .x = 2, .y = 4 } );
    w.make_clear( { .x = 2, .y = 5 } );
    w.make_clear( { .x = 2, .y = 6 } );
    src      = { .x = 5, .y = 3 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 92,
                           .iterations        = 79,
                           .queue_size_at_end = 53 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 4, .y = 8 },
                   { .x = 4, .y = 7 },
                   { .x = 4, .y = 6 },
                   { .x = 4, .y = 5 },
                   { .x = 5, .y = 4 },
                 } };
    REQUIRE( f( viewer ) == expected );

    // One more tile of visibilty, now the path is mostly opti-
    // mal.
    w.make_clear( { .x = 2, .y = 7 } );
    src      = { .x = 5, .y = 3 };
    dst      = { .x = 4, .y = 9 };
    expected = { .meta = { .tiles_touched     = 91,
                           .iterations        = 78,
                           .queue_size_at_end = 51 },

                 .reverse_path = {
                   { .x = 4, .y = 9 },
                   { .x = 3, .y = 8 },
                   { .x = 2, .y = 7 },
                   { .x = 2, .y = 6 },
                   { .x = 2, .y = 5 },
                   { .x = 3, .y = 4 },
                   { .x = 4, .y = 3 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "inside square" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ x,_,_,_,_,_,_,_,_,x, /*0
      1*/ x,_,_,_,_,_,_,_,_,x, /*1
      2*/ x,_,G,G,G,G,G,G,_,x, /*2
      3*/ x,_,G,_,_,_,_,G,_,x, /*3
      4*/ x,_,G,_,_,_,_,G,_,x, /*4
      5*/ x,_,G,_,_,_,_,_,_,x, /*5
      6*/ x,_,G,_,_,_,_,G,_,x, /*6
      7*/ x,_,G,_,_,_,_,G,_,x, /*7
      8*/ x,_,G,G,G,G,G,G,_,x, /*8
      9*/ x,_,_,_,_,_,_,_,_,x, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 3, .y = 5 };
    dst      = { .x = 1, .y = 4 };
    expected = { .meta = { .tiles_touched     = 67,
                           .iterations        = 76,
                           .queue_size_at_end = 12 },

                 .reverse_path = {
                   { .x = 1, .y = 4 },
                   { .x = 1, .y = 3 },
                   { .x = 1, .y = 2 },
                   { .x = 2, .y = 1 },
                   { .x = 3, .y = 1 },
                   { .x = 4, .y = 1 },
                   { .x = 5, .y = 1 },
                   { .x = 6, .y = 1 },
                   { .x = 7, .y = 1 },
                   { .x = 8, .y = 2 },
                   { .x = 8, .y = 3 },
                   { .x = 8, .y = 4 },
                   { .x = 7, .y = 5 },
                   { .x = 6, .y = 5 },
                   { .x = 5, .y = 5 },
                   { .x = 4, .y = 5 },
                 } };
    REQUIRE( f( viewer ) == expected );

    src      = { .x = 3, .y = 5 };
    dst      = { .x = 1, .y = 6 };
    expected = { .meta         = { .tiles_touched     = 65,
                                   .iterations        = 72,
                                   .queue_size_at_end = 12 },
                 .reverse_path = {
                   { .x = 1, .y = 6 },
                   { .x = 1, .y = 7 },
                   { .x = 1, .y = 8 },
                   { .x = 2, .y = 9 },
                   { .x = 3, .y = 9 },
                   { .x = 4, .y = 9 },
                   { .x = 5, .y = 9 },
                   { .x = 6, .y = 9 },
                   { .x = 7, .y = 9 },
                   { .x = 8, .y = 8 },
                   { .x = 8, .y = 7 },
                   { .x = 8, .y = 6 },
                   { .x = 7, .y = 5 },
                   { .x = 6, .y = 5 },
                   { .x = 5, .y = 5 },
                   { .x = 4, .y = 5 },
                 } };
    REQUIRE( f( viewer ) == expected );

    src      = { .x = 3, .y = 5 };
    dst      = { .x = 1, .y = 5 };
    expected = { .meta         = { .tiles_touched     = 72,
                                   .iterations        = 82,
                                   .queue_size_at_end = 11 },
                 .reverse_path = {
                   { .x = 1, .y = 5 },
                   { .x = 1, .y = 4 },
                   { .x = 1, .y = 3 },
                   { .x = 1, .y = 2 },
                   { .x = 2, .y = 1 },
                   { .x = 3, .y = 1 },
                   { .x = 4, .y = 1 },
                   { .x = 5, .y = 1 },
                   { .x = 6, .y = 1 },
                   { .x = 7, .y = 1 },
                   { .x = 8, .y = 2 },
                   { .x = 8, .y = 3 },
                   { .x = 8, .y = 4 },
                   { .x = 7, .y = 5 },
                   { .x = 6, .y = 5 },
                   { .x = 5, .y = 5 },
                   { .x = 4, .y = 5 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }
}

TEST_CASE( "[goto] compute_harbor_goto_path" ) {
  world w;
  point src;
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
        return compute_harbor_goto_path( viewer, src );
      };

  SECTION( "no accessible sea lane tiles" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ _,G,x,x,G,_,_,_,_,_, /*0
      1*/ _,G,G,G,G,_,_,_,_,_, /*1
      2*/ _,_,_,_,_,_,_,_,_,_, /*2
      3*/ _,_,_,_,_,_,_,_,_,_, /*3
      4*/ _,_,G,G,G,G,_,_,_,_, /*4
      5*/ _,_,G,_,_,G,_,_,_,_, /*5
      6*/ _,_,G,_,G,G,_,_,_,_, /*6
      7*/ _,_,G,_,_,_,_,_,_,_, /*7
      8*/ _,_,G,G,G,G,_,_,_,_, /*8
      9*/ _,_,_,_,_,_,_,_,_,_, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 38,
                           .iterations        = 27,
                           .queue_size_at_end = 12 },

                 .reverse_path = {
                   { .x = 9, .y = 5 },
                   { .x = 8, .y = 5 },
                   { .x = 7, .y = 5 },
                   { .x = 6, .y = 6 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "accessible sea lane tiles but too far" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ _,_,x,x,G,_,_,_,_,_, /*0
      1*/ _,G,G,G,G,_,_,_,_,_, /*1
      2*/ _,_,_,_,_,_,_,_,_,_, /*2
      3*/ _,_,_,_,_,_,_,_,_,_, /*3
      4*/ _,_,G,G,G,G,_,_,_,_, /*4
      5*/ _,_,G,_,_,G,_,_,_,_, /*5
      6*/ _,_,G,_,G,G,_,_,_,_, /*6
      7*/ _,_,G,_,_,_,_,_,_,_, /*7
      8*/ _,_,G,G,G,G,_,_,_,_, /*8
      9*/ _,_,_,_,_,_,_,_,_,_, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 38,
                           .iterations        = 27,
                           .queue_size_at_end = 12 },

                 .reverse_path = {
                   { .x = 9, .y = 5 },
                   { .x = 8, .y = 5 },
                   { .x = 7, .y = 5 },
                   { .x = 6, .y = 6 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "right map edge not accessible" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ _,_,x,x,G,_,_,_,_,G, /*0
      1*/ _,G,G,G,G,_,_,_,_,G, /*1
      2*/ _,_,_,_,_,_,_,_,_,G, /*2
      3*/ _,_,_,_,_,_,_,_,_,G, /*3
      4*/ _,_,G,G,G,G,_,_,_,G, /*4
      5*/ _,_,G,_,_,G,_,_,_,G, /*5
      6*/ _,_,G,_,G,G,_,_,_,G, /*6
      7*/ _,_,G,_,_,_,_,_,_,G, /*7
      8*/ _,_,G,G,G,G,_,_,_,G, /*8
      9*/ _,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 59,
                           .iterations        = 48,
                           .queue_size_at_end = 12 },

                 .reverse_path = {
                   { .x = 0, .y = 7 },
                   { .x = 1, .y = 8 },
                   { .x = 2, .y = 9 },
                   { .x = 3, .y = 9 },
                   { .x = 4, .y = 9 },
                   { .x = 5, .y = 9 },
                   { .x = 6, .y = 8 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "map edge not accessible" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,G,x,x,G,_,_,_,_,G, /*0
      1*/ G,G,G,G,G,_,_,_,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 58,
                           .iterations        = 58,
                           .queue_size_at_end = 0 },

                 .reverse_path = {} };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "one accessible sea lane but not launch point" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,x,G,_,_,_,_,G, /*0
      1*/ G,_,G,G,G,_,_,_,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 62,
                           .iterations        = 62,
                           .queue_size_at_end = 0 },

                 .reverse_path = {} };
    REQUIRE( f( viewer ) == expected );
  }

  // NOTE: for this one the ship has to move to {x=3,.y=0} be-
  // cause that is the launch point for pacific side sea lane.
  // i.e., the ship needs to go to that square and then move to
  // the left to launch.
  SECTION( "has left-moving launch point" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,x,x,G,_,_,_,_,G, /*0
      1*/ G,_,G,G,G,_,_,_,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 62,
                           .iterations        = 62,
                           .queue_size_at_end = 1 },

                 .reverse_path = {
                   { .x = 3, .y = 0 },
                   { .x = 2, .y = 0 },
                   { .x = 1, .y = 1 },
                   { .x = 2, .y = 2 },
                   { .x = 3, .y = 3 },
                   { .x = 4, .y = 3 },
                   { .x = 5, .y = 3 },
                   { .x = 6, .y = 4 },
                   { .x = 6, .y = 5 },
                   { .x = 6, .y = 6 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "has right-moving launch point" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,_,_,x,x,G,_,G, /*0
      1*/ G,_,_,_,_,G,G,G,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 60,
                           .iterations        = 53,
                           .queue_size_at_end = 8 },

                 .reverse_path = {
                   { .x = 5, .y = 0 },
                   { .x = 4, .y = 1 },
                   { .x = 4, .y = 2 },
                   { .x = 5, .y = 3 },
                   { .x = 6, .y = 4 },
                   { .x = 6, .y = 5 },
                   { .x = 6, .y = 6 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "sea lane at bottom" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,_,_,x,x,G,_,G, /*0
      1*/ G,_,_,_,_,G,G,G,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,x,x,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 26,
                           .iterations        = 17,
                           .queue_size_at_end = 10 },

                 .reverse_path = {
                   { .x = 7, .y = 9 },
                   { .x = 6, .y = 8 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "finds closer sea lane" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };
    MS const x{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,_,_,x,x,G,_,G, /*0
      1*/ G,_,_,_,_,G,G,G,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,x,x,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,x,x,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 19,
                           .iterations        = 11,
                           .queue_size_at_end = 9 },

                 .reverse_path = {
                   { .x = 7, .y = 5 },
                   { .x = 6, .y = 6 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "one map edge at left" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,_,_,_,_,_,_,G, /*0
      1*/ G,_,_,_,_,_,_,_,_,G, /*1
      2*/ _,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    GotoMapViewer const viewer( w.ss(), viz_entire,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 67,
                           .iterations        = 67,
                           .queue_size_at_end = 1 },

                 .reverse_path = {
                   { .x = 0, .y = 2 },
                   { .x = 1, .y = 3 },
                   { .x = 2, .y = 3 },
                   { .x = 3, .y = 3 },
                   { .x = 4, .y = 3 },
                   { .x = 5, .y = 3 },
                   { .x = 6, .y = 4 },
                   { .x = 6, .y = 5 },
                   { .x = 6, .y = 6 },
                   { .x = 5, .y = 7 },
                   { .x = 4, .y = 7 },
                   { .x = 3, .y = 6 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "all fogged, no sea lane or map edges" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,_,_,_,_,_,_,G, /*0
      1*/ G,_,_,_,_,_,_,_,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    for( point const p :
         rect_iterator( viz_player.rect_tiles() ) )
      w.make_fogged( p );

    GotoMapViewer const viewer( w.ss(), viz_player,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 66,
                           .iterations        = 66,
                           .queue_size_at_end = 0 },

                 .reverse_path = {} };
    REQUIRE( f( viewer ) == expected );
  }

  SECTION( "all fogged, one map edge hidden" ) {
    MS const G{ .surface = land, .ground = grassland };
    MS const _{ .surface = water, .ground = arctic };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ G,_,_,_,_,_,_,_,_,G, /*0
      1*/ G,_,_,_,_,_,_,_,_,G, /*1
      2*/ G,_,_,_,_,_,_,_,_,G, /*2
      3*/ G,_,_,_,_,_,_,_,_,G, /*3
      4*/ G,_,G,G,G,G,_,_,_,G, /*4
      5*/ G,_,G,_,_,G,_,_,_,G, /*5
      6*/ G,_,G,_,G,G,_,_,_,G, /*6
      7*/ G,_,G,_,_,_,_,_,_,G, /*7
      8*/ G,_,G,G,G,G,_,_,_,G, /*8
      9*/ G,_,_,_,_,_,_,_,_,G, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );

    for( point const p :
         rect_iterator( viz_player.rect_tiles() ) )
      if( p != point{ .x = 9, .y = 2 } &&
          p != point{ .x = 5, .y = 5 } )
        w.make_fogged( p );

    GotoMapViewer const viewer( w.ss(), viz_player,
                                w.default_player_type(),
                                e_unit_type::frigate );

    src      = { .x = 4, .y = 5 };
    expected = { .meta = { .tiles_touched     = 48,
                           .iterations        = 35,
                           .queue_size_at_end = 14 },

                 .reverse_path = {
                   { .x = 9, .y = 2 },
                   { .x = 8, .y = 3 },
                   { .x = 7, .y = 4 },
                   { .x = 6, .y = 5 },
                   { .x = 5, .y = 5 },
                 } };
    REQUIRE( f( viewer ) == expected );
  }
}

TEST_CASE( "[goto] unit_has_reached_goto_target" ) {
  world w;
  goto_target target;

  using enum e_surface;
  using enum e_ground_terrain;
  using enum e_land_overlay;
  using enum e_river;
  using enum e_unit_type;

  using MS = MapSquare;

  static MS const _{ .surface = water };
  static MS const X{ .surface = land, .ground = grassland };
  static MS const s{ .surface = water, .sea_lane = true };

  // clang-format off
  vector<MapSquare> tiles{ /*
        0 1 2 3 4 5 6 7 8 9
    0*/ s,_,_,_,_,_,_,_,_,s, /*0
    1*/ s,_,_,_,_,_,_,_,_,s, /*1
    2*/ s,_,X,X,X,X,X,X,_,s, /*2
    3*/ s,_,X,X,X,X,X,X,_,s, /*3
    4*/ s,_,X,X,X,X,X,X,_,s, /*4
    5*/ s,_,X,X,X,X,X,X,_,s, /*5
    6*/ s,_,X,X,X,X,X,X,_,s, /*6
    7*/ s,_,X,X,X,X,X,X,_,s, /*7
    8*/ s,_,_,_,_,_,_,_,_,s, /*8
    9*/ s,_,_,_,_,_,_,_,_,s, /*9
        0 1 2 3 4 5 6 7 8 9
  */};
  // clang-format on

  w.build_map( std::move( tiles ), 10 );

  auto const f = [&] [[clang::noinline]] (
                     UnitId const unit_id ) {
    Unit const& unit = w.units().unit_for( unit_id );
    return unit_has_reached_goto_target( w.ss(), unit, target );
  };

  Unit& unit1 =
      w.add_unit_on_map( free_colonist, { .x = 2, .y = 2 } );

  target = goto_target::map{ .tile = { .x = 2, .y = 1 } };
  REQUIRE( f( unit1.id() ) == false );
  target = goto_target::map{ .tile = { .x = 3, .y = 4 } };
  REQUIRE( f( unit1.id() ) == false );
  target = goto_target::map{ .tile = { .x = 2, .y = 2 } };
  REQUIRE( f( unit1.id() ) == true );

  Unit& unit2 = w.add_unit_on_map( frigate, { .x = 1, .y = 2 } );
  target      = goto_target::map{ .tile = { .x = 2, .y = 1 } };
  REQUIRE( f( unit2.id() ) == false );
  target = goto_target::map{ .tile = { .x = 1, .y = 1 } };
  REQUIRE( f( unit2.id() ) == false );
  target = goto_target::map{ .tile = { .x = 1, .y = 2 } };
  REQUIRE( f( unit2.id() ) == true );

  unit_sail_to_harbor( w.ss(), unit2.id() );
  target = goto_target::map{ .tile = { .x = 1, .y = 2 } };
  REQUIRE( f( unit2.id() ) == false );
  target = goto_target::harbor{};
  REQUIRE( f( unit2.id() ) == false );

  Unit& unit3 = w.add_unit_in_port( frigate );
  target      = goto_target::map{ .tile = { .x = 1, .y = 2 } };
  REQUIRE( f( unit3.id() ) == false );
  target = goto_target::harbor{};
  REQUIRE( f( unit3.id() ) == true );
  unit_sail_to_new_world( w.ss(), unit3.id() );
  REQUIRE( f( unit3.id() ) == false );
}

TEST_CASE( "[goto] find_goto_port" ) {
  world w;
  point src;
  e_unit_type unit_type = {};
  GotoPort expected;
  Player& player = w.default_player();

  auto const f = [&] [[clang::noinline]] {
    return find_goto_port(
        w.ss().as_const, w.map_updater().connectivity(),
        w.default_player_type(), unit_type, src );
  };

  using enum e_ground_terrain;
  using enum e_land_overlay;
  using enum e_player;
  using enum e_revolution_status;
  using enum e_river;
  using enum e_surface;
  using enum e_unit_type;

  using MS = MapSquare;

  static MS const _{ .surface = water };
  static MS const X{ .surface = land, .ground = grassland };
  static MS const s{ .surface = water, .sea_lane = true };

  // clang-format off
  vector<MapSquare> tiles{ /*
        0 1 2 3 4 5 6 7 8 9
    0*/ s,_,_,_,X,_,_,_,_,s, /*0
    1*/ s,_,_,_,X,_,_,_,_,s, /*1
    2*/ s,_,X,X,X,_,X,X,_,s, /*2
    3*/ s,_,X,_,X,_,X,X,_,s, /*3
    4*/ X,X,X,X,X,_,X,X,_,s, /*4
    5*/ s,_,_,_,_,_,_,_,_,s, /*5
    6*/ X,X,X,X,X,X,X,X,X,X, /*6
    7*/ X,_,X,X,X,_,X,X,_,X, /*7
    8*/ X,_,_,_,_,_,_,_,_,X, /*8
    9*/ X,_,_,_,_,X,_,_,_,X, /*9
        0 1 2 3 4 5 6 7 8 9
  */};
  // clang-format on

  w.build_map( std::move( tiles ), 10 );

  unit_type = caravel;
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = caravel;
  src                      = { .x = 0, .y = 0 };
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = caravel;
  src       = { .x = 3, .y = 3 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 7, .y = 0 };
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 5, .y = 7 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 2, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 6, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 6 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 7 }; // as if in colony
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist; // as if on ship
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = free_colonist;
  src                      = { .x = 0, .y = 0 }; // as if on ship
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = free_colonist;
  src       = { .x = 3, .y = 3 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 7, .y = 0 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 5, .y = 7 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 2, .y = 2 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 6, .y = 2 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 6 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 7 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 2, .y = 2 } );

  unit_type = caravel;
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = true, .colonies = { 1 } };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = caravel;
  src                      = { .x = 0, .y = 0 };
  expected = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = caravel;
  src       = { .x = 3, .y = 3 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 7, .y = 0 };
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 5, .y = 7 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 2, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 6, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 6 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 7 }; // as if in colony
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist; // as if on ship
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = free_colonist;
  src                      = { .x = 0, .y = 0 }; // as if on ship
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = free_colonist;
  src       = { .x = 3, .y = 3 }; // as if on ship
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 7, .y = 0 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 5, .y = 7 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 2, .y = 2 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 4, .y = 2 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 6, .y = 2 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 6 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 7 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 7, .y = 4 } );

  unit_type = caravel;
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = true, .colonies = { 1 } };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = caravel;
  src                      = { .x = 0, .y = 0 };
  expected = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = caravel;
  src       = { .x = 3, .y = 3 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 7, .y = 0 };
  expected  = { .europe = true, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 5, .y = 7 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 2, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 6, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 6 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 7 }; // as if in colony
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist; // as if on ship
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = free_colonist;
  src                      = { .x = 0, .y = 0 }; // as if on ship
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = free_colonist;
  src       = { .x = 3, .y = 3 }; // as if on ship
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 7, .y = 0 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 5, .y = 7 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 2, .y = 2 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 4, .y = 2 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 6, .y = 2 };
  expected  = { .europe = false, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 6 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 7 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 7, .y = 7 } );

  unit_type = caravel;
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = true, .colonies = { 1 } };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = caravel;
  src                      = { .x = 0, .y = 0 };
  expected = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = caravel;
  src       = { .x = 3, .y = 3 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 7, .y = 0 };
  expected  = { .europe = true, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 5, .y = 7 };
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 2, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 6, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 6 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 7 }; // as if in colony
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist; // as if on ship
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = free_colonist;
  src                      = { .x = 0, .y = 0 }; // as if on ship
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = free_colonist;
  src       = { .x = 3, .y = 3 }; // as if on ship
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 7, .y = 0 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 5, .y = 7 }; // as if on ship
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 2, .y = 2 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 4, .y = 2 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 6, .y = 2 };
  expected  = { .europe = false, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 6 };
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 7 };
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 1, .y = 4 } );

  unit_type = caravel;
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = true, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = caravel;
  src                      = { .x = 0, .y = 0 };
  expected = { .europe = false, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = caravel;
  src       = { .x = 3, .y = 3 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 7, .y = 0 };
  expected  = { .europe = true, .colonies = { 2, 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 5, .y = 7 };
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 2, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = { 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 6, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2, 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 6 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2, 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 7 }; // as if in colony
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist; // as if on ship
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = free_colonist;
  src                      = { .x = 0, .y = 0 }; // as if on ship
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = free_colonist;
  src       = { .x = 3, .y = 3 }; // as if on ship
  expected  = { .europe = false, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 7, .y = 0 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 5, .y = 7 }; // as if on ship
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 2, .y = 2 };
  expected  = { .europe = false, .colonies = { 4 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 4, .y = 2 };
  expected  = { .europe = false, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 6, .y = 2 };
  expected  = { .europe = false, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 6 };
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 7 };
  expected  = { .europe = false, .colonies = { 3 } };
  REQUIRE( f() == expected );

  w.add_colony( { .x = 9, .y = 7 }, french );
  w.add_colony( { .x = 9, .y = 9 } );

  unit_type = caravel;
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = true, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = caravel;
  src                      = { .x = 0, .y = 0 };
  expected = { .europe = false, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = caravel;
  src       = { .x = 3, .y = 3 };
  expected  = { .europe = false, .colonies = { 1 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 7, .y = 0 };
  expected  = { .europe = true, .colonies = { 2, 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 5, .y = 7 };
  expected  = { .europe = false, .colonies = { 3, 6 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 2, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = { 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 6, .y = 2 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2, 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 6 }; // as if in colony
  expected  = { .europe = true, .colonies = { 2, 4 } };
  REQUIRE( f() == expected );

  unit_type = caravel;
  src       = { .x = 3, .y = 7 }; // as if in colony
  expected  = { .europe = false, .colonies = { 3, 6 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist; // as if on ship
  src       = { .x = 0, .y = 0 };
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  player.revolution.status = declared;
  unit_type                = free_colonist;
  src                      = { .x = 0, .y = 0 }; // as if on ship
  expected                 = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );
  player.revolution.status = not_declared;

  unit_type = free_colonist;
  src       = { .x = 3, .y = 3 }; // as if on ship
  expected  = { .europe = false, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 7, .y = 0 }; // as if on ship
  expected  = { .europe = false, .colonies = {} };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 5, .y = 7 }; // as if on ship
  expected  = { .europe = false, .colonies = { 3, 6 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 2, .y = 2 };
  expected  = { .europe = false, .colonies = { 4 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 4, .y = 2 };
  expected  = { .europe = false, .colonies = { 1, 4 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 6, .y = 2 };
  expected  = { .europe = false, .colonies = { 2 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 6 };
  expected  = { .europe = false, .colonies = { 3, 6 } };
  REQUIRE( f() == expected );

  unit_type = free_colonist;
  src       = { .x = 3, .y = 7 };
  expected  = { .europe = false, .colonies = { 3, 6 } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[goto] ask_goto_port" ) {
  world w;
  GotoPort goto_port;
  goto_target expected;
  e_unit_type unit_type = {};

  auto const f = [&] [[clang::noinline]] {
    return co_await_test( ask_goto_port(
        w.ss().as_const, w.gui(), w.default_player(), goto_port,
        unit_type ) );
  };

  using enum e_surface;
  using enum e_ground_terrain;
  using enum e_land_overlay;
  using enum e_river;
  using enum e_unit_type;

  {
    using MS = MapSquare;

    static MS const _{ .surface = water };
    static MS const X{ .surface = land, .ground = grassland };
    static MS const s{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ s,_,_,_,_,_,_,_,_,s, /*0
      1*/ s,_,_,_,_,_,_,_,_,s, /*1
      2*/ s,_,X,X,X,X,X,X,_,s, /*2
      3*/ s,_,X,X,X,X,X,X,_,s, /*3
      4*/ s,_,X,X,X,X,X,X,_,s, /*4
      5*/ s,_,X,X,X,X,X,X,_,s, /*5
      6*/ s,_,X,X,X,X,X,X,_,s, /*6
      7*/ s,_,X,X,X,X,X,X,_,s, /*7
      8*/ s,_,_,_,_,_,_,_,_,s, /*8
      9*/ s,_,_,_,_,_,_,_,_,s, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );
  }

  // No colonies, no harbor.
  goto_port = {};
  unit_type = caravel;
  w.gui().EXPECT__message_box(
      StrContains( "no available ports" ) );
  REQUIRE( f() == nothing );

  // No colonies, with harbor, escapes.
  goto_port = { .europe = true };
  unit_type = caravel;
  w.gui().EXPECT__choice( _ ).returns( nothing );
  REQUIRE( f() == nothing );

  // No colonies, with harbor, chooses harbor.
  goto_port = { .europe = true };
  unit_type = caravel;
  w.gui().EXPECT__choice( _ ).returns( " <europe> " );
  expected = goto_target::harbor{};
  REQUIRE( f() == expected );

  w.add_colony( { .x = 2, .y = 2 } ).name = "colony_1";

  // One colony, no harbor.
  goto_port = { .colonies = { 1 } };
  unit_type = caravel;
  ChoiceConfig const config1{
    .msg     = "Select Destination Port:",
    .options = { ChoiceConfigOption{
      .key = "1", .display_name = "colony_1" } } };
  w.gui().EXPECT__choice( config1 ).returns( "1" );
  expected = goto_target::map{
    .tile     = { .x = 2, .y = 2 },
    .snapshot = GotoTargetSnapshot::empty_or_friendly{} };
  REQUIRE( f() == expected );

  // One colony, with harbor, escapes.
  goto_port = { .europe = true, .colonies = { 1 } };
  unit_type = caravel;
  ChoiceConfig const config2{
    .msg     = "Select Destination Port:",
    .options = {
      ChoiceConfigOption{
        .key          = " <europe> ",
        .display_name = "Amsterdam (The Netherlands)" },
      ChoiceConfigOption{ .key          = "1",
                          .display_name = "colony_1" } } };
  w.gui().EXPECT__choice( config2 ).returns( nothing );
  REQUIRE( f() == nothing );

  // One colony, with harbor, chooses harbor.
  goto_port = { .europe = true, .colonies = { 1 } };
  unit_type = caravel;
  ChoiceConfig const config3{
    .msg     = "Select Destination Port:",
    .options = {
      ChoiceConfigOption{
        .key          = " <europe> ",
        .display_name = "Amsterdam (The Netherlands)" },
      ChoiceConfigOption{ .key          = "1",
                          .display_name = "colony_1" } } };
  w.gui().EXPECT__choice( config3 ).returns( " <europe> " );
  expected = goto_target::harbor{};
  REQUIRE( f() == expected );

  // One colony, with harbor, chooses colony.
  goto_port = { .europe = true, .colonies = { 1 } };
  unit_type = caravel;
  ChoiceConfig const config4{
    .msg     = "Select Destination Port:",
    .options = {
      ChoiceConfigOption{
        .key          = " <europe> ",
        .display_name = "Amsterdam (The Netherlands)" },
      ChoiceConfigOption{ .key          = "1",
                          .display_name = "colony_1" } } };
  w.gui().EXPECT__choice( config4 ).returns( "1" );
  expected = goto_target::map{
    .tile     = { .x = 2, .y = 2 },
    .snapshot = GotoTargetSnapshot::empty_or_friendly{} };
  REQUIRE( f() == expected );

  // One colony, chooses colony.
  goto_port = { .europe = false, .colonies = { 1 } };
  unit_type = free_colonist;
  ChoiceConfig const config5{
    .msg     = "Select Destination Colony:",
    .options = { ChoiceConfigOption{
      .key = "1", .display_name = "colony_1" } } };
  w.gui().EXPECT__choice( config5 ).returns( "1" );
  expected = goto_target::map{
    .tile     = { .x = 2, .y = 2 },
    .snapshot = GotoTargetSnapshot::empty_or_friendly{} };
  REQUIRE( f() == expected );
}

TEST_CASE( "[goto] compute_goto_target_snapshot" ) {
  world w;
  point tile;

  using enum e_player;
  using enum e_tribe;

  using S = GotoTargetSnapshot;

  using empty_or_friendly = S::empty_or_friendly;
  using foreign_colony    = S::foreign_colony;
  using foreign_unit      = S::foreign_unit;
  using dwelling          = S::dwelling;
  using brave             = S::brave;
  using empty_or_friendly_with_sea_lane =
      S::empty_or_friendly_with_sea_lane;
  using empty_with_lcr = S::empty_with_lcr;

  IVisibility const* p_viz = {};

  auto const f = [&] [[clang::noinline]] {
    BASE_CHECK( p_viz );
    return compute_goto_target_snapshot(
        w.ss(), *p_viz, w.default_player_type(), tile );
  };

  {
    using enum e_surface;
    using enum e_ground_terrain;
    using enum e_land_overlay;
    using enum e_river;
    using enum e_unit_type;

    using MS = MapSquare;

    static MS const _{ .surface = water };
    static MS const X{ .surface = land, .ground = grassland };
    static MS const s{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ s,_,_,_,_,_,_,_,_,s, /*0
      1*/ s,_,_,_,_,_,_,_,_,s, /*1
      2*/ s,_,X,X,X,X,X,X,_,s, /*2
      3*/ s,_,X,X,X,X,X,X,_,s, /*3
      4*/ s,_,X,X,X,X,X,X,_,s, /*4
      5*/ s,_,X,X,X,X,X,X,_,s, /*5
      6*/ s,_,X,X,X,X,X,X,_,s, /*6
      7*/ s,_,X,X,X,X,X,X,_,s, /*7
      8*/ s,_,_,_,_,_,_,_,_,s, /*8
      9*/ s,_,_,_,_,_,_,_,_,s, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );
  }

  SECTION( "player visibility" ) {
    VisibilityForPlayer const viz( w.ss(),
                                   w.default_player_type() );
    p_viz = &viz;

    tile = { .x = 2, .y = 2 };
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.square( tile ).lost_city_rumor = true;
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_with_lcr{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_with_lcr{} );

    tile = { .x = 1, .y = 2 };
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.square( tile ).sea_lane = true;
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly_with_sea_lane{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_or_friendly_with_sea_lane{} );

    tile = { .x = 7, .y = 2 };
    Dwelling const& dwelling1 =
        w.add_dwelling( tile, e_tribe::iroquois );
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == dwelling{} );
    w.make_clear( tile );
    REQUIRE( f() == dwelling{} );

    tile = tile.moved_left();
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling1.id );
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );
    w.make_clear( tile );
    REQUIRE( f() == brave{ .tribe = iroquois } );

    tile = { .x = 7, .y = 3 };
    Dwelling const& dwelling2 =
        w.add_dwelling( tile, e_tribe::iroquois );
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling2.id );
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == dwelling{} );
    w.make_clear( tile );
    REQUIRE( f() == dwelling{} );

    tile = { .x = 7, .y = 4 };
    w.add_colony( tile );
    REQUIRE( f() == nothing );

    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_or_friendly{} );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_or_friendly{} );

    tile = { .x = 7, .y = 7 };
    w.add_colony( tile, e_player::french );
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == foreign_colony{ .player = french } );
    w.make_clear( tile );
    REQUIRE( f() == foreign_colony{ .player = french } );

    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    REQUIRE( f() == foreign_colony{ .player = french } );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    REQUIRE( f() == nothing );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );
    w.make_clear( tile );
    REQUIRE( f() == foreign_unit{ .player = french } );
  }

  SECTION( "full visibility" ) {
    VisibilityEntire const viz( w.ss() );
    p_viz = &viz;

    tile = { .x = 2, .y = 2 };
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.square( tile ).lost_city_rumor = true;
    REQUIRE( f() == empty_with_lcr{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_with_lcr{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_with_lcr{} );

    tile = { .x = 1, .y = 2 };
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.square( tile ).sea_lane = true;
    REQUIRE( f() == empty_or_friendly_with_sea_lane{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly_with_sea_lane{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_or_friendly_with_sea_lane{} );

    tile = { .x = 7, .y = 2 };
    Dwelling const& dwelling1 =
        w.add_dwelling( tile, e_tribe::iroquois );
    REQUIRE( f() == dwelling{} );

    w.make_fogged( tile );
    REQUIRE( f() == dwelling{} );
    w.make_clear( tile );
    REQUIRE( f() == dwelling{} );

    tile = tile.moved_left();
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling1.id );
    REQUIRE( f() == brave{ .tribe = iroquois } );

    w.make_fogged( tile );
    REQUIRE( f() == brave{ .tribe = iroquois } );
    w.make_clear( tile );
    REQUIRE( f() == brave{ .tribe = iroquois } );

    tile = { .x = 7, .y = 3 };
    Dwelling const& dwelling2 =
        w.add_dwelling( tile, e_tribe::iroquois );
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling2.id );
    REQUIRE( f() == dwelling{} );

    w.make_fogged( tile );
    REQUIRE( f() == dwelling{} );
    w.make_clear( tile );
    REQUIRE( f() == dwelling{} );

    tile = { .x = 7, .y = 4 };
    w.add_colony( tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_or_friendly{} );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    REQUIRE( f() == empty_or_friendly{} );

    w.make_fogged( tile );
    REQUIRE( f() == empty_or_friendly{} );
    w.make_clear( tile );
    REQUIRE( f() == empty_or_friendly{} );

    tile = { .x = 7, .y = 7 };
    w.add_colony( tile, e_player::french );
    REQUIRE( f() == foreign_colony{ .player = french } );

    w.make_fogged( tile );
    REQUIRE( f() == foreign_colony{ .player = french } );
    w.make_clear( tile );
    REQUIRE( f() == foreign_colony{ .player = french } );

    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    REQUIRE( f() == foreign_colony{ .player = french } );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    REQUIRE( f() == foreign_unit{ .player = french } );

    w.make_fogged( tile );
    REQUIRE( f() == foreign_unit{ .player = french } );
    w.make_clear( tile );
    REQUIRE( f() == foreign_unit{ .player = french } );
  }
}

TEST_CASE( "[goto] create_goto_map_target" ) {
  world w;
  point tile;
  goto_target::map expected;

  using enum e_player;
  using enum e_tribe;

  using S = GotoTargetSnapshot;

  using empty_or_friendly = S::empty_or_friendly;
  using foreign_colony    = S::foreign_colony;
  using foreign_unit      = S::foreign_unit;
  using dwelling          = S::dwelling;
  using brave             = S::brave;
  using empty_or_friendly_with_sea_lane =
      S::empty_or_friendly_with_sea_lane;
  using empty_with_lcr = S::empty_with_lcr;

  auto const f = [&] [[clang::noinline]] {
    expected.tile = tile;
    return create_goto_map_target(
        w.ss(), w.default_player_type(), tile );
  };

  {
    using enum e_surface;
    using enum e_ground_terrain;
    using enum e_land_overlay;
    using enum e_river;
    using enum e_unit_type;

    using MS = MapSquare;

    static MS const _{ .surface = water };
    static MS const X{ .surface = land, .ground = grassland };
    static MS const s{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ s,_,_,_,_,_,_,_,_,s, /*0
      1*/ s,_,_,_,_,_,_,_,_,s, /*1
      2*/ s,_,X,X,X,X,X,X,_,s, /*2
      3*/ s,_,X,X,X,X,X,X,_,s, /*3
      4*/ s,_,X,X,X,X,X,X,_,s, /*4
      5*/ s,_,X,X,X,X,X,X,_,s, /*5
      6*/ s,_,X,X,X,X,X,X,_,s, /*6
      7*/ s,_,X,X,X,X,X,X,_,s, /*7
      8*/ s,_,_,_,_,_,_,_,_,s, /*8
      9*/ s,_,_,_,_,_,_,_,_,s, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );
  }

  SECTION( "player visibility" ) {
    w.land_view().map_revealed =
        MapRevealed::player{ .type = w.default_player_type() };

    tile              = { .x = 2, .y = 2 };
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.square( tile ).lost_city_rumor = true;
    expected.snapshot                = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_with_lcr{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_with_lcr{};
    REQUIRE( f() == expected );

    tile              = { .x = 1, .y = 2 };
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.square( tile ).sea_lane = true;
    expected.snapshot         = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly_with_sea_lane{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_or_friendly_with_sea_lane{};
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 2 };
    Dwelling const& dwelling1 =
        w.add_dwelling( tile, e_tribe::iroquois );
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );

    tile = tile.moved_left();
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling1.id );
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = brave{ .tribe = iroquois };
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 3 };
    Dwelling const& dwelling2 =
        w.add_dwelling( tile, e_tribe::iroquois );
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling2.id );
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 4 };
    w.add_colony( tile );
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 7 };
    w.add_colony( tile, e_player::french );
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );

    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    expected.snapshot = nothing;
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = foreign_unit{ .player = french };
    REQUIRE( f() == expected );
  }

  SECTION( "full visibility" ) {
    w.land_view().map_revealed = MapRevealed::entire{};

    tile              = { .x = 2, .y = 2 };
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.square( tile ).lost_city_rumor = true;
    expected.snapshot                = empty_with_lcr{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_with_lcr{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_with_lcr{};
    REQUIRE( f() == expected );

    tile              = { .x = 1, .y = 2 };
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.square( tile ).sea_lane = true;
    expected.snapshot = empty_or_friendly_with_sea_lane{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly_with_sea_lane{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_or_friendly_with_sea_lane{};
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 2 };
    Dwelling const& dwelling1 =
        w.add_dwelling( tile, e_tribe::iroquois );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );

    tile = tile.moved_left();
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling1.id );
    expected.snapshot = brave{ .tribe = iroquois };
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = brave{ .tribe = iroquois };
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = brave{ .tribe = iroquois };
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 3 };
    Dwelling const& dwelling2 =
        w.add_dwelling( tile, e_tribe::iroquois );
    w.add_native_unit_on_map( e_native_unit_type::brave, tile,
                              dwelling2.id );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = dwelling{};
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 4 };
    w.add_colony( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = empty_or_friendly{};
    REQUIRE( f() == expected );

    tile = { .x = 7, .y = 7 };
    w.add_colony( tile, e_player::french );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );

    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    expected.snapshot = foreign_colony{ .player = french };
    REQUIRE( f() == expected );

    tile = tile.moved_left();
    w.add_unit_on_map( e_unit_type::free_colonist, tile,
                       e_player::french );
    expected.snapshot = foreign_unit{ .player = french };
    REQUIRE( f() == expected );

    w.make_fogged( tile );
    expected.snapshot = foreign_unit{ .player = french };
    REQUIRE( f() == expected );
    w.make_clear( tile );
    expected.snapshot = foreign_unit{ .player = french };
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[goto] is_new_goto_snapshot_allowed" ) {
  using enum e_player;
  using S = GotoTargetSnapshot;

  using empty_or_friendly = S::empty_or_friendly;
  using foreign_colony    = S::foreign_colony;
  using foreign_unit      = S::foreign_unit;
  using dwelling          = S::dwelling;
  using brave             = S::brave;
  using empty_or_friendly_with_sea_lane =
      S::empty_or_friendly_with_sea_lane;
  using empty_with_lcr = S::empty_with_lcr;

  auto const f =
      [&] [[clang::noinline]]
      ( maybe<GotoTargetSnapshot> const old,
        GotoTargetSnapshot const& New ) {
        return is_new_goto_snapshot_allowed( old, New );
      };

  REQUIRE( f( nothing, empty_or_friendly{} ) == true );
  REQUIRE( f( nothing, foreign_colony{} ) == false );
  REQUIRE( f( nothing, foreign_unit{} ) == false );
  REQUIRE( f( nothing, dwelling{} ) == false );
  REQUIRE( f( nothing, brave{} ) == false );
  REQUIRE( f( nothing, empty_or_friendly_with_sea_lane{} ) ==
           true );
  REQUIRE( f( nothing, empty_with_lcr{} ) == false );

  REQUIRE( f( empty_or_friendly{}, empty_or_friendly{} ) ==
           true );
  REQUIRE( f( empty_or_friendly{}, foreign_colony{} ) == false );
  REQUIRE( f( empty_or_friendly{}, foreign_unit{} ) == false );
  REQUIRE( f( empty_or_friendly{}, dwelling{} ) == false );
  REQUIRE( f( empty_or_friendly{}, brave{} ) == false );
  REQUIRE( f( empty_or_friendly{},
              empty_or_friendly_with_sea_lane{} ) == false );
  REQUIRE( f( empty_or_friendly{}, empty_with_lcr{} ) == false );

  REQUIRE( f( foreign_colony{ .player = french },
              empty_or_friendly{} ) == true );
  REQUIRE( f( foreign_colony{}, foreign_colony{} ) == true );
  REQUIRE( f( foreign_colony{ .player = french },
              foreign_colony{ .player = french } ) == true );
  REQUIRE( f( foreign_colony{ .player = french },
              foreign_colony{ .player = spanish } ) == false );
  REQUIRE( f( foreign_colony{}, foreign_unit{} ) == false );
  REQUIRE( f( foreign_colony{}, dwelling{} ) == false );
  REQUIRE( f( foreign_colony{}, brave{} ) == false );
  REQUIRE( f( foreign_colony{},
              empty_or_friendly_with_sea_lane{} ) == false );
  REQUIRE( f( foreign_colony{}, empty_with_lcr{} ) == false );

  REQUIRE( f( foreign_unit{ .player = french },
              empty_or_friendly{} ) == true );
  REQUIRE( f( foreign_unit{}, foreign_colony{} ) == false );
  REQUIRE( f( foreign_unit{ .player = french },
              foreign_unit{ .player = french } ) == true );
  REQUIRE( f( foreign_unit{ .player = french },
              foreign_unit{ .player = spanish } ) == false );
  REQUIRE( f( foreign_unit{}, dwelling{} ) == false );
  REQUIRE( f( foreign_unit{}, brave{} ) == false );
  REQUIRE( f( foreign_unit{},
              empty_or_friendly_with_sea_lane{} ) == false );
  REQUIRE( f( foreign_unit{}, empty_with_lcr{} ) == false );

  REQUIRE( f( dwelling{}, empty_or_friendly{} ) == true );
  REQUIRE( f( dwelling{}, foreign_colony{} ) == false );
  REQUIRE( f( dwelling{}, foreign_unit{} ) == false );
  REQUIRE( f( dwelling{}, dwelling{} ) == true );
  REQUIRE( f( dwelling{}, brave{} ) == false );
  REQUIRE( f( dwelling{}, empty_or_friendly_with_sea_lane{} ) ==
           false );
  REQUIRE( f( dwelling{}, empty_with_lcr{} ) == false );

  REQUIRE( f( brave{ .tribe = e_tribe::cherokee },
              empty_or_friendly{} ) == true );
  REQUIRE( f( brave{}, foreign_colony{} ) == false );
  REQUIRE( f( brave{}, foreign_unit{} ) == false );
  REQUIRE( f( brave{}, dwelling{} ) == false );
  REQUIRE( f( brave{ .tribe = e_tribe::cherokee },
              brave{ .tribe = e_tribe::cherokee } ) == true );
  REQUIRE( f( brave{ .tribe = e_tribe::cherokee },
              brave{ .tribe = e_tribe::inca } ) == false );
  REQUIRE( f( brave{}, empty_or_friendly_with_sea_lane{} ) ==
           false );
  REQUIRE( f( brave{}, empty_with_lcr{} ) == false );

  REQUIRE( f( empty_or_friendly_with_sea_lane{},
              empty_or_friendly{} ) == true );
  REQUIRE( f( empty_or_friendly_with_sea_lane{},
              foreign_colony{} ) == false );
  REQUIRE( f( empty_or_friendly_with_sea_lane{},
              foreign_unit{} ) == false );
  REQUIRE( f( empty_or_friendly_with_sea_lane{}, dwelling{} ) ==
           false );
  REQUIRE( f( empty_or_friendly_with_sea_lane{}, brave{} ) ==
           false );
  REQUIRE( f( empty_or_friendly_with_sea_lane{},
              empty_or_friendly_with_sea_lane{} ) == true );
  REQUIRE( f( empty_or_friendly_with_sea_lane{},
              empty_with_lcr{} ) == false );

  REQUIRE( f( empty_with_lcr{}, empty_or_friendly{} ) == true );
  REQUIRE( f( empty_with_lcr{}, foreign_colony{} ) == false );
  REQUIRE( f( empty_with_lcr{}, foreign_unit{} ) == false );
  REQUIRE( f( empty_with_lcr{}, dwelling{} ) == false );
  REQUIRE( f( empty_with_lcr{}, brave{} ) == false );
  REQUIRE( f( empty_with_lcr{},
              empty_or_friendly_with_sea_lane{} ) == false );
  REQUIRE( f( empty_with_lcr{}, empty_with_lcr{} ) == true );
}

TEST_CASE( "[goto] find_next_move_for_unit_with_goto_target" ) {
  world w;
  EvolveGoto expected;
  goto_target target;

  using enum e_player;
  using enum e_tribe;
  using enum e_unit_type;
  using enum e_surface;

  using S = GotoTargetSnapshot;

  using empty_or_friendly = S::empty_or_friendly;
  // using foreign_colony    = S::foreign_colony;
  // using foreign_unit      = S::foreign_unit;
  // using dwelling          = S::dwelling;
  // using brave             = S::brave;
  // using empty_or_friendly_with_sea_lane =
  //     S::empty_or_friendly_with_sea_lane;
  // using empty_with_lcr = S::empty_with_lcr;

  GotoRegistry registry;
  IVisibility const* p_viz = {};
  Unit const* p_unit       = {};

  auto const f = [&] [[clang::noinline]] {
    BASE_CHECK( p_unit );
    BASE_CHECK( p_viz );
    GotoMapViewer const viewer(
        w.ss(), *p_viz, p_unit->player_type(), p_unit->type() );
    return find_next_move_for_unit_with_goto_target(
        w.ss().as_const, w.map_updater().connectivity(),
        registry, viewer, *p_unit, target );
  };

  auto const set_unit_pos = [&]( point const p ) {
    BASE_CHECK( p_unit );
    UnitOwnershipChanger( w.ss(), p_unit->id() )
        .change_to_map_non_interactive( w.map_updater(), p );
  };

  {
    using enum e_surface;
    using enum e_ground_terrain;
    using enum e_land_overlay;
    using enum e_river;
    using enum e_unit_type;

    using MS = MapSquare;

    static MS const _{ .surface = water };
    static MS const X{ .surface = land, .ground = grassland };
    static MS const s{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ s,_,_,_,_,_,_,_,_,s, /*0
      1*/ s,_,_,_,_,_,_,s,s,s, /*1
      2*/ s,_,X,X,X,X,X,X,_,s, /*2
      3*/ s,_,X,X,X,X,X,X,_,s, /*3
      4*/ s,_,X,X,X,X,X,X,_,s, /*4
      5*/ s,_,X,X,X,X,X,X,_,s, /*5
      6*/ s,_,X,X,X,X,X,X,_,s, /*6
      7*/ s,_,X,X,X,X,X,X,_,s, /*7
      8*/ s,_,_,_,_,_,_,_,_,s, /*8
      9*/ s,_,_,_,_,_,_,_,_,s, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );
  }

  VisibilityEntire const viz_entire( w.ss() );
  VisibilityForPlayer const viz_player(
      w.ss(), w.default_player_type() );

  Unit& land_unit =
      w.add_unit_on_map( free_colonist, { .x = 2, .y = 2 } );

  // ------------------------------------------------------------
  // goto tile
  // ------------------------------------------------------------
  p_viz  = &viz_entire;
  p_unit = &land_unit;

  // Simple path.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 4, .y = 4 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 3, .y = 3 } );
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 4, .y = 4 } );
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );

  // Change location mid-path.
  registry.units.clear();
  target   = goto_target::map{ .tile     = { .x = 7, .y = 7 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 5, .y = 6 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 6, .y = 6 } );
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 7, .y = 7 } );
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );

  // Change target mid-path.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 7, .y = 7 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  target   = goto_target::map{ .tile     = { .x = 7, .y = 2 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );

  // Finds a path to water adjacent to land.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 8, .y = 8 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );

  // Cannot find a path.
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 9, .y = 9 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );

  // Goto current location.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 2, .y = 2 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );

  // Changes location just before destination, empty goto path
  // detected.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 3, .y = 3 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 4, .y = 3 } );
  expected = EvolveGoto::move{ .to = e_direction::w };
  REQUIRE( f() == expected );

  // Changes location mid-path.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 5, .y = 5 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 3, .y = 3 } );
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 7, .y = 7 } );
  expected = EvolveGoto::move{ .to = e_direction::nw };
  REQUIRE( f() == expected );

  // Dwelling appears on tile on path.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 5, .y = 5 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 3, .y = 3 } );
  DwellingId const dwelling_id_1 =
      w.add_dwelling( { .x = 4, .y = 4 }, arawak ).id;
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  destroy_dwelling( w.ss(), w.map_updater(), dwelling_id_1 );

  // Destination changes contents.
  registry.units.clear();
  set_unit_pos( { .x = 2, .y = 2 } );
  target   = goto_target::map{ .tile     = { .x = 4, .y = 4 },
                               .snapshot = empty_or_friendly{} };
  expected = EvolveGoto::move{ .to = e_direction::se };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 3, .y = 3 } );
  w.square( { .x = 4, .y = 4 } ).lost_city_rumor = true;
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );
  w.square( { .x = 4, .y = 4 } ).lost_city_rumor = false;

  // ------------------------------------------------------------
  // goto harbor
  // ------------------------------------------------------------
  Unit& ship_unit =
      w.add_unit_on_map( caravel, { .x = 1, .y = 1 } );

  p_viz  = &viz_entire;
  p_unit = &ship_unit;
  target = goto_target::harbor{};

  // Goto east sea lane.
  set_unit_pos( { .x = 5, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 6, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 7, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );

  // Changes location just before destination, empty goto path
  // detected.
  registry.units.clear();
  set_unit_pos( { .x = 6, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  REQUIRE(
      registry.units[p_unit->id()].path.reverse_path.empty() );
  set_unit_pos( { .x = 3, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::w };
  REQUIRE( f() == expected );

  // Changes location mid-path.
  registry.units.clear();
  set_unit_pos( { .x = 4, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::w };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 4, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::w };
  REQUIRE( f() == expected );

  // Foreign ship appears on tile on path.
  registry.units.clear();
  set_unit_pos( { .x = 5, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 6, .y = 1 } );
  UnitId const foreign_unit_id_1 =
      w.add_unit_on_map( caravel, { .x = 7, .y = 1 }, french )
          .id();
  expected = EvolveGoto::move{ .to = e_direction::ne };
  REQUIRE( f() == expected );
  UnitOwnershipChanger( w.ss(), foreign_unit_id_1 ).destroy();

  // Destination changes contents twice.
  registry.units.clear();
  set_unit_pos( { .x = 5, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 6, .y = 1 } );
  w.square( { .x = 7, .y = 1 } ).surface = land;
  expected = EvolveGoto::move{ .to = e_direction::ne };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 7, .y = 0 } );
  w.square( { .x = 7, .y = 1 } ).surface = water;
  expected = EvolveGoto::move{ .to = e_direction::s };
  REQUIRE( f() == expected );
  set_unit_pos( { .x = 7, .y = 1 } );
  expected = EvolveGoto::move{ .to = e_direction::e };
  REQUIRE( f() == expected );

  {
    using enum e_surface;
    using enum e_ground_terrain;
    using enum e_land_overlay;
    using enum e_river;
    using enum e_unit_type;

    using MS = MapSquare;

    static MS const _{ .surface = water };
    static MS const X{ .surface = land, .ground = grassland };
    static MS const s{ .surface = water, .sea_lane = true };

    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7 8 9
      0*/ X,_,_,_,_,_,_,_,_,X, /*0
      1*/ X,_,_,_,_,_,_,s,_,X, /*1
      2*/ X,_,X,X,X,X,X,X,_,X, /*2
      3*/ X,_,X,X,X,X,X,X,_,X, /*3
      4*/ X,_,X,X,X,X,X,X,_,X, /*4
      5*/ X,_,X,X,X,X,X,X,_,X, /*5
      6*/ X,_,X,X,X,X,X,X,_,X, /*6
      7*/ X,_,X,X,X,X,X,X,_,X, /*7
      8*/ X,_,_,_,_,_,_,_,_,X, /*8
      9*/ X,_,_,_,_,_,_,_,_,X, /*9
          0 1 2 3 4 5 6 7 8 9
    */};
    // clang-format on

    w.build_map( std::move( tiles ), 10 );
  }

  // No launch points available.
  registry.units.clear();
  set_unit_pos( { .x = 5, .y = 1 } );
  expected = EvolveGoto::abort{};
  REQUIRE( f() == expected );
}

TEST_CASE( "[goto] evolve_goto_human" ) {
  world w;
}

} // namespace
} // namespace rn
