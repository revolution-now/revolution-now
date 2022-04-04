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

e_tile tile_for_ground_terrain( MapSquare const& square ) {
  return tile_for_ground_terrain( square.ground );
}

e_ground_terrain ground_terrain_for_square(
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

  // FIXME: figure out why control flow gets here.
  // SHOULD_NOT_BE_HERE;
  return e_ground_terrain::arctic;
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
    rr::Painter painter = renderer.painter();
    render_sprite_section(
        painter,
        tile_for_ground_terrain( ground_terrain_for_square(
            terrain_state, *west, world_square - 1_w ) ),
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
    rr::Painter painter = renderer.painter();
    render_sprite_section(
        painter,
        tile_for_ground_terrain( ground_terrain_for_square(
            terrain_state, *north, world_square - 1_h ) ),
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
    rr::Painter painter = renderer.painter();
    render_sprite_section(
        painter,
        tile_for_ground_terrain( ground_terrain_for_square(
            terrain_state, *south, world_square + 1_h ) ),
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
    rr::Painter painter = renderer.painter();
    render_sprite_section(
        painter,
        tile_for_ground_terrain( ground_terrain_for_square(
            terrain_state, *east, world_square + 1_w ) ),
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
    return;
  }

  e_tile water_tile = {};

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
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile = e_tile::terrain_ocean_up_right_down_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile = e_tile::terrain_ocean_up_right_down_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile = e_tile::terrain_ocean_up_right_down_o_o;
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
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_right_down_left_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_right_down_left_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_right_down_left_o_o;
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
          break;
        case 0b01:
          // top closed, bottom open.
          water_tile = e_tile::terrain_ocean_down_left_up_c_o;
          break;
        case 0b10:
          // top open, bottom closed.
          water_tile = e_tile::terrain_ocean_down_left_up_o_c;
          break;
        case 0b11:
          // top open, bottom open.
          water_tile = e_tile::terrain_ocean_down_left_up_o_o;
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
          break;
        case 0b01:
          // left closed, right open.
          water_tile = e_tile::terrain_ocean_left_up_right_c_o;
          break;
        case 0b10:
          // left open, right closed.
          water_tile = e_tile::terrain_ocean_left_up_right_o_c;
          break;
        case 0b11:
          // left open, right open.
          water_tile = e_tile::terrain_ocean_left_up_right_o_o;
          break;
        default: SHOULD_NOT_BE_HERE;
      }
      break;
    }
    case 0b0110:
      // land on left and top.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_right_down;
      break;
    case 0b0011:
      // land on top and right.
      DCHECK( up.has_value() );
      water_tile = e_tile::terrain_ocean_down_left;
      break;
    case 0b1001:
      // land on right and bottom.
      DCHECK( right.has_value() );
      water_tile = e_tile::terrain_ocean_left_up;
      break;
    case 0b1100:
      // land on bottom and left.
      DCHECK( down.has_value() );
      water_tile = e_tile::terrain_ocean_up_right;
      break;
    case 0b1010:
      // land on left and right.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_up_down;
      break;
    case 0b0101:
      // land on top and bottom.
      DCHECK( up.has_value() );
      water_tile = e_tile::terrain_ocean_left_right;
      break;
    case 0b1000:
      // land on right, bottom, left.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_up;
      break;
    case 0b0100:
      // land on bottom, left, top.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_right;
      break;
    case 0b0010:
      // land on left, top, right.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_down;
      break;
    case 0b0001:
      // land on top, right, bottom.
      DCHECK( up.has_value() );
      water_tile = e_tile::terrain_ocean_left;
      break;
    case 0b0000:
      // land on all sides.
      DCHECK( left.has_value() );
      water_tile = e_tile::terrain_ocean_island;
      break;
    default: {
      FATAL( "invalid ocean mask: {}", mask );
    }
  }

  // We have at least one bordering land square, so we need to
  // render a ground tile first because there will be a bit of
  // land visible on this tile.
  e_ground_terrain ground = ground_terrain_for_square(
      terrain_state, square, world_square );
  render_terrain_ground( terrain_state, painter, renderer, where,
                         world_square, ground );

  render_sprite( painter, where, water_tile );
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
