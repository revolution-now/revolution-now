/****************************************************************
**spread-render.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Creates and renders spreads of tiles.
*
*****************************************************************/
#pragma once

// rds
#include "spread-render.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

namespace rr {
struct Renderer;
struct ITextometer;
}

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
[[nodiscard]] TileSpreadRenderPlans render_plan_for_tile_spread(
    rr::ITextometer const& textometer,
    TileSpreadSpecs const& tile_spreads );

[[nodiscard]] TileSpreadRenderPlan
render_plan_for_tile_progress_spread(
    rr::ITextometer const& textometer,
    ProgressTileSpreadSpec const& tile_spec );

// This one just spaces the tiles apart by a fixed amount.
[[nodiscard]] TileSpreadRenderPlan
render_plan_for_tile_fixed_spread(
    FixedTileSpreadSpec const& tile_spec );

// Iterate through the non-overlay tiles and keep replacing
// `from` tiles with `to` tiles until either N is reached or we
// encounter a non-`from` tile.
//
// The tile spread framework does not support rendering a single
// spread with multiple different tiles. But that is what we must
// do in some cases in order to replicate the behavior of the OG.
// In particular, there are cases where we have two different
// tiles and the switch from one to the next must happen within a
// single spread AND where we are not sure which spread it will
// happen in (since it depends on e.g. production counts). This
// is tricky to do with the existing spread algo, so instead of
// complicating that to support a case that isn't very common, we
// just take this post-facto approach where we tell the spread
// algo that we're only rendering a single type of tile, then go
// in after and change the first N tiles. NOTE: To work properly,
// the two tiles should have the same (or similar) trimmed dimen-
// sions.
void replace_first_n_tiles( TileSpreadRenderPlans& plans,
                            int n_replace, e_tile from,
                            e_tile to );

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlan const& plan,
    TileSpreadRenderOptions const& options = {} );

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlans const& plans );

} // namespace rn
