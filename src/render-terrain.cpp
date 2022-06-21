/****************************************************************
**render-terrain.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Renders individual terrain squares.
*
*****************************************************************/
#include "render-terrain.hpp"

// Revolution Now
#include "error.hpp"
#include "game-state.hpp"
#include "gs-terrain.hpp"
#include "logger.hpp"
#include "lua.hpp"
#include "plow.hpp"
#include "renderer.hpp" // FIXME: remove
#include "road.hpp"
#include "tiles.hpp"

// render
#include "render/renderer.hpp"

// luapp
#include "luapp/state.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

bool g_show_grid = false;

constexpr double g_tile_overlap_scaling       = .8;
constexpr double g_tile_overlap_width_percent = .2;

constexpr double g_tile_overlap_stage_two_alpha = .5;
constexpr double g_tile_overlap_stage_two_stage = .85;

// This will get applied multiplicatively after the stage two al-
// pha, so we need to divide it to yield the end value that we
// want.
constexpr double g_tile_overlap_stage_one_alpha =
    .85 / g_tile_overlap_stage_two_alpha;
constexpr double g_tile_overlap_stage_one_stage = .70;

e_tile tile_for_ground_terrain( e_ground_terrain terrain ) {
  switch( terrain ) {
    case e_ground_terrain::arctic: return e_tile::terrain_arctic;
    case e_ground_terrain::desert: return e_tile::terrain_desert;
    case e_ground_terrain::grassland:
      return e_tile::terrain_grassland;
    case e_ground_terrain::marsh: return e_tile::terrain_marsh;
    case e_ground_terrain::plains: return e_tile::terrain_plains;
    case e_ground_terrain::prairie:
      return e_tile::terrain_prairie;
    case e_ground_terrain::savannah:
      return e_tile::terrain_savannah;
    case e_ground_terrain::swamp: return e_tile::terrain_swamp;
    case e_ground_terrain::tundra: return e_tile::terrain_tundra;
  }
}

maybe<e_ground_terrain> ground_terrain_for_square(
    TerrainState const& terrain_state, MapSquare const& square,
    Coord world_square ) {
  if( square.surface == e_surface::land ) return square.ground;
  // We have a water so get it from the surroundings.
  MapSquare const& left =
      terrain_state.total_square_at( world_square - 1_w );
  if( left.surface == e_surface::land ) return left.ground;

  MapSquare const& up =
      terrain_state.total_square_at( world_square - 1_h );
  if( up.surface == e_surface::land ) return up.ground;

  MapSquare const& right =
      terrain_state.total_square_at( world_square + 1_w );
  if( right.surface == e_surface::land ) return right.ground;

  MapSquare const& down =
      terrain_state.total_square_at( world_square + 1_h );
  if( down.surface == e_surface::land ) return down.ground;

  MapSquare const& up_left =
      terrain_state.total_square_at( world_square - 1_w - 1_h );
  if( up_left.surface == e_surface::land ) return up_left.ground;

  MapSquare const& up_right =
      terrain_state.total_square_at( world_square - 1_h + 1_w );
  if( up_right.surface == e_surface::land )
    return up_right.ground;

  MapSquare const& down_right =
      terrain_state.total_square_at( world_square + 1_w + 1_h );
  if( down_right.surface == e_surface::land )
    return down_right.ground;

  MapSquare const& down_left =
      terrain_state.total_square_at( world_square + 1_h - 1_w );
  if( down_left.surface == e_surface::land )
    return down_left.ground;

  return nothing;
}

void render_forest( TerrainState const& terrain_state,
                    rr::Painter& painter, Coord where,
                    Coord world_square ) {
  MapSquare const& here =
      terrain_state.total_square_at( world_square );
  DCHECK( here.surface == e_surface::land );
  if( here.ground == e_ground_terrain::desert ) {
    render_sprite( painter, where,
                   e_tile::terrain_forest_scrub_island );
    return;
  }

  // Returns true if the the tile exists, it is land, it is
  // non-desert, and it has a forest.
  auto is_forest = [&]( e_direction d ) {
    MapSquare const& s =
        terrain_state.total_square_at( world_square.moved( d ) );
    return s.surface == e_surface::land &&
           s.overlay == e_land_overlay::forest &&
           s.ground != e_ground_terrain::desert;
  };

  bool has_left  = is_forest( e_direction::w );
  bool has_up    = is_forest( e_direction::n );
  bool has_right = is_forest( e_direction::e );
  bool has_down  = is_forest( e_direction::s );

  // 0000abcd:
  // a=forest up, b=forest right, c=forest down, d=forest left.
  int mask = ( has_up ? ( 1 << 3 ) : 0 ) |
             ( has_right ? ( 1 << 2 ) : 0 ) |
             ( has_down ? ( 1 << 1 ) : 0 ) |
             ( has_left ? ( 1 << 0 ) : 0 );

  e_tile forest_tile = {};

  switch( mask ) {
    case 0b0001: {
      // forest on left.
      forest_tile = e_tile::terrain_forest_left;
      break;
    }
    case 0b1000: {
      // forest on top.
      forest_tile = e_tile::terrain_forest_up;
      break;
    }
    case 0b0100: {
      // forest on right.
      forest_tile = e_tile::terrain_forest_right;
      break;
    }
    case 0b0010: {
      // forest on bottom.
      forest_tile = e_tile::terrain_forest_down;
      break;
    }
    case 0b1001: {
      // forest on left and top.
      forest_tile = e_tile::terrain_forest_left_up;
      break;
    }
    case 0b1100: {
      // forest on top and right.
      forest_tile = e_tile::terrain_forest_up_right;
      break;
    }
    case 0b0110: {
      // forest on right and bottom.
      forest_tile = e_tile::terrain_forest_right_down;
      break;
    }
    case 0b0011: {
      // forest on bottom and left.
      forest_tile = e_tile::terrain_forest_down_left;
      break;
    }
    case 0b0101: {
      // forest on left and right.
      forest_tile = e_tile::terrain_forest_left_right;
      break;
    }
    case 0b1010: {
      // forest on top and bottom.
      forest_tile = e_tile::terrain_forest_up_down;
      break;
    }
    case 0b0111: {
      // forest on right, bottom, left.
      forest_tile = e_tile::terrain_forest_right_down_left;
      break;
    }
    case 0b1011: {
      // forest on bottom, left, top.
      forest_tile = e_tile::terrain_forest_down_left_up;
      break;
    }
    case 0b1101: {
      // forest on left, top, right.
      forest_tile = e_tile::terrain_forest_left_up_right;
      break;
    }
    case 0b1110: {
      // forest on top, right, bottom.
      forest_tile = e_tile::terrain_forest_up_right_down;
      break;
    }
    case 0b1111:
      // forest on all sides.
      forest_tile = e_tile::terrain_forest_all;
      break;
    case 0b0000:
      // forest on no sides.
      forest_tile = e_tile::terrain_forest_island;
      break;
    default: {
      FATAL( "invalid forest mask: {}", mask );
    }
  }
  render_sprite( painter, where, forest_tile );
}

