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
#include "render/stencil.rds.hpp"

// gfx
#include "gfx/coord.hpp"
#include "gfx/pixel.hpp"

// C++ standard library
#include <string_view>

namespace rr {
struct Renderer;
struct Painter;
}

namespace rn {

/****************************************************************
** Global Cache (TODO: find a better way of doing this)
*****************************************************************/
void init_sprites( rr::Renderer& renderer );

base::valid_or<std::string> validate_sprites(
    rr::Renderer& renderer );

void deinit_sprites();

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
gfx::size sprite_size( e_tile tile );

gfx::rect trimmed_area_for( e_tile tile );

// Use this to set a value in the trimmed tile cache for unit
// testing purposes. Computing this cache for unit tests proved
// to be too tricky because currently it requires access to the
// renderer which itself can't be initialized without initial-
// izing video, etc. So easier just to do it this way.
void testing_set_trimmed_cache( e_tile tile, gfx::rect trimmed );

// Similary for burrow stencil tiles.
void testing_set_burrow_cache( e_tile tile, int id );

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
void render_sprite_dulled( rr::Renderer& renderer, Coord where,
                           e_tile tile, bool dulled );

// This allows drawing a section of a sprite. The `section` rect
// has its origin relative to the upper left corner of the
// sprite. Any parts of the section that fall outside of the
// sprite will be clipped. The destination pixel coord is the lo-
// cation of the clipped section of the sprite (i.e. the visible
// part); it is /not/ the origin that the full sprite would have
// had were it visible.
void render_sprite_section( rr::Renderer& renderer, e_tile tile,
                            gfx::point pixel_coord,
                            gfx::rect source );

// Unfortunately this requires rendering the sprite five times.
void render_sprite_outline( rr::Renderer& renderer,
                            gfx::point where, e_tile tile,
                            gfx::pixel outline_color );

/****************************************************************
** Stencils
*****************************************************************/
rr::StencilPlan::sprite stencil_plan_for(
    gfx::pixel key_color, e_tile replacement_tile );

rr::StencilPlan::fixed stencil_plan_for(
    gfx::pixel key_color, gfx::pixel replacement_color );

void render_sprite_stencil( rr::Renderer& renderer, Coord where,
                            e_tile tile, e_tile replacement_tile,
                            gfx::pixel key_color );

/****************************************************************
** Burrowing.
*****************************************************************/
// Burrowing refers to the mechanism that allows e.g. the native
// dwellings and colonies to appear like they are nested in the
// forest instead of just appearing to hover over the forest
// tiles. For each sprite that supports burrowing, a second
// sprite is precomputed that marks the bottom edge of the opaque
// region of the sprite with some depixelation stages so that the
// forest can then be drawn over that depixelation guide to make
// it appear that the forest grow is encroaching on the bottom
// each of the dwelling sprite.
//
// In the example of the dwelling nested in the forest, the "ren-
// dered tile" would be the forest tile and the "reference tile"
// would be the dwelling tile. But note that the reference tile
// generally may be larger than the reference tile (as is the
// case for the dwelling). To handle this we draw multiple bur-
// rowed forest tiles over the dwelling, and the "offset from
// center" parameter tells which portion of the dwelling burrowed
// sprite we want to look at.
rr::TexturedDepixelatePlan burrowed_sprite_plan_for(
    e_tile rendered_tile, e_tile reference_tile,
    gfx::size tile_offset_from_center );

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
    gfx::size size_tiles,   // tile coords, including border
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
