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

struct SSConst;
struct Visibility;

/****************************************************************
** TerrainRenderOptions
*****************************************************************/
struct TerrainRenderOptions {
  bool render_forests    = true;
  bool render_resources  = true;
  bool render_lcrs       = true;
  bool grid              = false;
  bool render_fog_of_war = true;
};

/****************************************************************
** Rendering
*****************************************************************/
// Render the terrain square as seen by a player (meaning that it
// might not be visible).
void render_terrain_square(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    Coord world_square, Visibility const& viz,
    TerrainRenderOptions const& options );

// Render the entire map to the landscape buffer. Should only be
// called once after the map is generated.
void render_terrain( rr::Renderer& renderer, SSConst const& ss,
                     Visibility const&           viz,
                     TerrainRenderOptions const& options,
                     Matrix<rr::VertexRange>&    tile_bounds );

} // namespace rn
