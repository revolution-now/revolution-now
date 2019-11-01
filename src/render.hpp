/****************************************************************
**render.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "orders.hpp"
#include "unit.hpp"

namespace rn {

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
void render_unit( Texture& tx, UnitId id, Coord pixel_coord,
                  bool with_icon );
void render_unit( Texture& tx, e_unit_type unit_type,
                  Coord pixel_coord );

void render_nationality_icon( Texture& dest, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders,
                              Coord         pixel_coord );

void render_nationality_icon( Texture& dest, UnitId id,
                              Coord pixel_coord );

} // namespace rn
