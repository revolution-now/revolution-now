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
#include "imap-updater.hpp"
#include "logger.hpp"
#include "map-square.hpp"
#include "plow.hpp"
#include "road.hpp"
#include "tiles.hpp"
#include "visibility.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/terrain.hpp"

// render
#include "render/painter.hpp"
#include "render/renderer.hpp"

// gfx
#include "gfx/iter.hpp"
#include "gfx/matrix.hpp"

// refl
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

constexpr double g_tile_overlap_width_percent = .2;

e_tile tile_for_ground_terrain( e_ground_terrain terrain ) {
  switch( terrain ) {
    case e_ground_terrain::arctic:
      return e_tile::terrain_arctic;
    case e_ground_terrain::desert:
      return e_tile::terrain_desert;
    case e_ground_terrain::grassland:
      return e_tile::terrain_grassland;
    case e_ground_terrain::marsh:
      return e_tile::terrain_marsh;
    case e_ground_terrain::plains:
      return e_tile::terrain_plains;
    case e_ground_terrain::prairie:
      return e_tile::terrain_prairie;
    case e_ground_terrain::savannah:
      return e_tile::terrain_savannah;
    case e_ground_terrain::swamp:
      return e_tile::terrain_swamp;
    case e_ground_terrain::tundra:
      return e_tile::terrain_tundra;
  }
}

maybe<e_ground_terrain> ground_terrain_for_square(
    IVisibility const& viz, MapSquare const& square,
    Coord world_square ) {
  if( square.surface == e_surface::land ) return square.ground;
  // We have a water so get it from the surroundings.
  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );
  if( left.surface == e_surface::land ) return left.ground;

  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  if( up.surface == e_surface::land ) return up.ground;

  MapSquare const& right =
      viz.square_at( world_square + Delta{ .w = 1 } );
  if( right.surface == e_surface::land ) return right.ground;

  MapSquare const& down =
      viz.square_at( world_square + Delta{ .h = 1 } );
  if( down.surface == e_surface::land ) return down.ground;

  MapSquare const& up_left = viz.square_at(
      world_square - Delta{ .w = 1 } - Delta{ .h = 1 } );
  if( up_left.surface == e_surface::land ) return up_left.ground;

  MapSquare const& up_right = viz.square_at(
      world_square - Delta{ .h = 1 } + Delta{ .w = 1 } );
  if( up_right.surface == e_surface::land )
    return up_right.ground;

  MapSquare const& down_right = viz.square_at(
      world_square + Delta{ .w = 1 } + Delta{ .h = 1 } );
  if( down_right.surface == e_surface::land )
    return down_right.ground;

  MapSquare const& down_left = viz.square_at(
      world_square + Delta{ .h = 1 } - Delta{ .w = 1 } );
  if( down_left.surface == e_surface::land )
    return down_left.ground;

  return nothing;
}

void render_mountains( IVisibility const& viz,
                       rr::Renderer& renderer, Coord where,
                       Coord world_square ) {
  MapSquare const& here = viz.square_at( world_square );
  CHECK( here.surface == e_surface::land );

  // Returns true if the tile is land and it has mountains.
  auto is_mountains = [&]( e_direction d ) {
    MapSquare const& s =
        viz.square_at( world_square.moved( d ) );
    return s.surface == e_surface::land &&
           s.overlay == e_land_overlay::mountains;
  };

  bool has_left  = is_mountains( e_direction::w );
  bool has_up    = is_mountains( e_direction::n );
  bool has_right = is_mountains( e_direction::e );
  bool has_down  = is_mountains( e_direction::s );

  // 0000abcd:
  // a=mountains up, b=mountains right, c=mountains down,
  // d=mountains left.
  int mask = ( has_up ? ( 1 << 3 ) : 0 ) |
             ( has_right ? ( 1 << 2 ) : 0 ) |
             ( has_down ? ( 1 << 1 ) : 0 ) |
             ( has_left ? ( 1 << 0 ) : 0 );

  e_tile mountains_tile = {};

  switch( mask ) {
    case 0b0001: {
      // mountains on left.
      mountains_tile = e_tile::terrain_mountains_left;
      break;
    }
    case 0b1000: {
      // mountains on top.
      mountains_tile = e_tile::terrain_mountains_up;
      break;
    }
    case 0b0100: {
      // mountains on right.
      mountains_tile = e_tile::terrain_mountains_right;
      break;
    }
    case 0b0010: {
      // mountains on bottom.
      mountains_tile = e_tile::terrain_mountains_down;
      break;
    }
    case 0b1001: {
      // mountains on left and top.
      mountains_tile = e_tile::terrain_mountains_left_up;
      break;
    }
    case 0b1100: {
      // mountains on top and right.
      mountains_tile = e_tile::terrain_mountains_up_right;
      break;
    }
    case 0b0110: {
      // mountains on right and bottom.
      mountains_tile = e_tile::terrain_mountains_right_down;
      break;
    }
    case 0b0011: {
      // mountains on bottom and left.
      mountains_tile = e_tile::terrain_mountains_down_left;
      break;
    }
    case 0b0101: {
      // mountains on left and right.
      mountains_tile = e_tile::terrain_mountains_left_right;
      break;
    }
    case 0b1010: {
      // mountains on top and bottom.
      mountains_tile = e_tile::terrain_mountains_up_down;
      break;
    }
    case 0b0111: {
      // mountains on right, bottom, left.
      mountains_tile = e_tile::terrain_mountains_right_down_left;
      break;
    }
    case 0b1011: {
      // mountains on bottom, left, top.
      mountains_tile = e_tile::terrain_mountains_down_left_up;
      break;
    }
    case 0b1101: {
      // mountains on left, top, right.
      mountains_tile = e_tile::terrain_mountains_left_up_right;
      break;
    }
    case 0b1110: {
      // mountains on top, right, bottom.
      mountains_tile = e_tile::terrain_mountains_up_right_down;
      break;
    }
    case 0b1111:
      // mountains on all sides.
      mountains_tile = e_tile::terrain_mountains_all;
      break;
    case 0b0000:
      // mountains on no sides.
      mountains_tile = e_tile::terrain_mountains_island;
      break;
    default: {
      FATAL( "invalid mountains mask: {}", mask );
    }
  }
  render_sprite( renderer, where, mountains_tile );
}

