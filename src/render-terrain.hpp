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

// render
#include "render/renderer.rds.hpp"

// gfx
#include "gfx/coord.hpp"

namespace rr {
struct Renderer;
}

namespace gfx {
template<typename T>
struct Matrix;
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
// might not be visible, in which case nothing will be drawn).
void render_landscape_square_if_not_fully_hidden(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    Coord world_square, Visibility const& viz,
    TerrainRenderOptions const& options );

// Renders the overlays both for unexplored terrain and fog of
// when (when enabled) for the square and possibly surrounding
// squares whose fog extends into this one.
void render_obfuscation_overlay(
    rr::Renderer& renderer, Coord where, Coord world_square,
    Visibility const& viz, TerrainRenderOptions const& options );

// Render the landscape buffer (all tiles).
void render_landscape_buffer(
    rr::Renderer& renderer, SSConst const& ss,
    Visibility const& viz, TerrainRenderOptions const& options,
    gfx::Matrix<rr::VertexRange>& tile_bounds );

// Render the obfuscation buffer (all tiles). The reason that the
// obfuscation overlays get put into a different buffer is so
// that we can draw the entities (e.g. units) in between the
// landscape and obfuscation layers.
void render_obfuscation_buffer(
    rr::Renderer& renderer, Visibility const& viz,
    TerrainRenderOptions const&   options,
    gfx::Matrix<rr::VertexRange>& tile_bounds );

// This is for when a particular terrain square in its entirety
// (including landscape and obfuscation) needs to be rendered in
// isolation, e.g. in the colony view. This shouldn't be used to
// render terrain squares normally in the map view; for that, the
// above functions should be used which will put the landscape
// and obfuscation into separate buffers so that entities (units,
// etc.) can be drawn in between them.
void render_terrain_square_merged(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    Coord world_square, Visibility const& viz,
    TerrainRenderOptions const& options );

} // namespace rn
