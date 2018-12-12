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

#include "sdl-util.hpp"
#include "util.hpp"

#include <SDL.h>

#include <string_view>

namespace rn {

constexpr inline W g_tile_width{32};
constexpr inline H g_tile_height{32};

constexpr int g_tile_set_width = 8;

enum class ND g_tile {
  water = 0 * g_tile_set_width,
  land,
  land_1_side,
  land_2_sides,
  land_3_sides,
  land_4_sides,
  land_corner,

  fog = 1 * g_tile_set_width,
  fog_1_side,
  fog_corner,

  terrain_grass = 2 * g_tile_set_width,

  panel = 3 * g_tile_set_width,
  panel_edge_left,
  panel_slate,
  panel_slate_1_side,
  panel_slate_2_sides,

  free_colonist = 5 * g_tile_set_width,
  caravel
};

struct ND sprite {
  // try making these const
  Texture const* texture{};
  SDL_Rect       source{};
  W              w{};
  H              h{};
};

void load_sprites();

ND sprite lookup_sprite( std::string_view name );

void render_sprite( g_tile tile, Y pixel_row, X pixel_col,
                    int rot, int flip_x );
void render_sprite_grid( g_tile tile, Y tile_row, X tile_col,
                         int rot, int flip_x );

void load_tile_maps();
void render_tile_map( std::string_view name );

} // namespace rn