void render_hills( IVisibility const& viz,
                   rr::Renderer& renderer, Coord where,
                   Coord world_square ) {
  MapSquare const& here = viz.square_at( world_square );
  CHECK( here.surface == e_surface::land );

  // Returns true if the tile is land and it has hills.
  auto is_hills = [&]( e_direction d ) {
    MapSquare const& s =
        viz.square_at( world_square.moved( d ) );
    return s.surface == e_surface::land &&
           s.overlay == e_land_overlay::hills;
  };

  bool has_left  = is_hills( e_direction::w );
  bool has_up    = is_hills( e_direction::n );
  bool has_right = is_hills( e_direction::e );
  bool has_down  = is_hills( e_direction::s );

  // 0000abcd:
  // a=hills up, b=hills right, c=hills down, d=hills left.
  int mask = ( has_up ? ( 1 << 3 ) : 0 ) |
             ( has_right ? ( 1 << 2 ) : 0 ) |
             ( has_down ? ( 1 << 1 ) : 0 ) |
             ( has_left ? ( 1 << 0 ) : 0 );

  e_tile hills_tile = {};

  switch( mask ) {
    case 0b0001: {
      // hills on left.
      hills_tile = e_tile::terrain_hills_left;
      break;
    }
    case 0b1000: {
      // hills on top.
      hills_tile = e_tile::terrain_hills_up;
      break;
    }
    case 0b0100: {
      // hills on right.
      hills_tile = e_tile::terrain_hills_right;
      break;
    }
    case 0b0010: {
      // hills on bottom.
      hills_tile = e_tile::terrain_hills_down;
      break;
    }
    case 0b1001: {
      // hills on left and top.
      hills_tile = e_tile::terrain_hills_left_up;
      break;
    }
    case 0b1100: {
      // hills on top and right.
      hills_tile = e_tile::terrain_hills_up_right;
      break;
    }
    case 0b0110: {
      // hills on right and bottom.
      hills_tile = e_tile::terrain_hills_right_down;
      break;
    }
    case 0b0011: {
      // hills on bottom and left.
      hills_tile = e_tile::terrain_hills_down_left;
      break;
    }
    case 0b0101: {
      // hills on left and right.
      hills_tile = e_tile::terrain_hills_left_right;
      break;
    }
    case 0b1010: {
      // hills on top and bottom.
      hills_tile = e_tile::terrain_hills_up_down;
      break;
    }
    case 0b0111: {
      // hills on right, bottom, left.
      hills_tile = e_tile::terrain_hills_right_down_left;
      break;
    }
    case 0b1011: {
      // hills on bottom, left, top.
      hills_tile = e_tile::terrain_hills_down_left_up;
      break;
    }
    case 0b1101: {
      // hills on left, top, right.
      hills_tile = e_tile::terrain_hills_left_up_right;
      break;
    }
    case 0b1110: {
      // hills on top, right, bottom.
      hills_tile = e_tile::terrain_hills_up_right_down;
      break;
    }
    case 0b1111:
      // hills on all sides.
      hills_tile = e_tile::terrain_hills_all;
      break;
    case 0b0000:
      // hills on no sides.
      hills_tile = e_tile::terrain_hills_island;
      break;
    default: {
      FATAL( "invalid hills mask: {}", mask );
    }
  }
  render_sprite( renderer, where, hills_tile );
}

void render_forest( IVisibility const& viz,
                    rr::Renderer& renderer, Coord where,
                    Coord world_square ) {
  MapSquare const& here = viz.square_at( world_square );
  CHECK( here.surface == e_surface::land );
  if( here.ground == e_ground_terrain::desert ) {
    render_sprite( renderer, where,
                   e_tile::terrain_forest_scrub_island );
    return;
  }

  // Returns true if the the tile exists, it is land, it is
  // non-desert, and it has a forest.
  auto is_forest = [&]( e_direction d ) {
    MapSquare const& s =
        viz.square_at( world_square.moved( d ) );
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
  render_sprite( renderer, where, forest_tile );
}

void render_adjacent_overlap( IVisibility const& viz,
                              rr::Renderer&      renderer,
                              Coord where, Coord world_square,
                              double chop_percent ) {
  MapSquare const& west =
      viz.square_at( world_square - Delta{ .w = 1 } );
  MapSquare const& north =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& east =
      viz.square_at( world_square + Delta{ .w = 1 } );
  MapSquare const& south =
      viz.square_at( world_square + Delta{ .h = 1 } );

  int chop_pixels = std::lround( g_tile_delta.w * chop_percent );
  W   chop_w      = W{ chop_pixels };
  H   chop_h      = H{ chop_pixels };

  // In the below, when two adjacent segments touch, we want
  // their depixelations to be .5, otherwise there will be a hard
  // edge between the tiles which will kill the smooth transi-
  // tion.
  int const overgrowth_pixels = ( g_tile_delta.w - chop_pixels );
  float const depixel_stage_slope = 0.5 / overgrowth_pixels;

  // Note in the below we set the top/bottom segments to be in-
  // verted so that the overlapping segment does not depixelate
  // the same set of pixels; if it did the only the segment that
  // was drawn first would be visible.

  {
    // Render bottom part of north tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.h -= chop_h;
    src.y += chop_h;
    dst.y += 0;
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             0.5 );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.inverted,
                             true );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient,
        gfx::dsize{ .h = -depixel_stage_slope } );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        dst.to_gfx().to_double() );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
        viz, north, world_square - Delta{ .h = 1 } );
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
    src.y += 0;
    dst.y += chop_h;
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             0.0 );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.inverted,
                             true );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient,
        gfx::dsize{ .h = depixel_stage_slope } );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        dst.to_gfx().to_double() );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
        viz, south, world_square + Delta{ .h = 1 } );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }

  {
    // Render east part of western tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.w -= chop_w;
    src.x += chop_w;
    dst.x += 0;
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             0.5 );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient,
        gfx::dsize{ .w = depixel_stage_slope } );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        dst.to_gfx().to_double() );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
        viz, west, world_square - Delta{ .w = 1 } );
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
    src.x += 0;
    dst.x += chop_w;
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             1.0 );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient,
        gfx::dsize{ .w = -depixel_stage_slope } );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        dst.to_gfx().to_double() );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
        viz, east, world_square + Delta{ .w = 1 } );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }
}

