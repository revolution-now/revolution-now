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

/****************************************************************
** Public API.
*****************************************************************/
TileSpreadRenderPlans build_tile_spread_multi(
    TileSpreadConfigMulti const& configs ) {
  SpreadSpecs const specs = [&] {
    SpreadSpecs res;
    res.bounds        = configs.options.bounds;
    res.group_spacing = configs.group_spacing;
    for( auto const& config : configs.tiles ) {
      res.specs.push_back(
          SpreadSpec{ .count   = config.count,
                      .trimmed = opaque_area_for( config.tile )
                                     .horizontal_slice() } );
    }
    return res;
  }();
  Spreads const icon_spreads = compute_icon_spread( specs );
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
        .label_opts  = configs.options.label_opts };
      if( config_it->has_x ) {
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

maybe<TileSpreadRenderPlan> build_tile_spread(
    TileSpreadConfig const& config ) {
  maybe<TileSpreadRenderPlan> res;
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
