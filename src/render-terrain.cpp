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
#include "tiles.hpp"
#include "world-map.hpp"

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

double g_tile_overlap_scaling       = .8;
double g_tile_overlap_width_percent = .2;

double g_tile_overlap_stage_one_alpha = .85;
double g_tile_overlap_stage_one_stage = .70;

double g_tile_overlap_stage_two_alpha = .5;
double g_tile_overlap_stage_two_stage = .85;

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
  maybe<MapSquare const&> left =
      maybe_square_at( terrain_state, world_square - 1_w );
  if( left.has_value() && left->surface == e_surface::land )
    return left->ground;

  maybe<MapSquare const&> up =
      maybe_square_at( terrain_state, world_square - 1_h );
  if( up.has_value() && up->surface == e_surface::land )
    return up->ground;

  maybe<MapSquare const&> right =
      maybe_square_at( terrain_state, world_square + 1_w );
  if( right.has_value() && right->surface == e_surface::land )
    return right->ground;

  maybe<MapSquare const&> down =
      maybe_square_at( terrain_state, world_square + 1_h );
  if( down.has_value() && down->surface == e_surface::land )
    return down->ground;

  maybe<MapSquare const&> up_left =
      maybe_square_at( terrain_state, world_square - 1_w - 1_h );
  if( up_left.has_value() &&
      up_left->surface == e_surface::land )
    return up_left->ground;

  maybe<MapSquare const&> up_right =
      maybe_square_at( terrain_state, world_square - 1_h + 1_w );
  if( up_right.has_value() &&
      up_right->surface == e_surface::land )
    return up_right->ground;

  maybe<MapSquare const&> down_right =
      maybe_square_at( terrain_state, world_square + 1_w + 1_h );
  if( down_right.has_value() &&
      down_right->surface == e_surface::land )
    return down_right->ground;

  maybe<MapSquare const&> down_left =
      maybe_square_at( terrain_state, world_square + 1_h - 1_w );
  if( down_left.has_value() &&
      down_left->surface == e_surface::land )
    return down_left->ground;

  return nothing;
}