void render_terrain_ground( IVisibility const& viz,
                            rr::Renderer& renderer, Coord where,
                            Coord            world_square,
                            e_ground_terrain ground ) {
  e_tile tile = tile_for_ground_terrain( ground );
  render_sprite( renderer, where, tile );
  render_adjacent_overlap(
      viz, renderer, where, world_square,
      /*chop_percent=*/
      clamp( 1.0 - g_tile_overlap_width_percent, 0.0, 1.0 ) );

  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );
  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& right =
      viz.square_at( world_square + Delta{ .w = 1 } );
  MapSquare const& up_left = viz.square_at(
      world_square - Delta{ .h = 1 } - Delta{ .w = 1 } );
  MapSquare const& up_right = viz.square_at(
      world_square + Delta{ .w = 1 } - Delta{ .h = 1 } );

  // This should be done at the end.
  if( up_right.surface == e_surface::land &&
      up.surface == e_surface::water &&
      right.surface == e_surface::water ) {
    render_sprite_stencil(
        renderer, where,
        e_tile::terrain_ocean_canal_corner_up_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite(
        renderer, where,
        e_tile::terrain_border_canal_corner_up_right );
  }
  if( up_left.surface == e_surface::land &&
      up.surface == e_surface::water &&
      left.surface == e_surface::water ) {
    render_sprite_stencil(
        renderer, where,
        e_tile::terrain_ocean_canal_corner_up_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( renderer, where,
                   e_tile::terrain_border_canal_corner_up_left );
  }
}

// Pass in the painter as well for efficiency.
void render_terrain_land_square( IVisibility const& viz,
                                 rr::Renderer&      renderer,
                                 Coord where, Coord world_square,
                                 MapSquare const& square ) {
  DCHECK( square.surface == e_surface::land );
  render_terrain_ground( viz, renderer, where, world_square,
                         square.ground );
}

void render_beach_corners(
    rr::Renderer& renderer, Coord where, MapSquare const& up,
    MapSquare const& right, MapSquare const& down,
    MapSquare const& left, MapSquare const& up_left,
    MapSquare const& up_right, MapSquare const& down_right,
    MapSquare const& down_left ) {
  // Render beach corners.
  if( up_left.surface == e_surface::land &&
      left.surface == e_surface::water &&
      up.surface == e_surface::water )
    render_sprite( renderer, where,
                   e_tile::terrain_beach_corner_up_left );
  if( up_right.surface == e_surface::land &&
      up.surface == e_surface::water &&
      right.surface == e_surface::water )
    render_sprite( renderer, where,
                   e_tile::terrain_beach_corner_up_right );
  if( down_right.surface == e_surface::land &&
      down.surface == e_surface::water &&
      right.surface == e_surface::water )
    render_sprite( renderer, where,
                   e_tile::terrain_beach_corner_down_right );
  if( down_left.surface == e_surface::land &&
      down.surface == e_surface::water &&
      left.surface == e_surface::water )
    render_sprite( renderer, where,
                   e_tile::terrain_beach_corner_down_left );
}

void render_river_water_tile( rr::Renderer& renderer,
                              Coord where, e_tile tile,
                              MapSquare const& square ) {
  double alpha =
      ( square.surface == e_surface::water ) ? .05 : .1;
  render_sprite_stencil( renderer, where, tile,
                         e_tile::terrain_ocean,
                         gfx::pixel::black() );
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
    render_sprite_stencil( renderer, where, tile,
                           e_tile::terrain_river_shading,
                           gfx::pixel::black() );
  }
}

