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
** Rendering Building Blocks
*****************************************************************/
void render_unit( rr::Renderer& renderer, Coord where, UnitId id,
                  bool with_icon );
void render_unit( rr::Renderer& renderer, Coord where,
                  e_unit_type unit_type );

void render_colony( rr::Renderer& renderer, Coord where,
                    ColonyId id );

void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders );

void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, UnitId id );

} // namespace rn
