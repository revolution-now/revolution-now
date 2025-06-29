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
#include "maybe.hpp"
#include "unit-flag.rds.hpp"
#include "unit-id.hpp"

// gfx
#include "gfx/coord.hpp"
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

enum class e_native_unit_type;
enum class e_unit_type;
enum class e_tile;
enum class e_tribe;

/****************************************************************
** Unit Rendering.
*****************************************************************/
e_tile tile_for_unit_type( e_unit_type unit_type );

gfx::rect trimmed_area_for_unit_type( e_unit_type unit_type );

struct UnitShadow {
  gfx::pixel color = default_color();
  W offset         = default_offset();

  static gfx::pixel default_color();
  static W default_offset();
};

struct UnitRenderOptions {
  maybe<UnitFlagRenderInfo> flag = {};

  maybe<UnitShadow> shadow = {};

  bool outline_right = false;

  // This is only relevant if outlining is enabled.
  gfx::pixel outline_color = gfx::pixel::black();
};

// Render an actual unit.
void render_unit( rr::Renderer& renderer, Coord where,
                  Unit const& unit,
                  UnitRenderOptions const& options );

// Render an actual native unit.
void render_native_unit( rr::Renderer& renderer, Coord where,
                         NativeUnit const& native_unit,
                         UnitRenderOptions const& options );

// Render an abstract unit of a given type.
void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_unit_type unit_type,
                       UnitRenderOptions const& options );

void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_native_unit_type unit_type,
                       UnitRenderOptions const& options );

void render_unit_depixelate( rr::Renderer& renderer, Coord where,
                             Unit const& unit, double stage,
                             UnitRenderOptions const& options );

void render_unit_depixelate_to(
    rr::Renderer& renderer, Coord where, Unit const& unit,
    e_unit_type target, double stage,
    UnitRenderOptions const& from_options,
    UnitRenderOptions const& target_options );

void render_native_unit_depixelate(
    rr::Renderer& renderer, Coord where, NativeUnit const& unit,
    double stage, UnitRenderOptions const& options );

void render_native_unit_depixelate_to(
    rr::Renderer& renderer, Coord where, NativeUnit const& unit,
    e_native_unit_type target, double stage,
    UnitRenderOptions const& from_options,
    UnitRenderOptions const& target_options );

/****************************************************************
** Colony Rendering.
*****************************************************************/
e_tile tile_for_colony( Colony const& colony );

struct ColonyRenderOptions {
  bool render_name       = true;
  bool render_population = true;
  bool render_flag       = true;
};

void render_colony( rr::Renderer& renderer, Coord where,
                    SSConst const& ss, Colony const& colony,
                    ColonyRenderOptions const& options );

/****************************************************************
** Dwelling Rendering.
*****************************************************************/
e_tile dwelling_tile_for_tribe( e_tribe const tribe_type );

e_tile tile_for_dwelling( SSConst const& ss,
                          Dwelling const& dwelling );

void render_dwelling( rr::Renderer& renderer, Coord where,
                      SSConst const& ss,
                      Dwelling const& dwelling );

} // namespace rn