e_tile overlay_tile( MapSquare const& square ) {
  DCHECK( square.overlay.has_value() );
  switch( *square.overlay ) {
    case e_land_overlay::forest: {
      if( square.ground == e_ground_terrain::desert )
        return e_tile::terrain_forest_scrub_island;
      return e_tile::terrain_forest_island;
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
  maybe<MapSquare const&> west =
      maybe_square_at( terrain_state, world_square - 1_w );
  maybe<MapSquare const&> north =
      maybe_square_at( terrain_state, world_square - 1_h );
  maybe<MapSquare const&> east =
      maybe_square_at( terrain_state, world_square + 1_w );
  maybe<MapSquare const&> south =
      maybe_square_at( terrain_state, world_square + 1_h );

  int chop_pixels =
      std::lround( g_tile_delta.w._ * chop_percent );
  W chop_w = W{ chop_pixels };
  H chop_h = H{ chop_pixels };

  if( west.has_value() ) {
    // Render east part of western tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.w -= chop_w;
    src.x += chop_w;
    dst.x += 0_w;
    SCOPED_RENDERER_MOD( painter_mods.depixelate.anchor,
                         dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, *west, world_square - 1_w );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }
  if( north.has_value() ) {
    // Render bottom part of north tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.h -= chop_h;
    src.y += chop_h;
    dst.y += 0_h;
    SCOPED_RENDERER_MOD( painter_mods.depixelate.anchor,
                         dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, *north, world_square - 1_h );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }
  if( south.has_value() ) {
    // Render northern part of southern tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.h -= chop_h;
    src.y += 0_h;
    dst.y += chop_h;
    SCOPED_RENDERER_MOD( painter_mods.depixelate.anchor,
                         dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, *south, world_square + 1_h );
    if( ground )
      render_sprite_section( painter,
                             tile_for_ground_terrain( *ground ),
                             dst, src );
  }
  if( east.has_value() ) {
    // Render west part of eastern tile.
    Rect  src = Rect::from( Coord{}, g_tile_delta );
    Coord dst = where;
    src.w -= chop_w;
    src.x += 0_w;
    dst.x += chop_w;
    SCOPED_RENDERER_MOD( painter_mods.depixelate.anchor,
                         dst + anchor_offset );
    // Need a new painter since we changed the mods.
    rr::Painter             painter = renderer.painter();
    maybe<e_ground_terrain> ground  = ground_terrain_for_square(
         terrain_state, *east, world_square + 1_w );
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
    SCOPED_RENDERER_MOD( painter_mods.alpha,
                         g_tile_overlap_stage_two_alpha );
    SCOPED_RENDERER_MOD( painter_mods.depixelate.stage,
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
    SCOPED_RENDERER_MOD( painter_mods.alpha,
                         g_tile_overlap_stage_one_alpha );
    SCOPED_RENDERER_MOD( painter_mods.depixelate.stage,
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

  maybe<MapSquare const&> left =
      maybe_square_at( terrain_state, world_square - 1_w );
  maybe<MapSquare const&> up =
      maybe_square_at( terrain_state, world_square - 1_h );
  maybe<MapSquare const&> right =
      maybe_square_at( terrain_state, world_square + 1_w );
  maybe<MapSquare const&> up_left =
      maybe_square_at( terrain_state, world_square - 1_h - 1_w );
  maybe<MapSquare const&> up_right =
      maybe_square_at( terrain_state, world_square + 1_w - 1_h );

  // This should be done at the end.
  if( up_right.has_value() &&
      up_right->surface == e_surface::land &&
      up->surface == e_surface::water &&
      right->surface == e_surface::water )
    render_sprite_stencil(
        painter, where,
        e_tile::terrain_ocean_canal_corner_up_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
  if( up_left.has_value() &&
      up_left->surface == e_surface::land &&
      up->surface == e_surface::water &&
      left->surface == e_surface::water )
    render_sprite_stencil(
        painter, where,
        e_tile::terrain_ocean_canal_corner_up_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
}

// Pass in the painter as well for efficiency.
void render_terrain_land_square(
    TerrainState const& terrain_state, rr::Painter& painter,
    rr::Renderer& renderer, Coord where, Coord world_square,
    MapSquare const& square ) {
  DCHECK( square.surface == e_surface::land );
  render_terrain_ground( terrain_state, painter, renderer, where,
                         world_square, square.ground );
  if( square.overlay.has_value() ) {
    e_tile overlay = overlay_tile( square );
    render_sprite( painter, where, overlay );
  }
}

void render_beach_corners( rr::Painter& painter, Coord where,
                           maybe<MapSquare const&> up,
                           maybe<MapSquare const&> right,
                           maybe<MapSquare const&> down,
                           maybe<MapSquare const&> left,
                           maybe<MapSquare const&> up_left,
                           maybe<MapSquare const&> up_right,
                           maybe<MapSquare const&> down_right,
                           maybe<MapSquare const&> down_left ) {
  // Render beach corners.
  if( up_left.has_value() &&
      up_left->surface == e_surface::land &&
      left->surface == e_surface::water &&
      up->surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_up_left );
  if( up_right.has_value() &&
      up_right->surface == e_surface::land &&
      up->surface == e_surface::water &&
      right->surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_up_right );
  if( down_right.has_value() &&
      down_right->surface == e_surface::land &&
      down->surface == e_surface::water &&
      right->surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_down_right );
  if( down_left.has_value() &&
      down_left->surface == e_surface::land &&
      down->surface == e_surface::water &&
      left->surface == e_surface::water )
    render_sprite( painter, where,
                   e_tile::terrain_beach_corner_down_left );
}

} // namespace

void render_terrain_ocean_square(
    rr::Renderer& renderer, rr::Painter& painter, Coord where,
    TerrainState const& terrain_state, MapSquare const& square,
    Coord world_square ) {
  DCHECK( square.surface == e_surface::water );

  maybe<MapSquare const&> up =
      maybe_square_at( terrain_state, world_square - 1_h );
  maybe<MapSquare const&> right =
      maybe_square_at( terrain_state, world_square + 1_w );
  maybe<MapSquare const&> down =
      maybe_square_at( terrain_state, world_square + 1_h );
  maybe<MapSquare const&> left =
      maybe_square_at( terrain_state, world_square - 1_w );
  maybe<MapSquare const&> up_left =
      maybe_square_at( terrain_state, world_square - 1_h - 1_w );
  maybe<MapSquare const&> up_right =
      maybe_square_at( terrain_state, world_square + 1_w - 1_h );
  maybe<MapSquare const&> down_right =
      maybe_square_at( terrain_state, world_square + 1_h + 1_w );
  maybe<MapSquare const&> down_left =
      maybe_square_at( terrain_state, world_square - 1_w + 1_h );

  // Treat off-map tiles as water for rendering purposes.
  bool water_up = !up || up->surface == e_surface::water;
  bool water_right =
      !right || right->surface == e_surface::water;
  bool water_down = !down || down->surface == e_surface::water;
  bool water_left = !left || left->surface == e_surface::water;

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

  e_tile        water_tile        = {};
  e_tile        beach_tile        = {};
  maybe<e_tile> second_water_tile = {};
  maybe<e_tile> second_beach_tile = {};
  maybe<e_tile> surf_tile         = {};

  auto is_land_if_exists = [&]( e_direction d ) {
    maybe<MapSquare const&> s = maybe_square_at(
        terrain_state, world_square.moved( d ) );
    return s.has_value() && s->surface == e_surface::land;
  };

  auto to_mask = []( bool l, bool r ) {
    return ( ( l ? 1 : 0 ) << 1 ) | ( r ? 1 : 0 );
  };

  switch( mask ) {
    case 0b1110: {
      // land on left.
      DCHECK( left.has_value() );
      bool top_open  = is_land_if_exists( e_direction::nw );
      bool down_open = is_land_if_exists( e_direction::sw );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile = e_tile::terrain_ocean_up_right_down_c_c;
          beach_tile = e_tile::terrain_beach_up_right_down_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile = e_tile::terrain_ocean_up_right_down_c_o;
          beach_tile = e_tile::terrain_beach_up_right_down_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile = e_tile::terrain_ocean_up_right_down_o_c;
          beach_tile = e_tile::terrain_beach_up_right_down_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile = e_tile::terrain_ocean_up_right_down_o_o;
          beach_tile = e_tile::terrain_beach_up_right_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0111: {
      // land on top.
      DCHECK( up.has_value() );
      bool left_open  = is_land_if_exists( e_direction::nw );
      bool right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile = e_tile::terrain_ocean_right_down_left_c_c;
          beach_tile = e_tile::terrain_beach_right_down_left_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_right_down_left_c_o;
          beach_tile = e_tile::terrain_beach_right_down_left_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_right_down_left_o_c;
          beach_tile = e_tile::terrain_beach_right_down_left_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_right_down_left_o_o;
          beach_tile = e_tile::terrain_beach_right_down_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1011: {
      // land on right.
      DCHECK( right.has_value() );
      bool top_open  = is_land_if_exists( e_direction::ne );
      bool down_open = is_land_if_exists( e_direction::se );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile = e_tile::terrain_ocean_down_left_up_c_c;
          beach_tile = e_tile::terrain_beach_down_left_up_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile = e_tile::terrain_ocean_down_left_up_c_o;
          beach_tile = e_tile::terrain_beach_down_left_up_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile = e_tile::terrain_ocean_down_left_up_o_c;
          beach_tile = e_tile::terrain_beach_down_left_up_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile = e_tile::terrain_ocean_down_left_up_o_o;
          beach_tile = e_tile::terrain_beach_down_left_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1101: {
      // land on bottom.
      DCHECK( down.has_value() );
      bool left_open  = is_land_if_exists( e_direction::sw );
      bool right_open = is_land_if_exists( e_direction::se );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile = e_tile::terrain_ocean_left_up_right_c_c;
          beach_tile = e_tile::terrain_beach_left_up_right_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_left_up_right_c_o;
          beach_tile = e_tile::terrain_beach_left_up_right_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_left_up_right_o_c;
          beach_tile = e_tile::terrain_beach_left_up_right_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_left_up_right_o_o;
          beach_tile = e_tile::terrain_beach_left_up_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0110: {
      // land on left and top.
      DCHECK( left.has_value() );
      bool down_open  = is_land_if_exists( e_direction::sw );
      bool right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( down_open, right_open ) ) {
        case 0b00:
          // down closed, right closed.
          water_tile = e_tile::terrain_ocean_right_down_c_c;
          beach_tile = e_tile::terrain_beach_right_down_c_c;
          surf_tile  = e_tile::terrain_surf_right_down_c_c;
          break;
        case 0b01:
          // down closed, right open.
          water_tile = e_tile::terrain_ocean_right_down_o_c;
          beach_tile = e_tile::terrain_beach_right_down_o_c;
          surf_tile  = e_tile::terrain_surf_right_down_o_c;
          break;
        case 0b10:
          // down open, right closed.
          water_tile = e_tile::terrain_ocean_right_down_c_o;
          beach_tile = e_tile::terrain_beach_right_down_c_o;
          surf_tile  = e_tile::terrain_surf_right_down_c_o;
          break;
        case 0b11:
          // down open, right open.
          water_tile = e_tile::terrain_ocean_right_down_o_o;
          beach_tile = e_tile::terrain_beach_right_down_o_o;
          surf_tile  = e_tile::terrain_surf_right_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0011: {
      // land on top and right.
      DCHECK( up.has_value() );
      bool left_open = is_land_if_exists( e_direction::nw );
      bool down_open = is_land_if_exists( e_direction::se );
      switch( to_mask( left_open, down_open ) ) {
        case 0b00:
          // left closed, down closed.
          water_tile = e_tile::terrain_ocean_down_left_c_c;
          beach_tile = e_tile::terrain_beach_down_left_c_c;
          surf_tile  = e_tile::terrain_surf_down_left_c_c;
          break;
        case 0b01:
          // left closed, down open.
          water_tile = e_tile::terrain_ocean_down_left_c_o;
          beach_tile = e_tile::terrain_beach_down_left_c_o;
          surf_tile  = e_tile::terrain_surf_down_left_c_o;
          break;
        case 0b10:
          // left open, down closed.
          water_tile = e_tile::terrain_ocean_down_left_o_c;
          beach_tile = e_tile::terrain_beach_down_left_o_c;
          surf_tile  = e_tile::terrain_surf_down_left_o_c;
          break;
        case 0b11:
          // left open, down open.
          water_tile = e_tile::terrain_ocean_down_left_o_o;
          beach_tile = e_tile::terrain_beach_down_left_o_o;
          surf_tile  = e_tile::terrain_surf_down_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1001: {
      // land on right and bottom.
      DCHECK( right.has_value() );
      bool left_open = is_land_if_exists( e_direction::sw );
      bool top_open  = is_land_if_exists( e_direction::ne );
      switch( to_mask( left_open, top_open ) ) {
        case 0b00:
          // left closed, top closed.
          water_tile = e_tile::terrain_ocean_left_up_c_c;
          beach_tile = e_tile::terrain_beach_left_up_c_c;
          surf_tile  = e_tile::terrain_surf_left_up_c_c;
          break;
        case 0b01:
          // left closed, top open.
          water_tile = e_tile::terrain_ocean_left_up_c_o;
          beach_tile = e_tile::terrain_beach_left_up_c_o;
          surf_tile  = e_tile::terrain_surf_left_up_c_o;
          break;
        case 0b10:
          // left open, top closed.
          water_tile = e_tile::terrain_ocean_left_up_o_c;
          beach_tile = e_tile::terrain_beach_left_up_o_c;
          surf_tile  = e_tile::terrain_surf_left_up_o_c;
          break;
        case 0b11:
          // left open, top open.
          water_tile = e_tile::terrain_ocean_left_up_o_o;
          beach_tile = e_tile::terrain_beach_left_up_o_o;
          surf_tile  = e_tile::terrain_surf_left_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1100: {
      // land on bottom and left.
      DCHECK( down.has_value() );
      bool up_open    = is_land_if_exists( e_direction::nw );
      bool right_open = is_land_if_exists( e_direction::se );
      switch( to_mask( up_open, right_open ) ) {
        case 0b00:
          // up closed, right closed.
          water_tile = e_tile::terrain_ocean_up_right_c_c;
          beach_tile = e_tile::terrain_beach_up_right_c_c;
          surf_tile  = e_tile::terrain_surf_up_right_c_c;
          break;
        case 0b01:
          // up closed, right open.
          water_tile = e_tile::terrain_ocean_up_right_c_o;
          beach_tile = e_tile::terrain_beach_up_right_c_o;
          surf_tile  = e_tile::terrain_surf_up_right_c_o;
          break;
        case 0b10:
          // up open, right closed.
          water_tile = e_tile::terrain_ocean_up_right_o_c;
          beach_tile = e_tile::terrain_beach_up_right_o_c;
          surf_tile  = e_tile::terrain_surf_up_right_o_c;
          break;
        case 0b11:
          // up open, right open.
          water_tile = e_tile::terrain_ocean_up_right_o_o;
          beach_tile = e_tile::terrain_beach_up_right_o_o;
          surf_tile  = e_tile::terrain_surf_up_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1010: {
      // land on left and right.
      DCHECK( left.has_value() );
      bool up_left_open  = is_land_if_exists( e_direction::nw );
      bool up_right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( up_left_open, up_right_open ) ) {
        case 0b00:
          // up left closed, up right closed.
          water_tile = e_tile::terrain_ocean_up_down_up_c_c;
          beach_tile = e_tile::terrain_beach_up_down_up_c_c;
          break;
        case 0b01:
          // up left closed, up right open.
          water_tile = e_tile::terrain_ocean_up_down_up_c_o;
          beach_tile = e_tile::terrain_beach_up_down_up_c_o;
          break;
        case 0b10:
          // up left open, up right closed.
          water_tile = e_tile::terrain_ocean_up_down_up_o_c;
          beach_tile = e_tile::terrain_beach_up_down_up_o_c;
          break;
        case 0b11:
          // up left open, up right open.
          water_tile = e_tile::terrain_ocean_up_down_up_o_o;
          beach_tile = e_tile::terrain_beach_up_down_up_o_o;
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
          break;
        case 0b01:
          // down left closed, down right open.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_c_o;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_c_o;
          break;
        case 0b10:
          // down left open, down right closed.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_o_c;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_o_c;
          break;
        case 0b11:
          // down left open, down right open.
          second_water_tile =
              e_tile::terrain_ocean_up_down_down_o_o;
          second_beach_tile =
              e_tile::terrain_beach_up_down_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0101: {
      // land on top and bottom.
      DCHECK( up.has_value() );
      bool up_left_open   = is_land_if_exists( e_direction::nw );
      bool down_left_open = is_land_if_exists( e_direction::sw );
      switch( to_mask( up_left_open, down_left_open ) ) {
        case 0b00:
          // up left closed, down left closed.
          water_tile = e_tile::terrain_ocean_left_right_left_c_c;
          beach_tile = e_tile::terrain_beach_left_right_left_c_c;
          break;
        case 0b01:
          // up left closed, down left open.
          water_tile = e_tile::terrain_ocean_left_right_left_c_o;
          beach_tile = e_tile::terrain_beach_left_right_left_c_o;
          break;
        case 0b10:
          // up left open, down left closed.
          water_tile = e_tile::terrain_ocean_left_right_left_o_c;
          beach_tile = e_tile::terrain_beach_left_right_left_o_c;
          break;
        case 0b11:
          // up left open, down left open.
          water_tile = e_tile::terrain_ocean_left_right_left_o_o;
          beach_tile = e_tile::terrain_beach_left_right_left_o_o;
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
          break;
        case 0b01:
          // up right closed, down right open.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_c_o;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_c_o;
          break;
        case 0b10:
          // up right open, down right closed.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_o_c;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_o_c;
          break;
        case 0b11:
          // up right open, down right open.
          second_water_tile =
              e_tile::terrain_ocean_left_right_right_o_o;
          second_beach_tile =
              e_tile::terrain_beach_left_right_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b1000: {
      // land on right, bottom, left.
      DCHECK( left.has_value() );
      bool left_open  = is_land_if_exists( e_direction::nw );
      bool right_open = is_land_if_exists( e_direction::ne );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile = e_tile::terrain_ocean_up_c_c;
          beach_tile = e_tile::terrain_beach_up_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_up_c_o;
          beach_tile = e_tile::terrain_beach_up_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_up_o_c;
          beach_tile = e_tile::terrain_beach_up_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_up_o_o;
          beach_tile = e_tile::terrain_beach_up_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0100: {
      // land on bottom, left, top.
      DCHECK( left.has_value() );
      bool top_open  = is_land_if_exists( e_direction::ne );
      bool down_open = is_land_if_exists( e_direction::se );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile = e_tile::terrain_ocean_right_c_c;
          beach_tile = e_tile::terrain_beach_right_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile = e_tile::terrain_ocean_right_c_o;
          beach_tile = e_tile::terrain_beach_right_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile = e_tile::terrain_ocean_right_o_c;
          beach_tile = e_tile::terrain_beach_right_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile = e_tile::terrain_ocean_right_o_o;
          beach_tile = e_tile::terrain_beach_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0010: {
      // land on left, top, right.
      DCHECK( left.has_value() );
      bool left_open  = is_land_if_exists( e_direction::sw );
      bool right_open = is_land_if_exists( e_direction::se );
      switch( to_mask( left_open, right_open ) ) {
        case 0b00:
          // left closed, right closed.
          water_tile = e_tile::terrain_ocean_down_c_c;
          beach_tile = e_tile::terrain_beach_down_c_c;
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_down_c_o;
          beach_tile = e_tile::terrain_beach_down_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_down_o_c;
          beach_tile = e_tile::terrain_beach_down_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_down_o_o;
          beach_tile = e_tile::terrain_beach_down_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0001: {
      // land on top, right, bottom.
      DCHECK( up.has_value() );
      bool top_open  = is_land_if_exists( e_direction::nw );
      bool down_open = is_land_if_exists( e_direction::sw );
      switch( to_mask( top_open, down_open ) ) {
        case 0b00:
          // top closed, bottom closed.
          water_tile = e_tile::terrain_ocean_left_c_c;
          beach_tile = e_tile::terrain_beach_left_c_c;
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile = e_tile::terrain_ocean_left_c_o;
          beach_tile = e_tile::terrain_beach_left_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile = e_tile::terrain_ocean_left_o_c;
          beach_tile = e_tile::terrain_beach_left_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile = e_tile::terrain_ocean_left_o_o;
          beach_tile = e_tile::terrain_beach_left_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0000:
      // land on all sides.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_island;
      beach_tile = e_tile::terrain_beach_island;
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

  render_sprite_stencil( painter, where, water_tile,
                         e_tile::terrain_ocean,
                         gfx::pixel::black() );
  if( second_water_tile.has_value() )
    render_sprite_stencil( painter, where, *second_water_tile,
                           e_tile::terrain_ocean,
                           gfx::pixel::black() );
  render_sprite( painter, where, beach_tile );
  if( second_beach_tile.has_value() )
    render_sprite( painter, where, *second_beach_tile );
  if( surf_tile.has_value() ) {
    SCOPED_RENDERER_MOD( painter_mods.cycling.enabled, true );
    rr::Painter painter = renderer.painter();
    render_sprite( painter, where, *surf_tile );
  }

  // Render canals.
  if( up_left.has_value() &&
      up_left->surface == e_surface::water &&
      left->surface == e_surface::land &&
      up->surface == e_surface::land )
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_up_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
  if( up_right.has_value() &&
      up_right->surface == e_surface::water &&
      up->surface == e_surface::land &&
      right->surface == e_surface::land )
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_up_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
  if( down_right.has_value() &&
      down_right->surface == e_surface::water &&
      down->surface == e_surface::land &&
      right->surface == e_surface::land )
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_down_right,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );
  if( down_left.has_value() &&
      down_left->surface == e_surface::water &&
      down->surface == e_surface::land &&
      left->surface == e_surface::land )
    render_sprite_stencil(
        painter, where, e_tile::terrain_ocean_canal_down_left,
        e_tile::terrain_ocean_canal_background,
        gfx::pixel::black() );

  render_beach_corners( painter, where, up, right, down, left,
                        up_left, up_right, down_right,
                        down_left );
}

// Pass in the painter as well for efficiency.
void render_terrain_square( TerrainState const& terrain_state,
                            rr::Renderer& renderer, Coord where,
                            Coord world_square ) {
  rr::Painter      painter = renderer.painter();
  MapSquare const& square =
      square_at( terrain_state, world_square );
  if( square.surface == e_surface::water )
    render_terrain_ocean_square( renderer, painter, where,
                                 terrain_state, square,
                                 world_square );
  else
    render_terrain_land_square( terrain_state, painter, renderer,
                                where, world_square, square );
  if( g_show_grid )
    painter.draw_empty_rect( Rect::from( where, g_tile_delta ),
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel{ 0, 0, 0, 30 } );
}

/****************************************************************
** Lua Bindings
*****************************************************************/
namespace {

LUA_FN( toggle_grid, void ) {
  g_show_grid = !g_show_grid;
  lg.debug( "terrain grid is {}.", g_show_grid ? "on" : "off" );
}

LUA_FN( set_tile_chop_multiplier, void, double mult ) {
  g_tile_overlap_scaling = std::clamp( mult, 0.0, 2.0 );
  lg.debug( "setting tile overlap multiplier to {}.",
            g_tile_overlap_scaling );
}

} // namespace

} // namespace rn
