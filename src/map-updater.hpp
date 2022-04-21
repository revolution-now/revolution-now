/****************************************************************
**map-updater.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-04-21.
*
* Description: Handlers when a map square needs to be modified.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "coord.hpp"
#include "game-state.hpp"
#include "map-square.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/function-ref.hpp"

namespace rn {

// This function should be used whenever a map square
// (specifically, a MapSquare object) must be updated as it will
// handler re-rendering the surrounding squares.
void modify_map_square(
    TerrainState& terrain_state, Coord tile,
    rr::Renderer&                          renderer,
    base::function_ref<void( MapSquare& )> mutator );

} // namespace rn
