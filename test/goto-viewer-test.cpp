/****************************************************************
**goto-viewer-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-27.
*
* Description: Unit tests for the goto-viewer module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/goto-viewer.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocks/ivisibility.hpp"

// ss
#include "ss/ref.hpp"

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
    create_default_map();
  }

  void create_default_map() {
    static MapSquare const _ = make_ocean();
    static MapSquare const X = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{ /*
          0 1 2 3 4 5 6 7
      0*/ _,X,X,X,X,X,X,_, /*0
      1*/ _,X,X,X,X,X,X,_, /*1
      2*/ _,X,X,X,X,X,X,_, /*2
      3*/ _,X,X,X,X,X,X,_, /*3
      4*/ _,X,X,X,X,X,X,_, /*4
      5*/ _,X,X,X,X,X,X,_, /*5
      6*/ _,X,X,X,X,X,X,_, /*6
      7*/ _,X,X,X,X,X,X,_, /*7
          0 1 2 3 4 5 6 7
    */};
    // clang-format on
    build_map( std::move( tiles ), 8 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[goto-viewer] can_enter_tile" ) {
  world w;
}

TEST_CASE( "[goto-viewer] map_side" ) {
  world w;
  MockIVisibility viz( w.ss().as_const );
  GotoMapViewer const viewer( w.ss(), viz,
                              w.default_player_type(),
                              e_unit_type::free_colonist );
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return viewer.map_side( tile );
  };

  using enum e_map_side;

  tile = { .x = 0, .y = 2 };
  REQUIRE( f() == pacific );
  tile = { .x = 1, .y = 2 };
  REQUIRE( f() == pacific );
  tile = { .x = 2, .y = 2 };
  REQUIRE( f() == pacific );
  tile = { .x = 3, .y = 2 };
  REQUIRE( f() == pacific );
  tile = { .x = 4, .y = 2 };
  REQUIRE( f() == atlantic );
  tile = { .x = 5, .y = 2 };
  REQUIRE( f() == atlantic );
  tile = { .x = 6, .y = 2 };
  REQUIRE( f() == atlantic );
  tile = { .x = 7, .y = 2 };
  REQUIRE( f() == atlantic );
  tile = { .x = 8, .y = 2 };
  REQUIRE( f() == atlantic );

  tile = { .x = 3, .y = 0 };
  REQUIRE( f() == pacific );
  tile = { .x = 3, .y = 7 };
  REQUIRE( f() == pacific );
  tile = { .x = 4, .y = 0 };
  REQUIRE( f() == atlantic );
  tile = { .x = 4, .y = 7 };
  REQUIRE( f() == atlantic );
}

TEST_CASE( "[goto-viewer] is_on_map_side_edge" ) {
  world w;
  MockIVisibility viz( w.ss().as_const );
  GotoMapViewer const viewer( w.ss(), viz,
                              w.default_player_type(),
                              e_unit_type::free_colonist );
  point tile;

  auto const f = [&] [[clang::noinline]] {
    return viewer.is_on_map_side_edge( tile );
  };

  using enum e_map_side_edge;

  tile = { .x = 0, .y = 2 };
  REQUIRE( f() == pacific );
  tile = { .x = 1, .y = 2 };
  REQUIRE( f() == none );
  tile = { .x = 2, .y = 2 };
  REQUIRE( f() == none );
  tile = { .x = 3, .y = 2 };
  REQUIRE( f() == none );
  tile = { .x = 4, .y = 2 };
  REQUIRE( f() == none );
  tile = { .x = 5, .y = 2 };
  REQUIRE( f() == none );
  tile = { .x = 6, .y = 2 };
  REQUIRE( f() == none );
  tile = { .x = 7, .y = 2 };
  REQUIRE( f() == atlantic );
  tile = { .x = 8, .y = 2 };
  REQUIRE( f() == none );

  tile = { .x = 3, .y = 0 };
  REQUIRE( f() == none );
  tile = { .x = 3, .y = 7 };
  REQUIRE( f() == none );
}

TEST_CASE( "[goto-viewer] is_sea_lane" ) {
  world w;
  MockIVisibility viz( w.ss().as_const );
  GotoMapViewer const viewer( w.ss(), viz,
                              w.default_player_type(),
                              e_unit_type::free_colonist );
  point const tile{ .x = 2, .y = 3 };

  MapSquare const kNoSeaLane;
  MapSquare const kSeaLane{ .sea_lane = true };

  auto const f = [&] [[clang::noinline]] {
    return viewer.is_sea_lane( tile );
  };

  using enum e_tile_visibility;

  viz.EXPECT__visible( tile ).returns( hidden );
  REQUIRE( f() == nothing );

  viz.EXPECT__visible( tile ).returns( fogged );
  viz.EXPECT__square_at( tile ).returns( kNoSeaLane );
  REQUIRE( f() == false );

  viz.EXPECT__visible( tile ).returns( fogged );
  viz.EXPECT__square_at( tile ).returns( kSeaLane );
  REQUIRE( f() == true );

  viz.EXPECT__visible( tile ).returns( clear );
  viz.EXPECT__square_at( tile ).returns( kNoSeaLane );
  REQUIRE( f() == false );

  viz.EXPECT__visible( tile ).returns( clear );
  viz.EXPECT__square_at( tile ).returns( kSeaLane );
  REQUIRE( f() == true );
}