// The idea here is that when we render rivers on the ocean
// (which will always be end points, or at least rendered that
// way), there are a couple of differences with rendering it on
// land:
//
//   1. We don't draw the bank.
//   2. We don't join to ocean squares that both contain rivers.
//   3. We draw special fan-out tiles, one for each surrounding
//      square that has a river.
//
void render_river_on_ocean( IVisibility const& viz,
                            rr::Renderer& renderer, Coord where,
                            Coord            world_square,
                            MapSquare const& square ) {
  CHECK( square.river.has_value() );
  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& right =
      viz.square_at( world_square + Delta{ .w = 1 } );
  MapSquare const& down =
      viz.square_at( world_square + Delta{ .h = 1 } );
  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );

  bool river_up =
      up.river.has_value() && up.surface == e_surface::land;
  bool river_right = right.river.has_value() &&
                     right.surface == e_surface::land;
  bool river_down =
      down.river.has_value() && down.surface == e_surface::land;
  bool river_left =
      left.river.has_value() && left.surface == e_surface::land;

  if( river_left ) {
    render_river_water_tile(
        renderer, where, e_tile::terrain_river_fanout_land_left,
        square );
    render_sprite( renderer, where,
                   e_tile::terrain_river_fanout_bank_land_left );
  }
  if( river_up ) {
    render_river_water_tile(
        renderer, where, e_tile::terrain_river_fanout_land_up,
        square );
    render_sprite( renderer, where,
                   e_tile::terrain_river_fanout_bank_land_up );
  }
  if( river_right ) {
    render_river_water_tile(
        renderer, where, e_tile::terrain_river_fanout_land_right,
        square );
    render_sprite(
        renderer, where,
        e_tile::terrain_river_fanout_bank_land_right );
  }
  if( river_down ) {
    render_river_water_tile(
        renderer, where, e_tile::terrain_river_fanout_land_down,
        square );
    render_sprite( renderer, where,
                   e_tile::terrain_river_fanout_bank_land_down );
  }
}

void render_river_on_land( IVisibility const& viz,
                           rr::Renderer& renderer, Coord where,
                           Coord            world_square,
                           MapSquare const& square,
                           bool             no_bank ) {
  DCHECK( square.river.has_value() );
  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& right =
      viz.square_at( world_square + Delta{ .w = 1 } );
  MapSquare const& down =
      viz.square_at( world_square + Delta{ .h = 1 } );
  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );

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
    default:
      FATAL( "invalid river mask: {}", mask );
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

  render_river_water_tile( renderer, where, water, square );
  {
    SCOPED_RENDERER_MOD_SET( painter_mods.cycling.plan,
                             rr::e_color_cycle_plan::river );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
    render_river_water_tile( renderer, where, cycle, square );
  }
  if( !no_bank ) render_sprite( renderer, where, bank );
}

bool has_surrounding_nonforest_river_squares(
    IVisibility const& viz, Coord world_square ) {
  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& right =
      viz.square_at( world_square + Delta{ .w = 1 } );
  MapSquare const& down =
      viz.square_at( world_square + Delta{ .h = 1 } );
  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );

  int res = 0;

  if( ( up.overlay != e_land_overlay::forest ||
        up.ground == e_ground_terrain::desert ) &&
      up.river.has_value() )
    ++res;
  if( ( right.overlay != e_land_overlay::forest ||
        right.ground == e_ground_terrain::desert ) &&
      right.river.has_value() )
    ++res;
  if( ( down.overlay != e_land_overlay::forest ||
        down.ground == e_ground_terrain::desert ) &&
      down.river.has_value() )
    ++res;
  if( ( left.overlay != e_land_overlay::forest ||
        left.ground == e_ground_terrain::desert ) &&
      left.river.has_value() )
    ++res;

  return res > 0;
}

void render_river_hinting( IVisibility const& viz,
                           rr::Renderer& renderer, Coord where,
                           Coord            world_square,
                           MapSquare const& square ) {
  DCHECK( square.overlay == e_land_overlay::forest );
  static double constexpr kEdgeAlpha         = 0.5;
  static double constexpr kInnerAlpha        = 0.7;
  static double constexpr kEdgeDepixelStage  = 0.7;
  static double constexpr kInnerDepixelStage = 0.6;
  static_assert( kInnerAlpha >= kEdgeAlpha );
  bool is_edge = has_surrounding_nonforest_river_squares(
      viz, world_square );
  double const alpha = is_edge ? kEdgeAlpha : kInnerAlpha;
  double const stage =
      is_edge ? kEdgeDepixelStage : kInnerDepixelStage;
  SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  render_river_on_land( viz, renderer, where, world_square,
                        square, /*no_bank=*/true );
}

void render_land_overlay( IVisibility const& viz,
                          rr::Renderer& renderer, Coord where,
                          Coord            world_square,
                          MapSquare const& square ) {
  if( !square.overlay.has_value() ) return;
  switch( *square.overlay ) {
    case e_land_overlay::forest: {
      render_forest( viz, renderer, where, world_square );
      if( square.river.has_value() ) {
        if( square.ground != e_ground_terrain::desert )
          // This forest square, which contains a river, has al-
          // ready had its river rendered under the forest tile
          // (which it looks best), but that means that it won't
          // be visible to the player. That's not ideal because
          // the river will give production bonuses. So let's
          // draw a light (faded and depixelated) river overtop
          // of the forest to hint about it to the player.
          //
          // For the best visual effect, we will only render this
          // hint on forest tiles that are completely surrounded
          // (in the cardinal directions) by other forest tiles.
          render_river_hinting( viz, renderer, where,
                                world_square, square );
        else
          // If it's a forest in a desert (scrub forest) then
          // just render the river over top of it seems to make
          // more sense visually.
          render_river_on_land( viz, renderer, where,
                                world_square, square,
                                /*no_bank=*/false );
      }
      break;
    }
    case e_land_overlay::hills:
      render_hills( viz, renderer, where, world_square );
      break;
    case e_land_overlay::mountains:
      render_mountains( viz, renderer, where, world_square );
      break;
  }
}

