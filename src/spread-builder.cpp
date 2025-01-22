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
#include "spread-algo.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
TileSpread build_tile_spread(
    TileSpreadConfigMulti const& configs ) {
  SpreadSpecs const specs = [&] {
    SpreadSpecs res;
    res.bounds        = configs.bounds;
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
    res.label_policy  = configs.label_policy;
    res.group_spacing = specs.group_spacing;
    for( auto config_it = configs.tiles.begin();
         Spread const& icon_spread : icon_spreads.spreads ) {
      CHECK( config_it != configs.tiles.end() );
      TileSpreadSpec tile_spread_spec{
        .icon_spread = icon_spread, .tile = config_it->tile };
      if( config_it->has_x )
        tile_spread_spec.overlay_tile = e_tile::boycott;
      res.spreads.push_back( tile_spread_spec );
      ++config_it;
    }
    return res;
  }();
  return render_plan_for_tile_spread( tile_spreads );
}

} // namespace rn