e_tile overlay_tile( MapSquare const& square ) {
  DCHECK( square.overlay.has_value() );
  switch( *square.overlay ) {
    case e_land_overlay::forest: {
      SHOULD_NOT_BE_HERE;
    }
    case e_land_overlay::hills:
      return e_tile::terrain_hills_island;
    case e_land_overlay::mountains:
      return e_tile::terrain_mountain_island;
  }
}

// Note that in this function the anchor needs to be different
// for each segment other wise overlaping segments (in the cor-
// ners) will depixelate the exact same pixels and only one will
// be visible.
void render_adjacent_overlap( TerrainState const& terrain_state,
                              rr::Renderer&       renderer,
                              Coord where, Coord world_square,
                              double chop_percent,
                              Delta  anchor_offset ) {
  MapSquare const& west =
      terrain_state.total_square_at( world_square - 1_w );
  MapSquare const& north =
      terrain_state.total_square_at( world_square - 1_h );
  MapSquare const& east =
      terrain_state.total_square_at( world_square + 1_w );
  MapSquare const& south =
      terrain_state.total_square_at( world_square + 1_h );

  int chop_pixels =
      std::lround( g_tile_delta.w._ * chop_percent );
  W chop_w = W{ chop_pixels };
  H chop_h = H{ chop_pixels };

  {
    // Render east part of western tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.w -= chop_w;
    src.x += chop_w;
    dst.x += 0_w;
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.anchor,
                             dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, west, world_square - 1_w );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }

  {
    // Render bottom part of north tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.h -= chop_h;
    src.y += chop_h;
    dst.y += 0_h;
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.anchor,
                             dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, north, world_square - 1_h );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }

  {
    // Render northern part of southern tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.h -= chop_h;
    src.y += 0_h;
    dst.y += chop_h;
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.anchor,
                             dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, south, world_square + 1_h );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }

  {
    // Render west part of eastern tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.w -= chop_w;
    src.x += 0_w;
    dst.x += chop_w;
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.anchor,
                             dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, east, world_square + 1_w );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }
}

void render_terrain_ground( TerrainState const& terrain_state,
                            rr::Painter&        painter,
                            rr::Renderer& renderer, Coord where,
                            Coord            world_square,
                            e_ground_terrain ground ) {
  e_tile tile = tile_for_ground_terrain( ground );
  render_sprite( painter, where, tile );
  // This will ensure good pseudo (deterministic) randomization
  // of the dithering from one tile to another.
  //
  // It's value is kind of arbitrary, but it should satisfy these
  // requirements:
  //
  //   1. Its value must not depend on `where`, i.e., the screen
  //      coordinate where we are rendering, otherwise the effect
  //      would appear to change as the map is scrolled.
  //   2. It should be different for each square (or at least ap-
  //      proximately.
  //   3. It should be different for the two overlap stages below
  //      (this is most important of the three).
  //   4. It should be in the range of screen coordinates because
  //      that is what the hash function in the fragment shader
  //      is calibrated for.
  //
  Delta const anchor_offset =
      Scale{ 10 } * ( world_square % Scale{ 10 } );
  {
#if 1
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             g_tile_overlap_stage_two_alpha );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             g_tile_overlap_stage_two_stage );
    render_adjacent_overlap(
        terrain_state, renderer, where, world_square,
        /*chop_percent=*/
        clamp( 1.0 - g_tile_overlap_width_percent *
                         g_tile_overlap_scaling,
               0.0, 1.0 ),
        anchor_offset );
#endif
#if 1
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             g_tile_overlap_stage_one_alpha );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             g_tile_overlap_stage_one_stage );
    render_adjacent_overlap(
        terrain_state, renderer, where, world_square,
        /*chop_percent=*/
        clamp( 1.0 - ( g_tile_overlap_width_percent / 2.0 ) *
                         g_tile_overlap_scaling,
               0.0, 1.0 ),
        anchor_offset + g_tile_delta );