void render_terrain_ocean_square( rr::Renderer&      renderer,
                                  Coord              where,
                                  IVisibility const& viz,
                                  MapSquare const&   square,
                                  Coord world_square ) {
  DCHECK( square.surface == e_surface::water );

  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& right =
      viz.square_at( world_square + Delta{ .w = 1 } );
  MapSquare const& down =
      viz.square_at( world_square + Delta{ .h = 1 } );
  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );
  MapSquare const& up_left = viz.square_at(
      world_square - Delta{ .h = 1 } - Delta{ .w = 1 } );
  MapSquare const& up_right = viz.square_at(
      world_square + Delta{ .w = 1 } - Delta{ .h = 1 } );
  MapSquare const& down_right = viz.square_at(
      world_square + Delta{ .h = 1 } + Delta{ .w = 1 } );
  MapSquare const& down_left = viz.square_at(
      world_square - Delta{ .w = 1 } + Delta{ .h = 1 } );

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

  auto render_maybe_with_sea_lane = [&]( auto&& f ) {
    f( e_tile::terrain_ocean );
    if( square.sea_lane ) {
      SCOPED_RENDERER_MOD_SET(
          painter_mods.cycling.plan,
          rr::e_color_cycle_plan::sea_lane );
      f( e_tile::terrain_ocean_sea_lane );
    }
  };

  if( mask == 0b1111 ) {
    // All surrounding water.
    render_maybe_with_sea_lane( [&]( e_tile const tile ) {
      render_sprite( renderer, where, tile );
    } );
    render_beach_corners( renderer, where, up, right, down, left,
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
        viz.square_at( world_square.moved( d ) );
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
        default:
          SHOULD_NOT_BE_HERE;
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
  UNWRAP_CHECK( ground, ground_terrain_for_square(
                            viz, square, world_square ) );
  render_terrain_ground( viz, renderer, where, world_square,
                         ground );

  render_maybe_with_sea_lane( [&]( e_tile const tile ) {
    render_sprite_stencil( renderer, where, water_tile, tile,
                           gfx::pixel::black() );
  } );
  if( second_water_tile.has_value() ) {
    render_maybe_with_sea_lane( [&]( e_tile const tile ) {
      render_sprite_stencil( renderer, where, *second_water_tile,
                             tile, gfx::pixel::black() );
    } );
  }
  render_sprite( renderer, where, beach_tile );
  render_sprite( renderer, where, border_tile );
  if( second_beach_tile.has_value() )
    render_sprite( renderer, where, *second_beach_tile );
  if( second_border_tile.has_value() )
    render_sprite( renderer, where, *second_border_tile );
  if( surf_tile.has_value() && !square.river.has_value() ) {
    SCOPED_RENDERER_MOD_SET( painter_mods.cycling.plan,
                             rr::e_color_cycle_plan::surf );
    render_sprite( renderer, where, *surf_tile );
  }

  // It's ok to draw canals after this because this won't be on a
  // tile with canals.
  if( sand_tile.has_value() ) {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .9 );
    render_sprite( renderer, where, *sand_tile );
  }

  // Render canals.
  if( up_left.surface == e_surface::water &&
      left.surface == e_surface::land &&
      up.surface == e_surface::land ) {
    render_sprite_stencil(
        renderer, where, e_tile::terrain_ocean_canal_up_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( renderer, where,
                   e_tile::terrain_border_canal_up_left );
  }
  if( up_right.surface == e_surface::water &&
      up.surface == e_surface::land &&
      right.surface == e_surface::land ) {
    render_sprite_stencil(
        renderer, where, e_tile::terrain_ocean_canal_up_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( renderer, where,
                   e_tile::terrain_border_canal_up_right );
  }
  if( down_right.surface == e_surface::water &&
      down.surface == e_surface::land &&
      right.surface == e_surface::land ) {
    render_sprite_stencil(
        renderer, where, e_tile::terrain_ocean_canal_down_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( renderer, where,
                   e_tile::terrain_border_canal_down_right );
  }
  if( down_left.surface == e_surface::water &&
      down.surface == e_surface::land &&
      left.surface == e_surface::land ) {
    render_sprite_stencil(
        renderer, where, e_tile::terrain_ocean_canal_down_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
    render_sprite( renderer, where,
                   e_tile::terrain_border_canal_down_left );
  }

  render_beach_corners( renderer, where, up, right, down, left,
                        up_left, up_right, down_right,
                        down_left );
}

void render_lost_city_rumor( rr::Renderer& renderer, Coord where,
                             MapSquare const& square ) {
  if( square.lost_city_rumor )
    render_sprite( renderer, where, e_tile::lost_city_rumor );
}

void render_fish( IVisibility const& viz, rr::Renderer& renderer,
                  Coord where, Coord world_square ) {
  MapSquare const& up =
      viz.square_at( world_square - Delta{ .h = 1 } );
  MapSquare const& left =
      viz.square_at( world_square - Delta{ .w = 1 } );
  MapSquare const& down =
      viz.square_at( world_square + Delta{ .h = 1 } );

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
    if( should_outline_up )
      render_sprite_silhouette( renderer,
                                where - Delta{ .h = 1 },
                                fish_body, outline_color );
    if( should_outline_left )
      render_sprite_silhouette( renderer,
                                where - Delta{ .w = 1 },
                                fish_body, outline_color );
  }

  render_sprite( renderer, where, fish_body );
  render_sprite( renderer, where, fish_splash_fin );
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

void render_resources( rr::Renderer&      renderer,
                       IVisibility const& viz, Coord where,
                       Coord world_square ) {
  maybe<e_natural_resource> const resource =
      viz.resource_at( world_square );
  if( !resource.has_value() ) return;
  if( *resource == e_natural_resource::fish )
    return render_fish( viz, renderer, where, world_square );
  render_sprite( renderer, where, resource_tile( *resource ) );
}

// This function is for squares that have some overlay but also
// reveal some part of the underlying tile in a pixelated way so
// blend with surrounding squares that might lack the overlay.
void render_pixelated_overlay_transitions(
    rr::Renderer& renderer, Coord where,
    Coord const world_square, IVisibility const& viz,
    refl::enum_map<e_cdirection, bool> const& has_overlay,
    e_tile                                    overlay_tile ) {
  rr::Painter painter = renderer.painter();
  // The below will render 9 pieces, and will do so with dif-
  // ferent depixelation stages and alphas depending on the
  // overlay status of the surrounding squares.
  // +------------+
  // |  |      |  |
  // |--+------+--|
  // |  |      |  |
  // |  |      |  |
  // |--+------+--|
  // |  |      |  |
  // +------------+
  Rect const tile_rect = Rect::from( Coord{}, g_tile_delta );

  int const kTotalEdgeThickness = 8;
  // Must be even because it straddles two tiles.
  static_assert( kTotalEdgeThickness % 2 == 0 );

  int const kEdgeThickness = kTotalEdgeThickness / 2;

  // --------------- Center ----------------

  bool const self_overlay = has_overlay[e_cdirection::c];
  if( self_overlay )
    render_sprite_section(
        painter, overlay_tile,
        where +
            Delta{ .w = kEdgeThickness, .h = kEdgeThickness },
        tile_rect.edges_removed( kEdgeThickness ) );

  // --------------- Sides ----------------

  // Draw a transition on tile with overlay status X to an adja-
  // cent tile that has the overlay.
  auto x_to_overlay = [&]( Delta delta, Rect part,
                           e_cardinal_direction d ) {
    if( part.area() == 0 ) return;
    double const stage_from = !self_overlay ? 1.0 : 0.0;
    double const stage_to   = !self_overlay ? 0.5 : 0.0;
    double const stage_slope =
        abs( stage_to - stage_from ) / kEdgeThickness;
    double     stage_nw = 0;
    gfx::dsize gradient = {};
    switch( d ) {
      case e_cardinal_direction::n:
        stage_nw = !self_overlay ? 0.5 : 0.0;
        gradient = { .h = stage_slope };
        break;
      case e_cardinal_direction::s:
        stage_nw = !self_overlay ? 1.0 : 0.0;
        gradient = { .h = -stage_slope };
        break;
      case e_cardinal_direction::e:
        stage_nw = !self_overlay ? 1.0 : 0.0;
        gradient = { .w = -stage_slope };
        break;
      case e_cardinal_direction::w:
        stage_nw = !self_overlay ? 0.5 : 0.0;
        gradient = { .w = stage_slope };
        break;
    }
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             stage_nw );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient, gradient );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        ( where + delta ).to_gfx().to_double() );
    rr::Painter painter = renderer.painter();
    render_sprite_section( painter, overlay_tile, where + delta,
                           part );
  };

  // Draw a transition on a tile with overlay to an adjacent tile
  // that has no overlay.
  auto overlay_to_non_overlay = [&]( Delta delta, Rect part,
                                     e_cardinal_direction d ) {
    if( !self_overlay ) return;
    double const stage_from = 0.0;
    double const stage_to   = 0.5;
    double const stage_slope =
        abs( stage_to - stage_from ) / kEdgeThickness;
    gfx::dsize gradient = {};
    double     stage_nw = 0;
    switch( d ) {
      case e_cardinal_direction::n:
        stage_nw = 0.5;
        gradient = { .h = -stage_slope };
        break;
      case e_cardinal_direction::s:
        stage_nw = 0.0;
        gradient = { .h = stage_slope };
        break;
      case e_cardinal_direction::e:
        stage_nw = 0.0;
        gradient = { .w = stage_slope };
        break;
      case e_cardinal_direction::w:
        stage_nw = 0.5;
        gradient = { .w = -stage_slope };
        break;
    }
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             stage_nw );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_gradient, gradient );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.stage_anchor,
        ( where + delta ).to_gfx().to_double() );
    rr::Painter painter = renderer.painter();
    render_sprite_section( painter, overlay_tile, where + delta,
                           part );
  };

  auto transition = [&]( Delta delta, Rect rect,
                         e_cardinal_direction d ) {
    if( !has_overlay[to_cdirection( d )] )
      overlay_to_non_overlay( delta, rect, d );
    else {
      Coord const moved =
          world_square.moved( to_direction( d ) );
      if( !self_overlay && !viz.on_map( moved ) )
        // This prevents drawing overlay transitions at the edge
        // of the map when those tiles have no overlay.
        return;
      x_to_overlay( delta, rect, d );
    }
  };

  // Top middle.
  {
    e_cardinal_direction const d = e_cardinal_direction::n;
    Delta const delta{ .w = kEdgeThickness, .h = 0 };
    Rect const  rect =
        tile_rect.with_new_left_edge( kEdgeThickness )
            .with_new_right_edge( g_tile_width - kEdgeThickness )
            .with_new_bottom_edge( kEdgeThickness );
    transition( delta, rect, d );
  }

  // Bottom middle.
  {
    e_cardinal_direction const d = e_cardinal_direction::s;
    Delta const                delta{ .w = kEdgeThickness,
                                      .h = g_tile_height - kEdgeThickness };
    Rect const                 rect =
        tile_rect.with_new_left_edge( kEdgeThickness )
            .with_new_right_edge( g_tile_width - kEdgeThickness )
            .with_new_top_edge( g_tile_height - kEdgeThickness );
    transition( delta, rect, d );
  }

  // Left middle.
  {
    e_cardinal_direction const d = e_cardinal_direction::w;
    Delta const delta{ .w = 0, .h = kEdgeThickness };
    Rect const  rect =
        tile_rect.with_new_top_edge( kEdgeThickness )
            .with_new_right_edge( kEdgeThickness )
            .with_new_bottom_edge( g_tile_height -
                                   kEdgeThickness );
    transition( delta, rect, d );
  }

  // Right middle.
  {
    e_cardinal_direction const d = e_cardinal_direction::e;
    Delta const delta{ .w = g_tile_width - kEdgeThickness,
                       .h = kEdgeThickness };
    Rect const  rect =
        tile_rect.with_new_top_edge( kEdgeThickness )
            .with_new_left_edge( g_tile_width - kEdgeThickness )
            .with_new_bottom_edge( g_tile_height -
                                   kEdgeThickness );
    transition( delta, rect, d );
  }

  // --------------- Corners ----------------

  double const kDepixelateStage      = .3;
  double const kDepixelateStageLight = .7;
  double const kDpAlpha              = 1.0;
  double const kDpAlphaLight         = 0.8;

  auto corner = [&]( Delta delta, Rect part,
                     e_cardinal_direction d1,
                     e_cardinal_direction d2 ) {
    if( part.area() == 0 ) return;
    Coord const moved = world_square.moved( to_direction( d1 ) )
                            .moved( to_direction( d2 ) );
    if( !self_overlay && !viz.on_map( moved ) )
      // This prevents drawing shadow transitions at the edge
      // of the map when those tiles are visible.
      return;
    bool const no_overlay1 = !has_overlay[to_cdirection( d1 )];
    bool const no_overlay2 = !has_overlay[to_cdirection( d2 )];
    int const  no_overlay_count =
        ( no_overlay1 ? 1 : 0 ) + ( no_overlay2 ? 1 : 0 );
    double stage = 0.0, alpha = 0.0;
    if( !self_overlay ) {
      stage = ( no_overlay_count == 0 )   ? kDepixelateStage
              : ( no_overlay_count == 1 ) ? kDepixelateStageLight
                                          : 1.0;
      alpha = ( no_overlay_count == 0 )   ? kDpAlpha
              : ( no_overlay_count == 1 ) ? kDpAlphaLight
                                          : 1.0;
    } else {
      stage = ( no_overlay_count == 0 ) ? 0.0
              : ( no_overlay_count == 1 )
                  ? kDepixelateStage
                  : kDepixelateStageLight;
      alpha = ( no_overlay_count == 0 )   ? 1.0
              : ( no_overlay_count == 1 ) ? kDpAlpha
                                          : kDpAlphaLight;
    }
    if( stage == 1.0 || alpha == 0.0 ) return;
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                             stage );
    rr::Painter painter = renderer.painter();
    render_sprite_section( painter, overlay_tile, where + delta,
                           part );
  };

  // Upper left.
  {
    e_cardinal_direction const d1 = e_cardinal_direction::w;
    e_cardinal_direction const d2 = e_cardinal_direction::n;
    Delta const                delta{ .w = 0, .h = 0 };
    Rect const                 rect =
        tile_rect.with_new_right_edge( kEdgeThickness )
            .with_new_bottom_edge( kEdgeThickness );
    corner( delta, rect, d1, d2 );
  }

  // Upper right.
  {
    e_cardinal_direction const d1 = e_cardinal_direction::n;
    e_cardinal_direction const d2 = e_cardinal_direction::e;
    Delta const delta{ .w = g_tile_width - kEdgeThickness,
                       .h = 0 };
    Rect const  rect =
        tile_rect
            .with_new_left_edge( g_tile_width - kEdgeThickness )
            .with_new_bottom_edge( kEdgeThickness );
    corner( delta, rect, d1, d2 );
  }

  // Bottom left.
  {
    e_cardinal_direction const d1 = e_cardinal_direction::s;
    e_cardinal_direction const d2 = e_cardinal_direction::w;
    Delta const                delta{ .w = 0,
                                      .h = g_tile_width - kEdgeThickness };
    Rect const                 rect =
        tile_rect.with_new_right_edge( kEdgeThickness )
            .with_new_top_edge( g_tile_width - kEdgeThickness );
    corner( delta, rect, d1, d2 );
  }

  // Bottom right.
  {
    e_cardinal_direction const d1 = e_cardinal_direction::e;
    e_cardinal_direction const d2 = e_cardinal_direction::s;
    Delta const delta{ .w = g_tile_width - kEdgeThickness,
                       .h = g_tile_width - kEdgeThickness };
    Rect const  rect =
        tile_rect
            .with_new_left_edge( g_tile_width - kEdgeThickness )
            .with_new_top_edge( g_tile_width - kEdgeThickness );
    corner( delta, rect, d1, d2 );
  }
}

