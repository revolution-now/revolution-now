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

// C+ standard library
#include <chrono>

namespace rn {

struct Plane;

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

/****************************************************************
** Panel Rendering
*****************************************************************/
// TODO: move panel into is own module
Plane* panel_plane();

// FIXME: temporary.
void mark_end_of_turn();
bool was_next_turn_button_clicked();

/****************************************************************
** Miscellaneous Rendering
*****************************************************************/
Plane* effects_plane();

void reset_fade_to_dark( std::chrono::milliseconds wait,
                         std::chrono::milliseconds fade,
                         uint8_t target_alpha );

} // namespace rn