#endif
  }

  MapSquare const& left =
      terrain_state.total_square_at( world_square - 1_w );
  MapSquare const& up =
      terrain_state.total_square_at( world_square - 1_h );
  MapSquare const& right =
      terrain_state.total_square_at( world_square + 1_w );
  MapSquare const& up_left =
      terrain_state.total_square_at( world_square - 1_h - 1_w );
  MapSquare const& up_right =
      terrain_state.total_square_at( world_square + 1_w - 1_h );

  // This should be done at the end.
  if( up_right.surface == e_surface::land &&
      up.surface == e_surface::water &&
      right.surface == e_surface::water ) {
    render_sprite_stencil(
        painter, where,
        e_tile::terrain_ocean_canal_corner_up_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite(
        painter, where,
        e_tile::terrain_border_canal_corner_up_right );
  }
  if( up_left.surface == e_surface::land &&
      up.surface == e_surface::water &&
      left.surface == e_surface::water ) {
    render_sprite_stencil(
        painter, where,
        e_tile::terrain_ocean_canal_corner_up_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( painter, where,
                   e_tile::terrain_border_canal_corner_up_left );
  }
}

// Pass in the painter as well for efficiency.
void render_terrain_land_square(
    TerrainState const& terrain_state, rr::Painter& painter,
    rr::Renderer& renderer, Coord where, Coord world_square,
    MapSquare const& square ) {
  DCHECK( square.surface == e_surface::land );
  render_terrain_ground( terrain_state, painter, renderer, where,
                         world_square, square.ground );
  if( square.overlay == e_land_overlay::forest ) {
    render_forest( terrain_state, painter, where, world_square );
  } else if( square.overlay.has_value() ) {
    e_tile overlay = overlay_tile( square );
    render_sprite( painter, where, overlay );
  }
}

void render_beach_corners(
    rr::Painter& painter, Coord where, MapSquare const& up,
    MapSquare const& right, MapSquare const& down,
    MapSquare const& left, MapSquare const& up_left,
    MapSquare const& up_right, MapSquare const& down_right,
    MapSquare const& down_left ) {
  // Render beach corners.
  if( up_left.surface == e_surface::land &&
      left.surface == e_surface::water &&
      up.surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_up_left );
  if( up_right.surface == e_surface::land &&
      up.surface == e_surface::water &&
      right.surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_up_right );
  if( down_right.surface == e_surface::land &&
      down.surface == e_surface::water &&
      right.surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_down_right );
  if( down_left.surface == e_surface::land &&
      down.surface == e_surface::water &&
      left.surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_down_left );
}

void render_river( TerrainState const& terrain_state,
                   rr::Renderer& renderer, Coord where,
                   Coord            world_square,
                   MapSquare const& square ) {
  DCHECK( square.river.has_value() );
  MapSquare const& up =
      terrain_state.total_square_at( world_square - 1_h );
  MapSquare const& right =
      terrain_state.total_square_at( world_square + 1_w );
  MapSquare const& down =
      terrain_state.total_square_at( world_square + 1_h );
  MapSquare const& left =
      terrain_state.total_square_at( world_square - 1_w );

  // Treat off-map tiles as water for rendering purposes.
  bool river_up    = up.river.has_value();
  bool river_right = right.river.has_value();
  bool river_down  = down.river.has_value();
  bool river_left  = left.river.has_value();

  // 0000abcd:
  // a=river left, b=river up, c=river right, d=river down.
  int mask = ( river_left ? ( 1 << 3 ) : 0 ) |
             ( river_up ? ( 1 << 2 ) : 0 ) |
             ( river_right ? ( 1 << 1 ) : 0 ) |
             ( river_down ? ( 1 << 0 ) : 0 );

  e_tile minor_river_tile = {};
  e_tile major_river_tile = {};
  e_tile minor_cycle_tile = {};
  e_tile major_cycle_tile = {};
  e_tile minor_bank_tile  = {};
  e_tile major_bank_tile  = {};

  switch( mask ) {
    case 0b1000:
      // river left.
      minor_river_tile = e_tile::terrain_river_minor_left;
      minor_bank_tile  = e_tile::terrain_river_bank_minor_left;
      major_river_tile = e_tile::terrain_river_major_left;
      major_bank_tile  = e_tile::terrain_river_bank_major_left;
      minor_cycle_tile = e_tile::terrain_river_cycle_minor_left;
      major_cycle_tile = e_tile::terrain_river_cycle_major_left;
      break;
    case 0b0100:
      // river up.
      minor_river_tile = e_tile::terrain_river_minor_up;
      minor_bank_tile  = e_tile::terrain_river_bank_minor_up;
      major_river_tile = e_tile::terrain_river_major_up;
      major_bank_tile  = e_tile::terrain_river_bank_major_up;
      minor_cycle_tile = e_tile::terrain_river_cycle_minor_up;
      major_cycle_tile = e_tile::terrain_river_cycle_major_up;
      break;
    case 0b0010:
      // river right.
      minor_river_tile = e_tile::terrain_river_minor_right;
      minor_bank_tile  = e_tile::terrain_river_bank_minor_right;
      major_river_tile = e_tile::terrain_river_major_right;
      major_bank_tile  = e_tile::terrain_river_bank_major_right;
      minor_cycle_tile = e_tile::terrain_river_cycle_minor_right;
      major_cycle_tile = e_tile::terrain_river_cycle_major_right;
      break;
    case 0b0001:
      // river down.
      minor_river_tile = e_tile::terrain_river_minor_down;
      minor_bank_tile  = e_tile::terrain_river_bank_minor_down;
      major_river_tile = e_tile::terrain_river_major_down;
      major_bank_tile  = e_tile::terrain_river_bank_major_down;
      minor_cycle_tile = e_tile::terrain_river_cycle_minor_down;
      major_cycle_tile = e_tile::terrain_river_cycle_major_down;
      break;
    case 0b1100:
      // river left up.
      minor_river_tile = e_tile::terrain_river_minor_left_up;
      minor_bank_tile = e_tile::terrain_river_bank_minor_left_up;
      major_river_tile = e_tile::terrain_river_major_left_up;
      major_bank_tile = e_tile::terrain_river_bank_major_left_up;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_left_up;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_left_up;
      break;
    case 0b0110:
      // river up right.
      minor_river_tile = e_tile::terrain_river_minor_up_right;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_up_right;
      major_river_tile = e_tile::terrain_river_major_up_right;
      major_bank_tile =
          e_tile::terrain_river_bank_major_up_right;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_up_right;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_up_right;
      break;
    case 0b0011:
      // river right down.
      minor_river_tile = e_tile::terrain_river_minor_right_down;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_right_down;
      major_river_tile = e_tile::terrain_river_major_right_down;
      major_bank_tile =
          e_tile::terrain_river_bank_major_right_down;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_right_down;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_right_down;
      break;
    case 0b1001:
      // river down left.
      minor_river_tile = e_tile::terrain_river_minor_down_left;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_down_left;
      major_river_tile = e_tile::terrain_river_major_down_left;
      major_bank_tile =
          e_tile::terrain_river_bank_major_down_left;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_down_left;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_down_left;
      break;
    case 0b1110:
      // river left up right.
      minor_river_tile =
          e_tile::terrain_river_minor_left_up_right;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_left_up_right;
      major_river_tile =
          e_tile::terrain_river_major_left_up_right;
      major_bank_tile =
          e_tile::terrain_river_bank_major_left_up_right;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_left_up_right;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_left_up_right;
      break;
    case 0b0111:
      // river up right down.
      minor_river_tile =
          e_tile::terrain_river_minor_up_right_down;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_up_right_down;
      major_river_tile =
          e_tile::terrain_river_major_up_right_down;
      major_bank_tile =
          e_tile::terrain_river_bank_major_up_right_down;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_up_right_down;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_up_right_down;
      break;
    case 0b1011:
      // river right down left.
      minor_river_tile =
          e_tile::terrain_river_minor_right_down_left;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_right_down_left;
      major_river_tile =
          e_tile::terrain_river_major_right_down_left;
      major_bank_tile =
          e_tile::terrain_river_bank_major_right_down_left;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_right_down_left;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_right_down_left;
      break;
    case 0b1101:
      // river down left up.
      minor_river_tile =
          e_tile::terrain_river_minor_down_left_up;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_down_left_up;
      major_river_tile =
          e_tile::terrain_river_major_down_left_up;
      major_bank_tile =
          e_tile::terrain_river_bank_major_down_left_up;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_down_left_up;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_down_left_up;
      break;
    case 0b1010:
      // river left right.
      minor_river_tile = e_tile::terrain_river_minor_left_right;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_left_right;
      major_river_tile = e_tile::terrain_river_major_left_right;
      major_bank_tile =
          e_tile::terrain_river_bank_major_left_right;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_left_right;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_left_right;
      break;
    case 0b0101:
      // river up down.
      minor_river_tile = e_tile::terrain_river_minor_up_down;
      minor_bank_tile = e_tile::terrain_river_bank_minor_up_down;
      major_river_tile = e_tile::terrain_river_major_up_down;
      major_bank_tile = e_tile::terrain_river_bank_major_up_down;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_up_down;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_up_down;
      break;
    case 0b1111:
      // river left up right down.
      minor_river_tile =
          e_tile::terrain_river_minor_left_up_right_down;
      minor_bank_tile =
          e_tile::terrain_river_bank_minor_left_up_right_down;
      major_river_tile =
          e_tile::terrain_river_major_left_up_right_down;
      major_bank_tile =
          e_tile::terrain_river_bank_major_left_up_right_down;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_left_up_right_down;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_left_up_right_down;
      break;
    case 0b0000:
      // no rivers around.
      minor_river_tile = e_tile::terrain_river_minor_island;
      minor_bank_tile  = e_tile::terrain_river_bank_minor_island;
      major_river_tile = e_tile::terrain_river_major_island;
      major_bank_tile  = e_tile::terrain_river_bank_major_island;
      minor_cycle_tile =
          e_tile::terrain_river_cycle_minor_island;
      major_cycle_tile =
          e_tile::terrain_river_cycle_major_island;
      break;
    default: FATAL( "invalid river mask: {}", mask );
  }

  e_tile water = ( square.river == e_river::major )
                     ? major_river_tile
                     : minor_river_tile;

  e_tile cycle = ( square.river == e_river::major )
                     ? major_cycle_tile
                     : minor_cycle_tile;

  e_tile bank = ( square.river == e_river::major )
                    ? major_bank_tile
                    : minor_bank_tile;

  rr::Painter painter = renderer.painter();
  render_sprite( painter, where, water );
  {
    SCOPED_RENDERER_MOD_SET( painter_mods.cycling.enabled,
                             true );
    rr::Painter painter = renderer.painter();
    render_sprite( painter, where, cycle );
  }
  render_sprite( painter, where, bank );
}

} // namespace