TEST_CASE( "[goto-viewer] has_lcr" ) {
  world w;
  MockIVisibility viz( w.ss().as_const );
  GotoMapViewer const viewer( w.ss(), viz,
                              w.default_player_type(),
                              e_unit_type::free_colonist );
  point const tile{ .x = 2, .y = 3 };

  MapSquare const kNoLcr;
  MapSquare const kLcr{ .lost_city_rumor = true };

  auto const f = [&] [[clang::noinline]] {
    return viewer.has_lcr( tile );
  };

  using enum e_tile_visibility;

  viz.EXPECT__visible( tile ).returns( hidden );
  REQUIRE( f() == nothing );

  viz.EXPECT__visible( tile ).returns( fogged );
  viz.EXPECT__square_at( tile ).returns( kNoLcr );
  REQUIRE( f() == false );

  viz.EXPECT__visible( tile ).returns( fogged );
  viz.EXPECT__square_at( tile ).returns( kLcr );
  REQUIRE( f() == true );

  viz.EXPECT__visible( tile ).returns( clear );
  viz.EXPECT__square_at( tile ).returns( kNoLcr );
  REQUIRE( f() == false );

  viz.EXPECT__visible( tile ).returns( clear );
  viz.EXPECT__square_at( tile ).returns( kLcr );
  REQUIRE( f() == true );
}

TEST_CASE( "[goto-viewer] movement_points_required" ) {
  world w;
  MockIVisibility viz( w.ss().as_const );

  point const src{ .x = 2, .y = 3 };
  e_direction d = {};
  point dst;

  e_unit_type unit_type = {};

  MapSquare src_square;
  MapSquare dst_square;

  auto const f = [&] [[clang::noinline]] {
    GotoMapViewer const viewer(
        w.ss(), viz, w.default_player_type(), unit_type );
    return viewer.movement_points_required( src, d );
  };

  using enum e_tile_visibility;
  using enum e_unit_type;
  using enum e_land_overlay;
  using enum e_surface;
  using enum e_river;

  d   = e_direction::se;
  dst = src.moved( d );

  // Neither visible.
  viz.EXPECT__visible( src ).returns( hidden );
  viz.EXPECT__visible( dst ).returns( hidden );
  REQUIRE( f() == nothing );

  // One visible.
  viz.EXPECT__visible( src ).returns( fogged );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__visible( dst ).returns( hidden );
  REQUIRE( f() == nothing );

  // One visible.
  viz.EXPECT__visible( src ).returns( hidden );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == nothing );

  // Both visible, default squares.
  unit_type = free_colonist;
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints( 1 ) );

  // Onto mountain, free_colonist.
  unit_type  = free_colonist;
  src_square = { .surface = land };
  dst_square = { .surface = land, .overlay = mountains };
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints( 1 ) );

  // Onto mountain, dragoon.
  unit_type  = dragoon;
  src_square = { .surface = land };
  dst_square = { .surface = land, .overlay = mountains };
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints( 3 ) );

  // Onto mountain, dragoon, road on dst.
  unit_type  = dragoon;
  src_square = { .surface = land };
  dst_square = {
    .surface = land, .overlay = mountains, .road = true };
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints( 3 ) );

  // Onto mountain, dragoon, road on both.
  unit_type  = dragoon;
  src_square = { .surface = land, .road = true };
  dst_square = {
    .surface = land, .overlay = mountains, .road = true };
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints::_1_3() );

  // Onto mountain, dragoon, river on both, not cardinal.
  unit_type  = dragoon;
  src_square = { .surface = land, .river = major };
  dst_square = {
    .surface = land, .overlay = mountains, .river = major };
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints( 3 ) );

  // Onto mountain, dragoon, river on both, cardinal.
  d          = e_direction::e;
  dst        = src.moved( d );
  unit_type  = dragoon;
  src_square = { .surface = land, .river = major };
  dst_square = {
    .surface = land, .overlay = mountains, .river = major };
  viz.EXPECT__visible( src ).returns( clear );
  viz.EXPECT__visible( dst ).returns( clear );
  viz.EXPECT__square_at( src ).returns( src_square );
  viz.EXPECT__square_at( dst ).returns( dst_square );
  REQUIRE( f() == MovementPoints::_1_3() );
}

TEST_CASE( "[goto-viewer] minimum_heuristic_tile_cost" ) {
  world w;
  MockIVisibility const viz( w.ss().as_const );

  auto const f =
      [&] [[clang::noinline]] ( e_unit_type const unit_type ) {
        GotoMapViewer const viewer(
            w.ss(), viz, w.default_player_type(), unit_type );
        return viewer.minimum_heuristic_tile_cost();
      };

  using enum e_unit_type;

  REQUIRE( f( free_colonist ) == MovementPoints::_1_3() );
  REQUIRE( f( wagon_train ) == MovementPoints::_1_3() );
  REQUIRE( f( dragoon ) == MovementPoints::_1_3() );
  REQUIRE( f( scout ) == MovementPoints::_1_3() );
  REQUIRE( f( caravel ) == MovementPoints( 1 ) );
  REQUIRE( f( man_o_war ) == MovementPoints( 1 ) );
  REQUIRE( f( merchantman ) == MovementPoints( 1 ) );
}

} // namespace
} // namespace rn
