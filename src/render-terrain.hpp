/****************************************************************
**render-terrain.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-27.
*
* Description: Renders individual terrain squares.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "game-state.hpp"

// render
#include "render/renderer.hpp"

namespace rn {

/****************************************************************
** Rendering
*****************************************************************/
// This will fully render a map square with no units or colonies
// on it.
void render_terrain_square( TerrainState const& terrain_state,
                            rr::Renderer& renderer, Coord where,
                            Coord world_square );

// Render the entire map to the landscape buffer. Should only be
// called once after the map is generated.
void render_terrain( TerrainState const& terrain_state,
                     rr::Renderer&       renderer );

} // namespace rn
