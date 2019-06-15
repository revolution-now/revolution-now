/****************************************************************
**tiles.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Handles loading and retrieving tiles
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "sdl-util.hpp"
#include "util.hpp"

// SDL
#include "SDL.h"

// C++ standard library
#include <string_view>

namespace rn {

namespace detail {
constexpr int tile_pixel_size{32};
}

constexpr inline SX    g_tile_width{detail::tile_pixel_size};
constexpr inline SY    g_tile_height{detail::tile_pixel_size};
constexpr inline Scale g_tile_scale{detail::tile_pixel_size};

inline Rect g_tile_rect = Rect::from(
    Coord{}, Delta{W{1} * g_tile_width, H{1} * g_tile_height} );

inline Delta g_tile_delta =
    Delta{W{1} * g_tile_width, H{1} * g_tile_height};

enum class ND g_tile {
  water,
  land,
  land_1_side,
  land_2_sides,
  land_3_sides,
  land_4_sides,
  land_corner,

  fog,
  fog_1_side,
  fog_corner,

  terrain_grass,

  panel,
  panel_edge_left,
  panel_slate,
  panel_slate_1_side,
  panel_slate_2_sides,

  wood_middle,
  wood_left_edge,

  free_colonist,
  privateer,
  caravel,
  soldier,

  menu_top_left,
  menu_body,
  menu_top,
  menu_left,
  menu_bottom,
  menu_bottom_left,
  menu_right,
  menu_top_right,
  menu_bottom_right,

  menu_item_sel_back,
  menu_hdr_sel_back,

  menu_bar_0,
  menu_bar_1,
  menu_bar_2,

  button_up_ul,
  button_up_um,
  button_up_ur,
  button_up_ml,
  button_up_mm,
  button_up_mr,
  button_up_ll,
  button_up_lm,
  button_up_lr,
  button_down_ul,
  button_down_um,
  button_down_ur,
  button_down_ml,
  button_down_mm,
  button_down_mr,
  button_down_ll,
  button_down_lm,
  button_down_lr,

  checkers
};

struct ND sprite {
  // try making these const
  Texture const* texture{};
  Rect           source{};
  Scale          scale{};
};

ND sprite const& lookup_sprite( g_tile tile );

void render_sprite( Texture const& tx, g_tile tile, Y pixel_row,
                    X pixel_col, int rot, int flip_x );
void render_sprite( Texture const& tx, g_tile tile,
                    Coord pixel_coord, int rot, int flip_x );
// This will render the sprite clipped to the given rect, where
// the coordinates of the rect are in pixels and the origin is
// relative to the origin of the sprite.
void render_sprite_clip( Texture const& tx, g_tile tile,
                         Coord pixel_coord, Rect const& clip );
void render_sprite_grid( Texture const& tx, g_tile tile,
                         Y tile_row, X tile_col, int rot,
                         int flip_x );
void render_sprite_grid( Texture const& tx, g_tile tile,
                         Coord coord, int rot, int flip_x );

// Here the word "tile" is used to mean "repeat the sprite in a
// tiled pattern within the rectangle".
void tile_sprite( Texture const& tx, g_tile tile,
                  Rect const& rect );

// This function will render a rectangle with border, but
// where the rectangle and border are comprised of tiles,
// not pixels.  All given tiles must have the same dimensions.
void render_rect_of_sprites_with_border(
    Texture& dest,        // where to draw it
    Coord    dest_origin, // pixel coord of upper left
    Delta    size_tiles,  // tile coords, including border
    g_tile   middle,      //
    g_tile   top,         //
    g_tile   bottom,      //
    g_tile   left,        //
    g_tile   right,       //
    g_tile   top_left,    //
    g_tile   top_right,   //
    g_tile   bottom_left, //
    g_tile   bottom_right //
);

void load_tile_maps();
// void render_tile_map( std::string_view name );

} // namespace rn
