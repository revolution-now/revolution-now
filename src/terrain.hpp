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
#include "enum.hpp"
#include "fb.hpp"
#include "sg-macros.hpp"
#include "tx.hpp"

// Flatbuffers
#include "fb/terrain_generated.h"

namespace rn {

inline constexpr auto world_size = Delta{ 120_w, 60_h };

DECLARE_SAVEGAME_SERIALIZERS( Terrain );

// FIXME: temporary.
void generate_terrain();

enum class e_( crust, water, land );
SERIALIZABLE_ENUM( e_crust );

struct ND Square {
  expect<> check_invariants_safe() const {
    return xp_success_t{};
  }

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, Square,
  ( e_crust, crust ));
  // clang-format on
};
NOTHROW_MOVE( Square );

Delta world_size_tiles();
Delta world_size_pixels();
Rect  world_rect_tiles();
Rect  world_rect_pixels();

bool square_exists( Y y, X x );
bool square_exists( Coord coord );

Square const&          square_at( Coord coord );
Opt<Ref<Square const>> maybe_square_at( Coord coord );

// Throws if coord is not on map.
bool terrain_is_land( Coord coord );

/****************************************************************
** Rendering
*****************************************************************/
// This will fully render a lang square with no units or colonies
// on it.
void render_terrain_square( Texture& tx, Coord world_square,
                            Coord texture_square );

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
