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
#include "command.hpp"
#include "unit-id.hpp"

// config
#include "config/tile-enum-fwd.hpp"

// ss
#include "ss/nation.rds.hpp"
#include "ss/native-enums.rds.hpp"
#include "ss/unit-type.rds.hpp"
#include "ss/unit.rds.hpp"

// gfx
#include "gfx/pixel.hpp"

namespace rr {
struct Painter;
struct Renderer;
}

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
enum class e_flag_count {
  none,
  single,
  // This is used to visually indicate when there are multiple
  // units on a tile; as in the OG, we draw two stacked flags.
  multiple
};

struct UnitRenderOptions {
  e_flag_count flag = e_flag_count::none;

  maybe<UnitShadow> shadow = {};

  bool outline_right = false;

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
void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_unit_type              unit_type,
                       UnitRenderOptions const& options = {} );

void render_native_unit_type(
    rr::Renderer& renderer, Coord where,
    e_native_unit_type       unit_type,
    UnitRenderOptions const& options = {} );

void render_unit_depixelate(
    rr::Renderer& renderer, Coord where, Unit const& unit,
    double stage, UnitRenderOptions const& options = {} );

void render_unit_depixelate_to( rr::Renderer& renderer,
                                Coord where, SSConst const& ss,
                                Unit const& unit, e_tile target,
                                double            stage,
                                UnitRenderOptions options = {} );

void render_native_unit_depixelate(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    NativeUnit const& unit, double stage,
    UnitRenderOptions const& options = {} );

void render_native_unit_depixelate_to(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    NativeUnit const& unit, e_tile target, double stage,
    UnitRenderOptions options = {} );

void render_colony( rr::Painter& painter, Coord where,
                    Colony const& colony );

void render_dwelling( rr::Renderer& renderer, Coord where,
                      SSConst const&  ss,
                      Dwelling const& dwelling );

// Note that the coordinate provided here is the coordinate of
// the unit whose flag is being drawn, not the flag position it-
// self (which could be shifted to another corner).
void render_unit_flag( rr::Renderer& renderer, Coord where,
                       e_unit_type type, e_nation nation,
                       unit_orders const& orders );

void render_shadow_hightlight_border( rr::Renderer& renderer,
                                      gfx::rect     rect,
                                      gfx::pixel left_and_bottom,
                                      gfx::pixel top_and_right );

} // namespace rn
