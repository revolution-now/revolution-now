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
#include "aliases.hpp"
#include "coord.hpp"
#include "fb.hpp"
#include "land-square.hpp"
#include "sg-macros.hpp"
#include "tx.hpp"

namespace rn {

inline constexpr auto world_size = Delta{ 200_w, 100_h };

DECLARE_SAVEGAME_SERIALIZERS( Terrain );

// FIXME: temporary.
void generate_terrain();

Delta world_size_tiles();
Delta world_size_pixels();
Rect  world_rect_tiles();
Rect  world_rect_pixels();

bool square_exists( Y y, X x );
bool square_exists( Coord coord );

LandSquare const&          square_at( Coord coord );
OptRef<LandSquare const> maybe_square_at( Coord coord );

// Throws if coord is not on map.
bool terrain_is_land( Coord coord );

/****************************************************************
** Rendering
*****************************************************************/
// This will fully render a land square with no units or colonies
// on it.
void render_terrain_square( Texture& tx, Coord world_square,
                            Coord pixel_coord );

// This function will render the terrain in large blocks and so
// it will in general overshoot the edges of the destination rec-
// tangle on the target texture instead of just rendering the
// minimal set of tiles (in `src_tiles`). This is to simplify the
// implementation.
void render_terrain( Rect src_tiles, Texture& dest,
                     Coord dest_pixel_coord );

/****************************************************************
** Testing
*****************************************************************/
void generate_unittest_terrain();

} // namespace rn
