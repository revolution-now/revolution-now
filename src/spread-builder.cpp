/****************************************************************
**spread-builder.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Helper for building renderable tile spreads.
*
*****************************************************************/
#include "spread-builder.hpp"

// Revolution Now
#include "spread-render.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

// gfx
#include "gfx/spread-algo.hpp"

using namespace std;

namespace rn {

using ::base::maybe;
using ::gfx::pixel;

/****************************************************************
** Public API.
*****************************************************************/
TileSpreadRenderPlans build_tile_spread_multi(
    TileSpreadConfigMulti const& configs_unadjusted ) {
  TileSpreadConfigMulti const configs = [&] {
    TileSpreadConfigMulti res = configs_unadjusted;
    for( auto& tile_spread : res.tiles ) {
      if( tile_spread.progress_count.has_value() )
        // This is done for two reasons: one because if the
        // progress happens to be larger than the total (which
        // can happen, e.g. if the player transiently has more
        // liberty bells than are needed for the next founding
        // father) then the below spread algo will crash. Second,
        // in that same scenario, if we were to instead cap the
        // player's progress count then there would be a visual
        // discrepancy between how many tiles are rendered and
        // what is shown on the label. So we just increase the
        // total to be equal to the progress, which should lead
        // to the right thing visually happening.
        tile_spread.count = std::max(
            tile_spread.count, *tile_spread.progress_count );
    }
    return res;
  }();
  SpreadSpecs const specs = [&] {
    SpreadSpecs res;
    res.bounds        = configs.options.bounds;
    res.group_spacing = configs.group_spacing;
    for( auto const& config : configs.tiles ) {
      res.specs.push_back(
          SpreadSpec{ .count   = config.count,
                      .trimmed = trimmed_area_for( config.tile )
                                     .horizontal_slice() } );
    }
    return res;
  }();
  Spreads const icon_spreads = [&] {
    Spreads spreads = compute_icon_spread( specs );
    for( auto tiles_it = configs.tiles.begin();
         auto& spread : spreads.spreads ) {
      CHECK( tiles_it != configs.tiles.end() );
      if( tiles_it->progress_count.has_value() ) {
        adjust_rendered_count_for_progress_count(
            spread, *tiles_it->progress_count );
      }
      ++tiles_it;
    }
    return spreads;
  }();
  TileSpreadSpecs const tile_spreads = [&] {
    TileSpreadSpecs res;
    res.label_policy  = configs.options.label_policy;
    res.group_spacing = specs.group_spacing;
    for( auto config_it = configs.tiles.begin();
         Spread const& icon_spread : icon_spreads.spreads ) {
      CHECK( config_it != configs.tiles.end() );
      TileSpreadSpec tile_spread_spec{
        .icon_spread = icon_spread,
        .tile        = config_it->tile,
        .label_opts  = configs.options.label_opts,
        .label_count = config_it->progress_count };
      if( config_it->has_x ) {
        tile_spread_spec.label_opts.color_fg = pixel::red();
        switch( ( *config_it->has_x ) ) {
          case rn::e_red_x_size::small: {
            tile_spread_spec.overlay_tile = e_tile::red_x_16;
            break;
          }
          case rn::e_red_x_size::large: {
            tile_spread_spec.overlay_tile = e_tile::red_x_20;
            break;
          }
        }
      }
      res.spreads.push_back( tile_spread_spec );
      ++config_it;
    }
    return res;
  }();
  return render_plan_for_tile_spread( tile_spreads );
}

TileSpreadRenderPlan build_tile_spread(
    TileSpreadConfig const& config ) {
  TileSpreadRenderPlan res;
  auto plans = build_tile_spread_multi(
      TileSpreadConfigMulti{ .tiles         = { config.tile },
                             .options       = config.options,
                             .group_spacing = 0 } );
  // Could be length zero if the input config has zero count.
  CHECK_LE( plans.plans.size(), 1u );
  if( !plans.plans.empty() ) res = std::move( plans.plans[0] );
  return res;
}

} // namespace rn