void render_visible_terrain_square( rr::Renderer& renderer,
                                    Coord         where,
                                    Coord const   world_square,
                                    IVisibility const& viz ) {
  MapSquare const& square = viz.square_at( world_square );
  if( square.surface == e_surface::water ) {
    render_terrain_ocean_square( renderer, where, viz, square,
                                 world_square );
    if( square.river.has_value() )
      render_river_on_ocean( viz, renderer, where, world_square,
                             square );
  } else {
    render_terrain_land_square( viz, renderer, where,
                                world_square, square );
    if( square.river.has_value() )
      render_river_on_land( viz, renderer, where, world_square,
                            square,
                            /*no_bank=*/false );
  }
  render_land_overlay( viz, renderer, where, world_square,
                       square );
  render_plow_if_present( renderer, where,
                          viz.square_at( world_square ) );
  render_resources( renderer, viz, where, world_square );
  render_road_if_present( renderer, where, viz, world_square );
  render_lost_city_rumor( renderer, where, square );
}

// An "overlay" can represent some kind of sprite that is over-
// layed on top of land. This could be the sprite that hides un-
// explored tiles or could be the sprite that partially covers
// fog-of-war tiles. These sprites need to be rendered on top of
// land tiles and pixelated at the boundaries (with tiles that do
// not have the overlay) for smooth transitions.
struct OverlayInfo {
  refl::enum_map<e_cdirection, bool> surroundings;
  bool                               fully_surrounded = false;
};

