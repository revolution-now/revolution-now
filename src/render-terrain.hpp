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

// ss
#include "ss/matrix.hpp"

// render
#include "render/renderer.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rr {
struct Renderer;
}

namespace rn {

struct Visibility;

/****************************************************************
** TerrainRenderOptions
*****************************************************************/
struct TerrainRenderOptions {
  bool render_forests   = true;
  bool render_resources = true;
  bool render_lcrs      = true;
  bool grid             = false;
};

/****************************************************************
** Rendering
*****************************************************************/
// Render the terrain square as seen by a player (meaning that it
// might not be visible).
void render_terrain_square(
    rr::Renderer& renderer, Coord where, Coord world_square,
    Visibility const& viz, TerrainRenderOptions const& options );

// Render the entire map to the landscape buffer. Should only be
// called once after the map is generated.
void render_terrain( rr::Renderer&               renderer,
                     Visibility const&           viz,
                     TerrainRenderOptions const& options,
                     Matrix<rr::TileBounds>&     tile_bounds );

} // namespace rn
