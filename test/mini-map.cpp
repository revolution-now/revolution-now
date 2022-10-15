/****************************************************************
**mini-map.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-10-14.
*
* Description: Unit tests for the src/mini-map.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/mini-map.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "ss/land-view.rds.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_default_player(); }

  void create_map( Delta size ) {
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles;
    for( int i = 0; i < size.w * size.h; ++i )
      tiles.push_back( L );
    build_map( std::move( tiles ), size.w );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[mini-map] 7x7 map, 5x5 mini-map, 3x3 viewport" ) {
  World       W;
  Delta const world_size_tiles{ .w = 7, .h = 7 };
  Delta const mini_map_size_tiles{ .w = 5, .h = 5 };
  Delta const viewport_size_tiles{ .w = 3, .h = 3 };
  W.create_map( world_size_tiles );
  MiniMap mm( W.ss(),
              mini_map_size_tiles * 2 /*pixels per tile*/ );
  mm.set_animation_speed( 1.0 );
  CHECK( W.land_view().viewport.get_zoom() == 1.0 );
  W.land_view().viewport.advance_state(
      Rect::from( Coord{}, viewport_size_tiles ) * 32 );

  SECTION( "mini-map origin 0,0" ) {
    SECTION( "viewport not moved" ) {
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 0, .y = 0 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to -1,-2 tiles" ) {
      // Should have no effect.
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = -32, .h = -64 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 0, .y = 0 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to 1.5,1.0 tiles" ) {
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = 32 + 16, .h = 32 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 1.5, .y = 1 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to 4,4 tiles" ) {
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = 32 * 4, .h = 32 * 4 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 4, .y = 4 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to 5,5 tiles" ) {
      // Should be the same as previous.
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = 32 * 4, .h = 32 * 4 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 4, .y = 4 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    // The remainder are things that are (supposed to be) inde-
    // pendent of the viewport position, so we will test them and
    // we do so after the viewport movement has been done so that
    // we can ensure that moving the viewport does not affect
    // them.
    REQUIRE( mm.size_screen_pixels() ==
             gfx::size{ .w = 10, .h = 10 } );
    REQUIRE( mm.tiles_visible_on_minimap() ==
             gfx::drect{ .origin = { .x = 0, .y = 0 },
                         .size   = { .w = 5, .h = 5 } } );
    REQUIRE( mm.origin() == gfx::dpoint{ .x = 0, .y = 0 } );
  }

  SECTION( "mini-map origin 1,1" ) {
    mm.set_origin( gfx::dpoint{ .x = 1.0, .y = 1.0 } ); // tiles

    SECTION( "viewport not moved" ) {
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 0, .y = 0 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to -1,-2 tiles" ) {
      // Should have no effect.
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = -32, .h = -64 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 0, .y = 0 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to 1.5,1.0 tiles" ) {
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = 32 + 16, .h = 32 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 1.5, .y = 1 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to 4,4 tiles" ) {
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = 32 * 4, .h = 32 * 4 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 4, .y = 4 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    SECTION( "viewport scrolled to 5,5 tiles" ) {
      // Should be the same as previous.
      W.land_view().viewport.pan_by_world_coords(
          Delta{ .w = 32 * 4, .h = 32 * 4 } );
      REQUIRE( mm.fractional_tiles_inside_white_box() ==
               gfx::drect{ .origin = { .x = 4, .y = 4 },
                           .size   = { .w = 3, .h = 3 } } );
    }

    // The remainder are things that are (supposed to be) inde-
    // pendent of the viewport position, so we will test them and
    // we do so after the viewport movement has been done so that
    // we can ensure that moving the viewport does not affect
    // them.
    REQUIRE( mm.size_screen_pixels() ==
             gfx::size{ .w = 10, .h = 10 } );
    REQUIRE( mm.tiles_visible_on_minimap() ==
             gfx::drect{ .origin = { .x = 1, .y = 1 },
                         .size   = { .w = 5, .h = 5 } } );
    REQUIRE( mm.origin() == gfx::dpoint{ .x = 1, .y = 1 } );
  }
}

TEST_CASE( "[mini-map] drag_box" ) {
  World       W;
  Delta const world_size_tiles{ .w = 11, .h = 11 };
  Delta const mini_map_size_tiles{ .w = 7, .h = 7 };
  Delta const viewport_size_tiles{ .w = 3, .h = 3 };
  W.create_map( world_size_tiles );
  MiniMap mm( W.ss(),
              mini_map_size_tiles * 2 /*pixels per tile*/ );
  mm.set_animation_speed( 1.0 );
  CHECK( W.land_view().viewport.get_zoom() == 1.0 );
  W.land_view().viewport.advance_state(
      Rect::from( Coord{}, viewport_size_tiles ) * 32 );

  // Move mini-map origin 2,2 and viewport to 4,4.
  mm.set_origin( gfx::dpoint{ .x = 2.0, .y = 2.0 } ); // tiles
  W.land_view().viewport.pan_by_world_coords(
      Delta{ .w = 32 * 4, .h = 32 * 4 } );

  REQUIRE( mm.size_screen_pixels() ==
           gfx::size{ .w = 14, .h = 14 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.origin() == gfx::dpoint{ .x = 2, .y = 2 } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4.5, .y = 4.5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5.5, .y = 5.5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 6, .y = 6 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2.5, .y = 2.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 6.5, .y = 6.5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 7, .y = 7 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3.5, .y = 3.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 7.5, .y = 7.5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 8, .y = 8 },
                       .size   = { .w = 3, .h = 3 } } );

  // Already at the map edge, no change.
  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 8, .y = 8 },
                       .size   = { .w = 3, .h = 3 } } );

  // Move the view back up to the upper left corner.
  mm.drag_box( gfx::size{ .w = -7, .h = -7 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4.5, .y = 4.5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = -2, .h = -2 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_box( gfx::size{ .w = -10, .h = -10 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 3, .h = 3 } } );
}