OverlayInfo surrounding_overlays(
    IVisibility const& viz, Coord tile,
    refl::enum_map<e_tile_visibility, bool> const& targets ) {
  OverlayInfo info;
  info.fully_surrounded = true;
  for( e_cdirection d : refl::enum_values<e_cdirection> ) {
    bool const overlay = targets[viz.visible( tile.moved( d ) )];
    info.surroundings[d]  = overlay;
    info.fully_surrounded = info.fully_surrounded && overlay;
  }
  return info;
}

} // namespace

void render_landscape_square_if_not_fully_hidden(
    rr::Renderer& renderer, Coord where,
    Coord const world_square, IVisibility const& viz,
    TerrainRenderOptions const& options ) {
  bool const fully_hidden =
      surrounding_overlays(
          viz, world_square,
          { { e_tile_visibility::hidden, true } } )
          .fully_surrounded;
  if( fully_hidden ) return;
  render_visible_terrain_square( renderer, where, world_square,
                                 viz );

  // Always last.
  rr::Painter painter = renderer.painter();
  if( options.grid )
    painter.draw_empty_rect( Rect::from( where, g_tile_delta ),
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

void render_obfuscation_overlay(
    rr::Renderer& renderer, Coord where,
    Coord const world_square, IVisibility const& viz,
    TerrainRenderOptions const& options ) {
  { // Unexplored.
    OverlayInfo const hidden = surrounding_overlays(
        viz, world_square,
        { { e_tile_visibility::hidden, true } } );
    if( hidden.fully_surrounded ) {
      render_sprite( renderer, where, e_tile::terrain_hidden );
    } else {
      render_pixelated_overlay_transitions(
          renderer, where, world_square, viz,
          hidden.surroundings, e_tile::terrain_hidden );
    }
  }

  // Fog of war.
  if( options.render_fog_of_war ) {
    OverlayInfo const fogged = surrounding_overlays(
        viz, world_square,
        { { e_tile_visibility::fogged, true },
          { e_tile_visibility::hidden, true } } );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             config_gfx.fog_of_war_alpha );
    if( fogged.fully_surrounded ) {
      render_sprite( renderer, where, e_tile::terrain_fogged );
    } else {
      render_pixelated_overlay_transitions(
          renderer, where, world_square, viz,
          fogged.surroundings, e_tile::terrain_fogged );
    }
  }
}

void render_terrain_square_merged(
    rr::Renderer& renderer, Coord where, Coord world_square,
    IVisibility const&          viz,
    TerrainRenderOptions const& options ) {
  render_landscape_square_if_not_fully_hidden(
      renderer, where, world_square, viz, options );
  render_obfuscation_overlay( renderer, where, world_square, viz,
                              options );
}

void render_landscape_buffer(
    rr::Renderer& renderer, IVisibility const& viz,
    TerrainRenderOptions const&   options,
    gfx::Matrix<rr::VertexRange>& tile_bounds ) {
  auto start_time = chrono::system_clock::now();
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );

  renderer.clear_buffer( rr::e_render_buffer::landscape );
  renderer.clear_buffer( rr::e_render_buffer::landscape_annex );
  SCOPED_RENDERER_MOD_SET( buffer_mods.buffer,
                           rr::e_render_buffer::landscape );
  gfx::rect_iterator const ri( viz.rect_tiles() );
  for( gfx::point const p : ri ) {
    Coord const square  = Coord::from_gfx( p );
    tile_bounds[square] = renderer.range_for( [&] {
      render_landscape_square_if_not_fully_hidden(
          renderer, square * g_tile_delta, square, viz,
          options );
    } );
  }

  auto end_time = chrono::system_clock::now();
  lg.info(
      "rendered landscape buffer: {}ms with {} vertices, "
      "occupying {:.2f}MB.",
      chrono::duration_cast<chrono::milliseconds>( end_time -
                                                   start_time )
          .count(),
      renderer.buffer_vertex_count(
          rr::e_render_buffer::landscape ),
      renderer.buffer_size_mb(
          rr::e_render_buffer::landscape ) );
}

