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

void render_terrain_ocean_square( rr::Painter&     painter,
                                  Coord            where,
                                  MapSquare const& square ) {
  DCHECK( square.surface == e_surface::water );
  e_tile tile = square.sea_lane ? e_tile::terrain_ocean_sea_lane
                                : e_tile::terrain_ocean;
  render_sprite( painter, where, tile );
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

  if( west.has_value() && west->surface == e_surface::land ) {
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
        painter, tile_for_ground_terrain( *west ), dst, src );
  }
  if( north.has_value() && north->surface == e_surface::land ) {
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
        painter, tile_for_ground_terrain( *north ), dst, src );
  }
  if( south.has_value() && south->surface == e_surface::land ) {
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
        painter, tile_for_ground_terrain( *south ), dst, src );
  }
  if( east.has_value() && east->surface == e_surface::land ) {
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
        painter, tile_for_ground_terrain( *east ), dst, src );
  }
}

// Pass in the painter as well for efficiency.
void render_terrain_land_square(
    TerrainState const& terrain_state, rr::Painter& painter,
    rr::Renderer& renderer, Coord where, Coord world_square,
    MapSquare const& square ) {
  DCHECK( square.surface == e_surface::land );
  e_tile tile = tile_for_ground_terrain( square );
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
  if( square.overlay.has_value() ) {
    e_tile overlay = overlay_tile( square );
    render_sprite( painter, where, overlay );
  }
}

} // namespace

// Pass in the painter as well for efficiency.
void render_terrain_square( TerrainState const& terrain_state,
                            rr::Renderer& renderer, Coord where,
                            Coord world_square ) {
  rr::Painter      painter = renderer.painter();
  MapSquare const& square =
      square_at( terrain_state, world_square );
  if( square.surface == e_surface::water )
    render_terrain_ocean_square( painter, where, square );
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