TEST_CASE( "[mini-map] drag_box viewport larger than map" ) {
  World       W;
  Delta const world_size_tiles{ .w = 11, .h = 11 };
  Delta const mini_map_size_tiles{ .w = 5, .h = 5 };
  Delta const viewport_size_tiles{ .w = 6, .h = 6 };
  W.create_map( world_size_tiles );
  MiniMap mm( W.ss(),
              mini_map_size_tiles * 2 /*pixels per tile*/ );
  mm.set_animation_speed( 1.0 );
  CHECK( W.land_view().viewport.get_zoom() == 1.0 );
  W.land_view().viewport.advance_state(
      Rect::from( Coord{}, viewport_size_tiles ) * 32 );

  // Move mini-map origin 3,3 and viewport to 2,2.
  mm.set_origin( gfx::dpoint{ .x = 3.0, .y = 3.0 } ); // tiles
  W.land_view().viewport.pan_by_world_coords(
      Delta{ .w = 32 * 2, .h = 32 * 2 } );

  REQUIRE( mm.size_screen_pixels() ==
           gfx::size{ .w = 10, .h = 10 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.origin() == gfx::dpoint{ .x = 3, .y = 3 } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 6, .h = 6 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 2.5, .y = 2.5 },
                       .size   = { .w = 6, .h = 6 } } );

  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 6, .h = 6 } } );

  // Because the viewport is larger than the mini-map, continuing
  // to drag it past this point will just move it further and
  // further off of the mini-map and will not move the mini-map.
  mm.drag_box( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 3.5, .y = 3.5 },
                       .size   = { .w = 6, .h = 6 } } );

  mm.drag_box( gfx::size{ .w = 4, .h = 4 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3, .y = 3 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 6, .h = 6 } } );

  // At this point we have a case where the viewport, which is
  // larger than the mini-map, is at least partially off of the
  // map, meaning that e.g. in the vertical direction its top
  // edge is below the top mini-map edge and its bottom edge is
  // below the bottom mini-map edge, and this will cause the
  // mini-map to scroll to bring the white box more into view.
  mm.advance_auto_pan();
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 6, .h = 6 } } );

  mm.advance_auto_pan();
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 6, .h = 6 } } );

  // At this point the left/top box edges are aligned with the
  // left/top mini-map edges, so we have scrolled sufficiently.
  // Further calls will not change anything.
  mm.advance_auto_pan();
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 5, .h = 5 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 6, .h = 6 } } );
}

TEST_CASE( "[mini-map] drag_map" ) {
  World       W;
  Delta const world_size_tiles{ .w = 11, .h = 11 };
  Delta const mini_map_size_tiles{ .w = 7, .h = 7 };
  Delta const viewport_size_tiles{ .w = 3, .h = 3 };
  W.create_map( world_size_tiles );
  MiniMap mm( W.ss(),
              mini_map_size_tiles * 2 /*pixels per tile*/ );
  mm.set_animation_speed( 1.0 );
  CHECK( W.land_view().viewport.get_zoom() == 1.0 );
  W.land_view().viewport.advance_state(
      Rect::from( Coord{}, viewport_size_tiles ) * 32 );

  // Move mini-map origin 2,2 and viewport to 5,5.
  mm.set_origin( gfx::dpoint{ .x = 2.0, .y = 2.0 } ); // tiles
  W.land_view().viewport.pan_by_world_coords(
      Delta{ .w = 32 * 5, .h = 32 * 5 } );

  REQUIRE( mm.size_screen_pixels() ==
           gfx::size{ .w = 14, .h = 14 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.origin() == gfx::dpoint{ .x = 2, .y = 2 } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 1.5, .y = 1.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 1.0, .y = 1.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 5, .y = 5 },
                       .size   = { .w = 3, .h = 3 } } );

  // The box starts getting pushed here.

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.5, .y = 0.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4.5, .y = 4.5 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.5, .y = 0.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 1.0, .y = 1.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 1.5, .y = 1.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2.0, .y = 2.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2.5, .y = 2.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3.0, .y = 3.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 3.5, .y = 3.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4.0, .y = 4.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.drag_map( gfx::size{ .w = -1, .h = -1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 4.0, .y = 4.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 4, .y = 4 },
                       .size   = { .w = 3, .h = 3 } } );
}