void render_terrain_ocean_square(
    rr::Renderer& renderer, rr::Painter& painter, Coord where,
    TerrainState const& terrain_state, MapSquare const& square,
    Coord world_square ) {
  DCHECK( square.surface == e_surface::water );

  MapSquare const& up =
      terrain_state.total_square_at( world_square - 1_h );
  MapSquare const& right =
      terrain_state.total_square_at( world_square + 1_w );
  MapSquare const& down =
      terrain_state.total_square_at( world_square + 1_h );
  MapSquare const& left =
      terrain_state.total_square_at( world_square - 1_w );
  MapSquare const& up_left =
      terrain_state.total_square_at( world_square - 1_h - 1_w );
  MapSquare const& up_right =
      terrain_state.total_square_at( world_square + 1_w - 1_h );
  MapSquare const& down_right =
      terrain_state.total_square_at( world_square + 1_h + 1_w );
  MapSquare const& down_left =
      terrain_state.total_square_at( world_square - 1_w + 1_h );

  // Treat off-map tiles as water for rendering purposes.
  bool water_up    = up.surface == e_surface::water;
  bool water_right = right.surface == e_surface::water;
  bool water_down  = down.surface == e_surface::water;
  bool water_left  = left.surface == e_surface::water;

  // 0000abcd:
  // a=water up, b=water right, c=water down, d=water left.
  int mask = ( water_up ? ( 1 << 3 ) : 0 ) |
             ( water_right ? ( 1 << 2 ) : 0 ) |
             ( water_down ? ( 1 << 1 ) : 0 ) |
             ( water_left ? ( 1 << 0 ) : 0 );

  if( mask == 0b1111 ) {
    // All surrounding water.
    e_tile tile = square.sea_lane
                      ? e_tile::terrain_ocean_sea_lane
                      : e_tile::terrain_ocean;

    render_sprite( painter, where, tile );
    render_beach_corners( painter, where, up, right, down, left,
                          up_left, up_right, down_right,
                          down_left );
    return;
  }

  e_tile        water_tile         = {};
  e_tile        beach_tile         = {};
  e_tile        border_tile        = {};
  maybe<e_tile> second_water_tile  = {};
  maybe<e_tile> second_beach_tile  = {};
  maybe<e_tile> second_border_tile = {};
  maybe<e_tile> surf_tile          = {};
  maybe<e_tile> sand_tile          = {};

  auto is_land_if_exists = [&]( e_direction d ) {
    MapSquare const& s =
        terrain_state.total_square_at( world_square.moved( d ) );
    return s.surface == e_surface::land;
  };

  auto to_mask = []( bool l, bool r ) {
    return ( ( l ? 1 : 0 ) << 1 ) | ( r ? 1 : 0 );
  };

  switch( mask ) {
    case 0b1110: {
      // land on left.
      bool top_open  = is_land_if_exists( e_direction::nw );
      bool down_open = is_land_if_exists( e_direction::sw );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile  = e_tile::terrain_ocean_up_right_down_c_c;
          beach_tile  = e_tile::terrain_beach_up_right_down_c_c;
          border_tile = e_tile::terrain_border_up_right_down_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile  = e_tile::terrain_ocean_up_right_down_c_o;
          beach_tile  = e_tile::terrain_beach_up_right_down_c_o;
          border_tile = e_tile::terrain_border_up_right_down_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile  = e_tile::terrain_ocean_up_right_down_o_c;
          beach_tile  = e_tile::terrain_beach_up_right_down_o_c;
          border_tile = e_tile::terrain_border_up_right_down_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile  = e_tile::terrain_ocean_up_right_down_o_o;
          beach_tile  = e_tile::terrain_beach_up_right_down_o_o;
          border_tile = e_tile::terrain_border_up_right_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0111: {
      // land on top.
      bool left_open  = is_land_if_exists( e_direction::nw );
      bool right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile = e_tile::terrain_ocean_right_down_left_c_c;
          beach_tile = e_tile::terrain_beach_right_down_left_c_c;
          border_tile =
              e_tile::terrain_border_right_down_left_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_right_down_left_c_o;
          beach_tile = e_tile::terrain_beach_right_down_left_c_o;
          border_tile =
              e_tile::terrain_border_right_down_left_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_right_down_left_o_c;
          beach_tile = e_tile::terrain_beach_right_down_left_o_c;
          border_tile =
              e_tile::terrain_border_right_down_left_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_right_down_left_o_o;
          beach_tile = e_tile::terrain_beach_right_down_left_o_o;
          border_tile =
              e_tile::terrain_border_right_down_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1011: {
      // land on right.
      bool top_open  = is_land_if_exists( e_direction::ne );
      bool down_open = is_land_if_exists( e_direction::se );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile  = e_tile::terrain_ocean_down_left_up_c_c;
          beach_tile  = e_tile::terrain_beach_down_left_up_c_c;
          border_tile = e_tile::terrain_border_down_left_up_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile  = e_tile::terrain_ocean_down_left_up_c_o;
          beach_tile  = e_tile::terrain_beach_down_left_up_c_o;
          border_tile = e_tile::terrain_border_down_left_up_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile  = e_tile::terrain_ocean_down_left_up_o_c;
          beach_tile  = e_tile::terrain_beach_down_left_up_o_c;
          border_tile = e_tile::terrain_border_down_left_up_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile  = e_tile::terrain_ocean_down_left_up_o_o;
          beach_tile  = e_tile::terrain_beach_down_left_up_o_o;
          border_tile = e_tile::terrain_border_down_left_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1101: {
      // land on bottom.
      bool left_open  = is_land_if_exists( e_direction::sw );
      bool right_open = is_land_if_exists( e_direction::se );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile  = e_tile::terrain_ocean_left_up_right_c_c;
          beach_tile  = e_tile::terrain_beach_left_up_right_c_c;
          border_tile = e_tile::terrain_border_left_up_right_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile  = e_tile::terrain_ocean_left_up_right_c_o;
          beach_tile  = e_tile::terrain_beach_left_up_right_c_o;
          border_tile = e_tile::terrain_border_left_up_right_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile  = e_tile::terrain_ocean_left_up_right_o_c;
          beach_tile  = e_tile::terrain_beach_left_up_right_o_c;
          border_tile = e_tile::terrain_border_left_up_right_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile  = e_tile::terrain_ocean_left_up_right_o_o;
          beach_tile  = e_tile::terrain_beach_left_up_right_o_o;
          border_tile = e_tile::terrain_border_left_up_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0110: {
      // land on left and top.
      bool down_open  = is_land_if_exists( e_direction::sw );
      bool right_open = is_land_if_exists( e_direction::ne );
      bool open_water = !is_land_if_exists( e_direction::se );
      bool apply_sand =
          is_land_if_exists( e_direction::nw ) && open_water;
      switch( to_mask( down_open, right_open ) ) {
        case 0b00:
          // down closed, right closed.
          water_tile  = e_tile::terrain_ocean_right_down_c_c;
          beach_tile  = e_tile::terrain_beach_right_down_c_c;
          border_tile = e_tile::terrain_border_right_down_c_c;
          surf_tile   = e_tile::terrain_surf_right_down_c_c;
          sand_tile   = e_tile::terrain_sand_right_down_c_c;
          break;
        case 0b01:
          // down closed, right open.
          water_tile  = e_tile::terrain_ocean_right_down_o_c;
          beach_tile  = e_tile::terrain_beach_right_down_o_c;
          border_tile = e_tile::terrain_border_right_down_o_c;
          surf_tile   = e_tile::terrain_surf_right_down_o_c;
          sand_tile   = e_tile::terrain_sand_right_down_o_c;
          break;
        case 0b10:
          // down open, right closed.
          water_tile  = e_tile::terrain_ocean_right_down_c_o;
          beach_tile  = e_tile::terrain_beach_right_down_c_o;
          border_tile = e_tile::terrain_border_right_down_c_o;
          surf_tile   = e_tile::terrain_surf_right_down_c_o;
          sand_tile   = e_tile::terrain_sand_right_down_c_o;
          break;
        case 0b11:
          // down open, right open.
          water_tile  = e_tile::terrain_ocean_right_down_o_o;
          beach_tile  = e_tile::terrain_beach_right_down_o_o;
          border_tile = e_tile::terrain_border_right_down_o_o;
          surf_tile   = e_tile::terrain_surf_right_down_o_o;
          sand_tile   = e_tile::terrain_sand_right_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      if( !apply_sand ) sand_tile.reset();
      if( !open_water ) surf_tile.reset();
      break;
    }
    case 0b0011: {
      // land on top and right.
      bool left_open  = is_land_if_exists( e_direction::nw );
      bool down_open  = is_land_if_exists( e_direction::se );
      bool open_water = !is_land_if_exists( e_direction::sw );
      bool apply_sand =
          is_land_if_exists( e_direction::ne ) && open_water;
      switch( to_mask( left_open, down_open ) ) {
        case 0b00:
          // left closed, down closed.
          water_tile  = e_tile::terrain_ocean_down_left_c_c;
          beach_tile  = e_tile::terrain_beach_down_left_c_c;
          border_tile = e_tile::terrain_border_down_left_c_c;
          surf_tile   = e_tile::terrain_surf_down_left_c_c;
          sand_tile   = e_tile::terrain_sand_down_left_c_c;
          break;
        case 0b01:
          // left closed, down open.
          water_tile  = e_tile::terrain_ocean_down_left_c_o;
          beach_tile  = e_tile::terrain_beach_down_left_c_o;
          border_tile = e_tile::terrain_border_down_left_c_o;
          surf_tile   = e_tile::terrain_surf_down_left_c_o;
          sand_tile   = e_tile::terrain_sand_down_left_c_o;
          break;
        case 0b10:
          // left open, down closed.
          water_tile  = e_tile::terrain_ocean_down_left_o_c;
          beach_tile  = e_tile::terrain_beach_down_left_o_c;
          border_tile = e_tile::terrain_border_down_left_o_c;
          surf_tile   = e_tile::terrain_surf_down_left_o_c;
          sand_tile   = e_tile::terrain_sand_down_left_o_c;
          break;
        case 0b11:
          // left open, down open.
          water_tile  = e_tile::terrain_ocean_down_left_o_o;
          beach_tile  = e_tile::terrain_beach_down_left_o_o;
          border_tile = e_tile::terrain_border_down_left_o_o;
          surf_tile   = e_tile::terrain_surf_down_left_o_o;
          sand_tile   = e_tile::terrain_sand_down_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      if( !apply_sand ) sand_tile.reset();
      if( !open_water ) surf_tile.reset();
      break;
    }
    case 0b1001: {
      // land on right and bottom.
      bool left_open  = is_land_if_exists( e_direction::sw );
      bool top_open   = is_land_if_exists( e_direction::ne );
      bool open_water = !is_land_if_exists( e_direction::nw );
      bool apply_sand =
          is_land_if_exists( e_direction::se ) && open_water;
      switch( to_mask( left_open, top_open ) ) {
        case 0b00:
          // left closed, top closed.
          water_tile  = e_tile::terrain_ocean_left_up_c_c;
          beach_tile  = e_tile::terrain_beach_left_up_c_c;
          border_tile = e_tile::terrain_border_left_up_c_c;
          surf_tile   = e_tile::terrain_surf_left_up_c_c;
          sand_tile   = e_tile::terrain_sand_left_up_c_c;
          break;
        case 0b01:
          // left closed, top open.
          water_tile  = e_tile::terrain_ocean_left_up_c_o;
          beach_tile  = e_tile::terrain_beach_left_up_c_o;
          border_tile = e_tile::terrain_border_left_up_c_o;
          surf_tile   = e_tile::terrain_surf_left_up_c_o;
          sand_tile   = e_tile::terrain_sand_left_up_c_o;
          break;
        case 0b10:
          // left open, top closed.
          water_tile  = e_tile::terrain_ocean_left_up_o_c;
          beach_tile  = e_tile::terrain_beach_left_up_o_c;
          border_tile = e_tile::terrain_border_left_up_o_c;
          surf_tile   = e_tile::terrain_surf_left_up_o_c;
          sand_tile   = e_tile::terrain_sand_left_up_o_c;
          break;
        case 0b11:
          // left open, top open.
          water_tile  = e_tile::terrain_ocean_left_up_o_o;
          beach_tile  = e_tile::terrain_beach_left_up_o_o;
          border_tile = e_tile::terrain_border_left_up_o_o;
          surf_tile   = e_tile::terrain_surf_left_up_o_o;
          sand_tile   = e_tile::terrain_sand_left_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      if( !apply_sand ) sand_tile.reset();
      if( !open_water ) surf_tile.reset();
      break;
    }
    case 0b1100: {
      // land on bottom and left.
      bool up_open    = is_land_if_exists( e_direction::nw );
      bool right_open = is_land_if_exists( e_direction::se );
      bool open_water = !is_land_if_exists( e_direction::ne );
      bool apply_sand =
          is_land_if_exists( e_direction::sw ) && open_water;
      switch( to_mask( up_open, right_open ) ) {
        case 0b00:
          // up closed, right closed.
          water_tile  = e_tile::terrain_ocean_up_right_c_c;
          beach_tile  = e_tile::terrain_beach_up_right_c_c;
          border_tile = e_tile::terrain_border_up_right_c_c;
          surf_tile   = e_tile::terrain_surf_up_right_c_c;
          sand_tile   = e_tile::terrain_sand_up_right_c_c;
          break;
        case 0b01:
          // up closed, right open.
          water_tile  = e_tile::terrain_ocean_up_right_c_o;
          beach_tile  = e_tile::terrain_beach_up_right_c_o;
          border_tile = e_tile::terrain_border_up_right_c_o;
          surf_tile   = e_tile::terrain_surf_up_right_c_o;
          sand_tile   = e_tile::terrain_sand_up_right_c_o;
          break;
        case 0b10:
          // up open, right closed.
          water_tile  = e_tile::terrain_ocean_up_right_o_c;
          beach_tile  = e_tile::terrain_beach_up_right_o_c;
          border_tile = e_tile::terrain_border_up_right_o_c;
          surf_tile   = e_tile::terrain_surf_up_right_o_c;
          sand_tile   = e_tile::terrain_sand_up_right_o_c;
          break;
        case 0b11:
          // up open, right open.
          water_tile  = e_tile::terrain_ocean_up_right_o_o;
          beach_tile  = e_tile::terrain_beach_up_right_o_o;
          border_tile = e_tile::terrain_border_up_right_o_o;
          surf_tile   = e_tile::terrain_surf_up_right_o_o;
          sand_tile   = e_tile::terrain_sand_up_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      if( !apply_sand ) sand_tile.reset();
      if( !open_water ) surf_tile.reset();
      break;
    }
    case 0b1010: {
      // land on left and right.
      bool up_left_open  = is_land_if_exists( e_direction::nw );
      bool up_right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( up_left_open, up_right_open ) ) {
        case 0b00:
          // up left closed, up right closed.
          water_tile  = e_tile::terrain_ocean_up_down_up_c_c;
          beach_tile  = e_tile::terrain_beach_up_down_up_c_c;
          border_tile = e_tile::terrain_border_up_down_up_c_c;
          break;
        case 0b01:
          // up left closed, up right open.
          water_tile  = e_tile::terrain_ocean_up_down_up_c_o;
          beach_tile  = e_tile::terrain_beach_up_down_up_c_o;
          border_tile = e_tile::terrain_border_up_down_up_c_o;
          break;
        case 0b10:
          // up left open, up right closed.
          water_tile  = e_tile::terrain_ocean_up_down_up_o_c;
          beach_tile  = e_tile::terrain_beach_up_down_up_o_c;
          border_tile = e_tile::terrain_border_up_down_up_o_c;
          break;
        case 0b11:
          // up left open, up right open.
          water_tile  = e_tile::terrain_ocean_up_down_up_o_o;
          beach_tile  = e_tile::terrain_beach_up_down_up_o_o;
          border_tile = e_tile::terrain_border_up_down_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      bool down_left_open = is_land_if_exists( e_direction::sw );
      bool down_right_open =
          is_land_if_exists( e_direction::se );
      switch( to_mask( down_left_open, down_right_open ) ) {
        case 0b00:
          // down left closed, down right closed.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_c_c;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_c_c;
          second_border_tile =
              e_tile::terrain_border_up_down_down_c_c;
          break;
        case 0b01:
          // down left closed, down right open.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_c_o;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_c_o;
          second_border_tile =
              e_tile::terrain_border_up_down_down_c_o;
          break;
        case 0b10:
          // down left open, down right closed.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_o_c;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_o_c;
          second_border_tile =
              e_tile::terrain_border_up_down_down_o_c;
          break;
        case 0b11:
          // down left open, down right open.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_o_o;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_o_o;
          second_border_tile =
              e_tile::terrain_border_up_down_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0101: {
      // land on top and bottom.
      bool up_left_open   = is_land_if_exists( e_direction::nw );
      bool down_left_open = is_land_if_exists( e_direction::sw );
      switch( to_mask( up_left_open, down_left_open ) ) {
        case 0b00:
          // up left closed, down left closed.
          water_tile = e_tile::terrain_ocean_left_right_left_c_c;
          beach_tile = e_tile::terrain_beach_left_right_left_c_c;
          border_tile =
              e_tile::terrain_border_left_right_left_c_c;
          break;
        case 0b01:
          // up left closed, down left open.
          water_tile = e_tile::terrain_ocean_left_right_left_c_o;
          beach_tile = e_tile::terrain_beach_left_right_left_c_o;
          border_tile =
              e_tile::terrain_border_left_right_left_c_o;
          break;
        case 0b10:
          // up left open, down left closed.
          water_tile = e_tile::terrain_ocean_left_right_left_o_c;
          beach_tile = e_tile::terrain_beach_left_right_left_o_c;
          border_tile =
              e_tile::terrain_border_left_right_left_o_c;
          break;
        case 0b11:
          // up left open, down left open.
          water_tile = e_tile::terrain_ocean_left_right_left_o_o;
          beach_tile = e_tile::terrain_beach_left_right_left_o_o;
          border_tile =
              e_tile::terrain_border_left_right_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      bool up_right_open = is_land_if_exists( e_direction::ne );
      bool down_right_open =
          is_land_if_exists( e_direction::se );
      switch( to_mask( up_right_open, down_right_open ) ) {
        case 0b00:
          // up right closed, down right closed.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_c_c;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_c_c;
          second_border_tile =
              e_tile::terrain_border_left_right_right_c_c;
          break;
        case 0b01:
          // up right closed, down right open.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_c_o;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_c_o;
          second_border_tile =
              e_tile::terrain_border_left_right_right_c_o;
          break;
        case 0b10:
          // up right open, down right closed.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_o_c;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_o_c;
          second_border_tile =
              e_tile::terrain_border_left_right_right_o_c;
          break;
        case 0b11:
          // up right open, down right open.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_o_o;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_o_o;
          second_border_tile =
              e_tile::terrain_border_left_right_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1000: {
      // land on right, bottom, left.
      bool left_open  = is_land_if_exists( e_direction::nw );
      bool right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile  = e_tile::terrain_ocean_up_c_c;
          beach_tile  = e_tile::terrain_beach_up_c_c;
          border_tile = e_tile::terrain_border_up_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile  = e_tile::terrain_ocean_up_c_o;
          beach_tile  = e_tile::terrain_beach_up_c_o;
          border_tile = e_tile::terrain_border_up_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile  = e_tile::terrain_ocean_up_o_c;
          beach_tile  = e_tile::terrain_beach_up_o_c;
          border_tile = e_tile::terrain_border_up_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile  = e_tile::terrain_ocean_up_o_o;
          beach_tile  = e_tile::terrain_beach_up_o_o;
          border_tile = e_tile::terrain_border_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0100: {
      // land on bottom, left, top.
      bool top_open  = is_land_if_exists( e_direction::ne );
      bool down_open = is_land_if_exists( e_direction::se );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile  = e_tile::terrain_ocean_right_c_c;
          beach_tile  = e_tile::terrain_beach_right_c_c;
          border_tile = e_tile::terrain_border_right_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile  = e_tile::terrain_ocean_right_c_o;
          beach_tile  = e_tile::terrain_beach_right_c_o;
          border_tile = e_tile::terrain_border_right_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile  = e_tile::terrain_ocean_right_o_c;
          beach_tile  = e_tile::terrain_beach_right_o_c;
          border_tile = e_tile::terrain_border_right_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile  = e_tile::terrain_ocean_right_o_o;
          beach_tile  = e_tile::terrain_beach_right_o_o;
          border_tile = e_tile::terrain_border_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0010: {
      // land on left, top, right.
      bool left_open  = is_land_if_exists( e_direction::sw );
      bool right_open = is_land_if_exists( e_direction::se );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile  = e_tile::terrain_ocean_down_c_c;
          beach_tile  = e_tile::terrain_beach_down_c_c;
          border_tile = e_tile::terrain_border_down_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile  = e_tile::terrain_ocean_down_c_o;
          beach_tile  = e_tile::terrain_beach_down_c_o;
          border_tile = e_tile::terrain_border_down_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile  = e_tile::terrain_ocean_down_o_c;
          beach_tile  = e_tile::terrain_beach_down_o_c;
          border_tile = e_tile::terrain_border_down_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile  = e_tile::terrain_ocean_down_o_o;
          beach_tile  = e_tile::terrain_beach_down_o_o;
          border_tile = e_tile::terrain_border_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0001: {
      // land on top, right, bottom.
      bool top_open  = is_land_if_exists( e_direction::nw );
      bool down_open = is_land_if_exists( e_direction::sw );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile  = e_tile::terrain_ocean_left_c_c;
          beach_tile  = e_tile::terrain_beach_left_c_c;
          border_tile = e_tile::terrain_border_left_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile  = e_tile::terrain_ocean_left_c_o;
          beach_tile  = e_tile::terrain_beach_left_c_o;
          border_tile = e_tile::terrain_border_left_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile  = e_tile::terrain_ocean_left_o_c;
          beach_tile  = e_tile::terrain_beach_left_o_c;
          border_tile = e_tile::terrain_border_left_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile  = e_tile::terrain_ocean_left_o_o;
          beach_tile  = e_tile::terrain_beach_left_o_o;
          border_tile = e_tile::terrain_border_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0000:
      // land on all sides.
      water_tile  = e_tile::terrain_ocean_island;
      beach_tile  = e_tile::terrain_beach_island;
      border_tile = e_tile::terrain_border_island;
      break;
    default: {
      FATAL( "invalid ocean mask: {}", mask );
    }
  }

  // We have at least one bordering land square, so we need to
  // render a ground tile first because there will be a bit of
  // land visible on this tile.
  UNWRAP_CHECK( ground,
                ground_terrain_for_square( terrain_state, square,
                                           world_square ) );
  render_terrain_ground( terrain_state, painter, renderer, where,
                         world_square, ground );

  e_tile ocean_background = square.sea_lane
                                ? e_tile::terrain_ocean_sea_lane
                                : e_tile::terrain_ocean;
  render_sprite_stencil( painter, where, water_tile,
                         ocean_background, gfx::pixel::black() );
  if( second_water_tile.has_value() )
    render_sprite_stencil( painter, where, *second_water_tile,
                           ocean_background,
                           gfx::pixel::black() );
  render_sprite( painter, where, beach_tile );
  render_sprite( painter, where, border_tile );
  if( second_beach_tile.has_value() )
    render_sprite( painter, where, *second_beach_tile );
  if( second_border_tile.has_value() )
    render_sprite( painter, where, *second_border_tile );
  if( surf_tile.has_value() ) {
    SCOPED_RENDERER_MOD_SET( painter_mods.cycling.enabled,
                             true );
    rr::Painter painter = renderer.painter();
    render_sprite( painter, where, *surf_tile );
  }

  // It's ok to draw canals after this because this won't be on a
  // tile with canals.
  if( sand_tile.has_value() ) {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .9 );
    rr::Painter painter = renderer.painter();
    render_sprite( painter, where, *sand_tile );
  }

  // Render canals.
  if( up_left.surface == e_surface::water &&
      left.surface == e_surface::land &&
      up.surface == e_surface::land ) {
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_up_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( painter, where,
                   e_tile::terrain_border_canal_up_left );
  }
  if( up_right.surface == e_surface::water &&
      up.surface == e_surface::land &&
      right.surface == e_surface::land ) {
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_up_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( painter, where,
                   e_tile::terrain_border_canal_up_right );
  }
  if( down_right.surface == e_surface::water &&
      down.surface == e_surface::land &&
      right.surface == e_surface::land ) {
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_down_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( painter, where,
                   e_tile::terrain_border_canal_down_right );
  }
  if( down_left.surface == e_surface::water &&
      down.surface == e_surface::land &&
      left.surface == e_surface::land ) {
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_down_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( painter, where,
                   e_tile::terrain_border_canal_down_left );
  }

  render_beach_corners( painter, where, up, right, down, left,
                        up_left, up_right, down_right,
                        down_left );
}

void render_lost_city_rumor( rr::Painter& painter, Coord where,
                             MapSquare const& square ) {
  if( square.lost_city_rumor )
    render_sprite( painter, where, e_tile::lost_city_rumor );
}

void render_fish( TerrainState const& terrain_state,
                  rr::Renderer& renderer, Coord where,
                  Coord world_square ) {
  MapSquare const& up =
      terrain_state.total_square_at( world_square - 1_h );
  MapSquare const& left =
      terrain_state.total_square_at( world_square - 1_w );
  MapSquare const& down =
      terrain_state.total_square_at( world_square + 1_h );

  // Treat off-map tiles as water for rendering purposes.
  bool const land_up   = up.surface == e_surface::land;
  bool const land_left = left.surface == e_surface::land;
  bool const land_down = down.surface == e_surface::land;

  // If there is land above or to the right of the fish then we
  // will render an outline facing that direction it so that it
  // can be visually distinguished.
  bool const should_outline_up   = land_up;
  bool const should_outline_left = land_left;

  // If there is land below then we will render a small splash so
  // that the splash doesn't overlap the land.
  bool const small_splash = land_down;

  e_tile fish_body = e_tile::resource_fish_body;
  e_tile fish_splash_fin =
      small_splash ? e_tile::resource_fish_splash_fin_small
                   : e_tile::resource_fish_splash_fin_large;

  gfx::pixel const outline_color = {
      .r = 32, .g = 85, .b = 78, .a = 255 };

  if( should_outline_up || should_outline_left ) {
    // TODO: insert mod here.
    rr::Painter painter = renderer.painter();
    if( should_outline_up )
      render_sprite_silhouette( painter, where - 1_h, fish_body,
                                outline_color );
    if( should_outline_left )
      render_sprite_silhouette( painter, where - 1_w, fish_body,
                                outline_color );
  }

  rr::Painter painter = renderer.painter();
  render_sprite( painter, where, fish_body );
  render_sprite( painter, where, fish_splash_fin );
}

e_tile resource_tile( e_natural_resource resource ) {
  switch( resource ) {
    case e_natural_resource::beaver: //
      return e_tile::resource_beaver;
    case e_natural_resource::deer: //
      return e_tile::resource_deer;
    case e_natural_resource::tree: //
      return e_tile::resource_tree;
    case e_natural_resource::minerals: //
      return e_tile::resource_minerals;
    case e_natural_resource::oasis: //
      return e_tile::resource_oasis;
    case e_natural_resource::cotton: //
      return e_tile::resource_cotton;
    case e_natural_resource::fish: //
      SHOULD_NOT_BE_HERE;
    case e_natural_resource::ore: //
      return e_tile::resource_ore;
    case e_natural_resource::silver: //
      return e_tile::resource_silver;
    case e_natural_resource::silver_depleted: //
      return e_tile::resource_silver_depleted;
    case e_natural_resource::sugar: //
      return e_tile::resource_sugar;
    case e_natural_resource::tobacco: //
      return e_tile::resource_tobacco;
    case e_natural_resource::wheat: //
      return e_tile::resource_wheat;
  }
}

void render_resources( rr::Renderer&       renderer,
                       rr::Painter&        painter,
                       TerrainState const& terrain_state,
                       Coord where, MapSquare const& square,
                       Coord world_square ) {
  maybe<e_natural_resource> resource =
      effective_resource( square );
  if( !resource.has_value() ) return;
  if( *resource == e_natural_resource::fish )
    return render_fish( terrain_state, renderer, where,
                        world_square );
  render_sprite( painter, where, resource_tile( *resource ) );
}

// Pass in the painter as well for efficiency.
void render_terrain_square( TerrainState const& terrain_state,
                            rr::Renderer& renderer, Coord where,
                            Coord const world_square ) {
  rr::Painter      painter = renderer.painter();
  MapSquare const& square =
      terrain_state.square_at( world_square );
  if( square.surface == e_surface::water )
    render_terrain_ocean_square( renderer, painter, where,
                                 terrain_state, square,
                                 world_square );
  else
    render_terrain_land_square( terrain_state, painter, renderer,
                                where, world_square, square );
  if( square.river.has_value() )
    render_river( terrain_state, renderer, where, world_square,
                  square );
  if( !square.lost_city_rumor )
    render_resources( renderer, painter, terrain_state, where,
                      square, world_square );
  render_plow_if_present( painter, where, terrain_state,
                          world_square );
  render_road_if_present( painter, where, terrain_state,
                          world_square );
  render_lost_city_rumor( painter, where, square );
  if( g_show_grid )
    painter.draw_empty_rect( Rect::from( where, g_tile_delta ),
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

void render_terrain( TerrainState const& terrain_state,
                     rr::Renderer&       renderer ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );
  auto const kLandscapeBuf =
      rr::e_render_target_buffer::landscape;
  renderer.clear_buffer( kLandscapeBuf );
  SCOPED_RENDERER_MOD_SET( buffer_mods.buffer, kLandscapeBuf );
  auto start_time = chrono::system_clock::now();
  for( Coord square : terrain_state.world_rect_tiles() )
    render_terrain_square( terrain_state, renderer,
                           square * g_tile_scale, square );
  auto end_time = chrono::system_clock::now();
  lg.info(
      "rendered landscape: {}ms with {} vertices, occupying "
      "{:.2f}MB.",
      chrono::duration_cast<chrono::milliseconds>( end_time -
                                                   start_time )
          .count(),
      renderer.buffer_vertex_count( kLandscapeBuf ),
      renderer.buffer_size_mb( kLandscapeBuf ) );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( toggle_grid, void ) {
  g_show_grid = !g_show_grid;
  lg.debug( "terrain grid is {}.", g_show_grid ? "on" : "off" );
  LUA_CHECK( st, is_renderer_loaded(),
             "the renderer has not yet been initialized." );
  MapUpdater map_updater(
      GameState::terrain(),
      global_renderer_use_only_when_needed() );
  map_updater.just_redraw_map();
}

LUA_FN( redraw, void ) {
  LUA_CHECK( st, is_renderer_loaded(),
             "the renderer has not yet been initialized." );
  MapUpdater map_updater(
      GameState::terrain(),
      global_renderer_use_only_when_needed() );
  map_updater.just_redraw_map();
}

} // namespace

} // namespace rn
