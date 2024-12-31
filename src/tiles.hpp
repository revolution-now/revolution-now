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

// config
#include "config/tile-enum-fwd.hpp"

// render
#include "render/painter.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <string_view>

namespace rr {
struct Renderer;
}

namespace rn {

/****************************************************************
** Global Cache (TODO: find a better way of doing this)
*****************************************************************/
void init_sprites( rr::Renderer& renderer );

/****************************************************************
** Global Constants
*****************************************************************/
namespace detail {
constexpr int tile_pixel_size{ 32 };
}

inline constexpr SX g_tile_width{ detail::tile_pixel_size };
inline constexpr SY g_tile_height{ detail::tile_pixel_size };
inline constexpr Delta g_tile_delta = Delta{
  .w = W{ 1 } * g_tile_width, .h = H{ 1 } * g_tile_height };

/****************************************************************
** Querying Tiles
*****************************************************************/
Delta sprite_size( e_tile tile );

/****************************************************************
** Rendering Tiles
*****************************************************************/
void render_sprite( rr::Renderer& renderer, Coord where,
                    e_tile tile );

void render_sprite( rr::Renderer& renderer, gfx::point where,
                    e_tile tile );

// This one allows stretching the tile.
void render_sprite( rr::Painter& painter, Rect where,
                    e_tile tile );

void render_sprite_silhouette( rr::Renderer& renderer,
                               Coord where, e_tile tile,
                               gfx::pixel color );

// This is used e.g. to dull a commodity in the cargo of a ship
// when the quantity is less than 100, as does the OG.
void render_sprite_dulled( rr::Renderer& renderer, e_tile tile,
                           Coord where, bool dulled );

// This will render a rectangular subsection of the sprite. The
// `source` rect has its origin relative to the upper left corner
// of the sprite in the atlas. Any part of the source rect that
// lies outside of the sprite will be clipped.
void render_sprite_section( rr::Painter& painter, e_tile tile,
                            Coord pixel_coord, Rect source );

/****************************************************************
** Stencils
*****************************************************************/
rr::StencilPlan stencil_plan_for( e_tile replacement_tile,
                                  gfx::pixel key_color );

void render_sprite_stencil( rr::Renderer& renderer, Coord where,
                            e_tile tile, e_tile replacement_tile,
                            gfx::pixel key_color );

/****************************************************************
** Tiling
*****************************************************************/
// Here the word "tile" is used to mean "repeat the sprite in a
// tiled pattern within the rectangle".
void tile_sprite( rr::Renderer& renderer, e_tile tile,
                  Rect const& rect );

// This function will render a rectangle with border, but
// where the rectangle and border are comprised of tiles,
// not pixels.  All given tiles must have the same dimensions.
void render_rect_of_sprites_with_border(
    rr::Renderer& renderer, // where to draw it
    Coord dest_origin,      // pixel coord of upper left
    Delta size_tiles,       // tile coords, including border
    e_tile middle,          //
    e_tile top,             //
    e_tile bottom,          //
    e_tile left,            //
    e_tile right,           //
    e_tile top_left,        //
    e_tile top_right,       //
    e_tile bottom_left,     //
    e_tile bottom_right     //
);

} // namespace rn
