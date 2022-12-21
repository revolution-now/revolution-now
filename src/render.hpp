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
#include "orders.hpp"
#include "unit-id.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/unit-type.rds.hpp"
#include "ss/unit.rds.hpp"

// render
#include "render/renderer.hpp"

namespace rn {

struct Colony;
struct Dwelling;
struct NativeUnit;
struct SSConst;
struct Unit;

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
  bool              flag          = false;
  maybe<UnitShadow> shadow        = {};
  bool              outline_right = false;
  // This is only relevant if outlining is enabled.
  gfx::pixel outline_color = gfx::pixel::black();
};

/****************************************************************
** Public API
*****************************************************************/
// Render an actual unit.
void render_unit( rr::Renderer& renderer, Coord where,
                  Unit const&              unit,
                  UnitRenderOptions const& options = {} );

// Render an actual native unit.
void render_native_unit( rr::Renderer& renderer, Coord where,
                         SSConst const&           ss,
                         NativeUnit const&        native_unit,
                         UnitRenderOptions const& options = {} );

// Render an abstract unit of a given type.
void render_unit_type( rr::Painter& painter, Coord where,
                       e_unit_type              unit_type,
                       UnitRenderOptions const& options = {} );

void render_unit_depixelate(
    rr::Renderer& renderer, Coord where, Unit const& unit,
    double stage, UnitRenderOptions const& options = {} );

void render_unit_depixelate_to( rr::Renderer& renderer,
                                Coord where, Unit const& unit,
                                e_unit_type target, double stage,
                                UnitRenderOptions options = {} );

void render_colony( rr::Painter& painter, Coord where,
                    Colony const& colony );

void render_dwelling( rr::Painter& painter, Coord where,
                      Dwelling const& dwelling );

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
                              Coord where, Unit const& unit );

} // namespace rn