void render_obfuscation_buffer(
    rr::Renderer& renderer, IVisibility const& viz,
    TerrainRenderOptions const&   options,
    gfx::Matrix<rr::VertexRange>& tile_bounds ) {
  auto start_time = chrono::system_clock::now();
  SCOPED_RENDERER_MOD_SET( painter_mods.repos.use_camera, true );

  renderer.clear_buffer( rr::e_render_buffer::obfuscation );
  renderer.clear_buffer(
      rr::e_render_buffer::obfuscation_annex );
  SCOPED_RENDERER_MOD_SET( buffer_mods.buffer,
                           rr::e_render_buffer::obfuscation );
  gfx::rect_iterator const ri( viz.rect_tiles() );
  for( gfx::point const p : ri ) {
    Coord const square  = Coord::from_gfx( p );
    tile_bounds[square] = renderer.range_for( [&] {
      render_obfuscation_overlay( renderer,
                                  square * g_tile_delta, square,
                                  viz, options );
    } );
  }

  auto end_time = chrono::system_clock::now();
  lg.info(
      "rendered obfuscation buffer: {}ms with {} vertices, "
      "occupying {:.2f}MB.",
      chrono::duration_cast<chrono::milliseconds>( end_time -
                                                   start_time )
          .count(),
      renderer.buffer_vertex_count(
          rr::e_render_buffer::obfuscation ),
      renderer.buffer_size_mb(
          rr::e_render_buffer::obfuscation ) );
}

} // namespace rn