TEST_CASE( "[mini-map] drag_map large viewport" ) {
  World       W;
  Delta const world_size_tiles{ .w = 11, .h = 11 };
  Delta const mini_map_size_tiles{ .w = 7, .h = 7 };
  Delta const viewport_size_tiles{ .w = 9, .h = 9 };
  W.create_map( world_size_tiles );
  MiniMap mm( W.ss(),
              mini_map_size_tiles * 2 /*pixels per tile*/ );
  mm.set_animation_speed( 1.0 );
  CHECK( W.land_view().viewport.get_zoom() == 1.0 );
  W.land_view().viewport.advance_state(
      Rect::from( Coord{}, viewport_size_tiles ) * 32 );

  // Move mini-map origin 2,2 and viewport to 1,1.
  mm.set_origin( gfx::dpoint{ .x = 2.0, .y = 2.0 } ); // tiles
  W.land_view().viewport.pan_by_world_coords(
      Delta{ .w = 32 * 1, .h = 32 * 1 } );

  REQUIRE( mm.size_screen_pixels() ==
           gfx::size{ .w = 14, .h = 14 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.origin() == gfx::dpoint{ .x = 2, .y = 2 } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 1, .y = 1 },
                       .size   = { .w = 9, .h = 9 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 1.5, .y = 1.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 1, .y = 1 },
                       .size   = { .w = 9, .h = 9 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 1.0, .y = 1.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 1, .y = 1 },
                       .size   = { .w = 9, .h = 9 } } );

  // At this point the left/top of the box start getting
  // "pulled", which makes the viewport go left/up.

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.5, .y = 0.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0.5, .y = 0.5 },
                       .size   = { .w = 9, .h = 9 } } );

  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 9, .h = 9 } } );

  // No change.
  mm.drag_map( gfx::size{ .w = 1, .h = 1 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 9, .h = 9 } } );
}

TEST_CASE( "[mini-map] auto pan with small viewport" ) {
  World       W;
  Delta const world_size_tiles{ .w = 11, .h = 11 };
  Delta const mini_map_size_tiles{ .w = 7, .h = 7 };
  Delta const viewport_size_tiles{ .w = 3, .h = 3 };
  W.create_map( world_size_tiles );
  MiniMap mm( W.ss(),
              mini_map_size_tiles * 2 /*pixels per tile*/ );
  mm.set_animation_speed( 1.5 );
  CHECK( W.land_view().viewport.get_zoom() == 1.0 );
  W.land_view().viewport.advance_state(
      Rect::from( Coord{}, viewport_size_tiles ) * 32 );

  // Move mini-map origin 2,2 and viewport to 0,0.
  mm.set_origin( gfx::dpoint{ .x = 2.0, .y = 2.0 } ); // tiles

  REQUIRE( mm.size_screen_pixels() ==
           gfx::size{ .w = 14, .h = 14 } );
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 2, .y = 2 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.origin() == gfx::dpoint{ .x = 2, .y = 2 } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 3, .h = 3 } } );

  // At this point the viewport is partially outside of the
  // mini-map's view, but it is smaller than the mini-map, so the
  // auto pan will bring it into view. Note that above we've set
  // the animation speed to 1.5.

  mm.advance_auto_pan();
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.5, .y = 0.5 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 3, .h = 3 } } );

  mm.advance_auto_pan();
  REQUIRE( mm.tiles_visible_on_minimap() ==
           gfx::drect{ .origin = { .x = 0.0, .y = 0.0 },
                       .size   = { .w = 7, .h = 7 } } );
  REQUIRE( mm.fractional_tiles_inside_white_box() ==
           gfx::drect{ .origin = { .x = 0, .y = 0 },
                       .size   = { .w = 3, .h = 3 } } );
}

} // namespace
} // namespace rn
