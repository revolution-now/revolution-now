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
#include "colony-id.hpp"
#include "orders.hpp"
#include "unit-id.hpp"
#include "unit.hpp"

// render
#include "render/renderer.hpp"

namespace rn {

/****************************************************************
** UnitShadow
*****************************************************************/
struct UnitShadow {
  gfx::pixel color  = default_color();
  W          offset = default_offset();

  static gfx::pixel default_color();
  static W          default_offset();
};

/****************************************************************
** UnitRenderingOptions
*****************************************************************/
struct UnitRenderOptions {
  bool              flag   = false;
  maybe<UnitShadow> shadow = {};
};

/****************************************************************
** Public API
*****************************************************************/
// Render an actual unit.
void render_unit( rr::Renderer& renderer, Coord where, UnitId id,
                  UnitRenderOptions const& options = {} );

// Render an abstract unit of a given type.
void render_unit_type( rr::Painter& painter, Coord where,
                       e_unit_type              unit_type,
                       UnitRenderOptions const& options = {} );

void render_colony( rr::Painter& painter, Coord where,
                    ColonyId id );

// Note that the coordinate provided here is the coordinate of
// the unit whose flag is being drawn, not the flag position it-
// self (which could be shifted to another corner).
void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders );

// Note that the coordinate provided here is the coordinate of
// the unit whose flag is being drawn, not the flag position it-
// self (which could be shifted to another corner).
void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, UnitId id );

} // namespace rn
