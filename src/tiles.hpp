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
#include "geo-types.hpp"
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
  privateer,
  caravel,
  soldier
};

struct ND sprite {
  // try making these const
  Texture const* texture{};
  Rect           source{};
  W              w{};
  H              h{};
};

void load_sprites();

ND sprite lookup_sprite( std::string_view name );

void render_sprite( Texture const& tx, g_tile tile, Y pixel_row,
                    X pixel_col, int rot, int flip_x );
void render_sprite( Texture const& tx, g_tile tile,
                    Coord pixel_coord, int rot, int flip_x );
void render_sprite_grid( Texture const& tx, g_tile tile,
                         Y tile_row, X tile_col, int rot,
                         int flip_x );

void load_tile_maps();
// void render_tile_map( std::string_view name );

} // namespace rn
