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
#include "tx.hpp"
#include "util.hpp"

// Rnl
#include "rnl/tiles.hpp"

// C++ standard library
#include <string_view>

namespace rn {

namespace detail {
constexpr int tile_pixel_size{ 32 };
}

constexpr inline SX    g_tile_width{ detail::tile_pixel_size };
constexpr inline SY    g_tile_height{ detail::tile_pixel_size };
constexpr inline Scale g_tile_scale{ detail::tile_pixel_size };

inline Rect g_tile_rect = Rect::from(
    Coord{},
    Delta{ W{ 1 } * g_tile_width, H{ 1 } * g_tile_height } );

inline Delta g_tile_delta =
    Delta{ W{ 1 } * g_tile_width, H{ 1 } * g_tile_height };

struct ND sprite {
  Delta size() const { return source.delta(); }

  // NOTE: the size of this texture will NOT be the same as the
  // size of the sprite.
  Texture const* texture{};
  Rect           source{};
  Scale          scale{};
};
NOTHROW_MOVE( sprite );

ND sprite const& lookup_sprite( e_tile tile );

void render_sprite( Texture& tx, e_tile tile, Y pixel_row,
                    X pixel_col, int rot, int flip_x );
void render_sprite( Texture& tx, e_tile tile,
                    Coord pixel_coord );
void render_sprite( Texture& tx, e_tile tile, Coord pixel_coord,
                    int rot, int flip_x );
// This will render the sprite clipped to the given rect, where
// the coordinates of the rect are in pixels and the origin is
// relative to the origin of the sprite.
void render_sprite_clip( Texture& tx, e_tile tile,
                         Coord pixel_coord, Rect const& clip );
void render_sprite_grid( Texture& tx, e_tile tile, Y tile_row,
                         X tile_col, int rot, int flip_x );
void render_sprite_grid( Texture& tx, e_tile tile, Coord coord,
                         int rot, int flip_x );

// Here the word "tile" is used to mean "repeat the sprite in a
// tiled pattern within the rectangle".
void tile_sprite( Texture& tx, e_tile tile, Rect const& rect );

// This function will render a rectangle with border, but
// where the rectangle and border are comprised of tiles,
// not pixels.  All given tiles must have the same dimensions.
void render_rect_of_sprites_with_border(
    Texture& dest,        // where to draw it
    Coord    dest_origin, // pixel coord of upper left
    Delta    size_tiles,  // tile coords, including border
    e_tile   middle,      //
    e_tile   top,         //
    e_tile   bottom,      //
    e_tile   left,        //
    e_tile   right,       //
    e_tile   top_left,    //
    e_tile   top_right,   //
    e_tile   bottom_left, //
    e_tile   bottom_right //
);

void load_tile_maps();
// void render_tile_map( std::string_view name );

} // namespace rn
