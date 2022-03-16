/****************************************************************
**terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Representation of the physical world.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "land-square.hpp"

// render
#include "render/renderer.hpp"

namespace rn {

inline constexpr auto world_size = Delta{ 200_w, 100_h };

// FIXME: temporary.
void generate_terrain();

Delta world_size_tiles();
Delta world_size_pixels();
Rect  world_rect_tiles();
Rect  world_rect_pixels();

bool square_exists( Y y, X x );
bool square_exists( Coord coord );

LandSquare const&        square_at( Coord coord );
maybe<LandSquare const&> maybe_square_at( Coord coord );

// Throws if coord is not on map.
bool terrain_is_land( Coord coord );

/****************************************************************
** Rendering
*****************************************************************/
// This will fully render a land square with no units or colonies
// on it.
void render_terrain_square( rr::Renderer& renderer,
                            Coord         world_square,
                            Coord         pixel_coord );

void render_terrain( Rect src_tiles, rr::Renderer& renderer,
                     Coord dest_pixel_coord );

/****************************************************************
** Testing
*****************************************************************/
void generate_unittest_terrain();

} // namespace rn
