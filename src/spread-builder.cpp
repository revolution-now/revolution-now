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
#include "spread-algo.hpp"
#include "tiles.hpp"

using namespace std;

namespace rn {

/****************************************************************
** Public API.
*****************************************************************/
TileSpread build_tile_spread(
    TileSpreadOptions const& options ) {
  SpreadSpecs const specs = [&] {
    SpreadSpecs res;
    res.bounds        = options.bounds;
    res.group_spacing = options.group_spacing;
    for( auto const& [tile, count] : options.tiles ) {
      res.specs.push_back( SpreadSpec{
        .count = count,
        .trimmed =
            opaque_area_for( tile ).horizontal_slice() } );
    }
    return res;
  }();
  Spreads const icon_spreads = compute_icon_spread( specs );
  TileSpreadSpecs const tile_spreads = [&] {
    TileSpreadSpecs res;
    res.label_policy  = options.label_policy;
    res.group_spacing = specs.group_spacing;
    for( auto tile_it = options.tiles.begin();
         Spread const& icon_spread : icon_spreads.spreads ) {
      CHECK( tile_it != options.tiles.end() );
      res.spreads.push_back( TileSpreadSpec{
        .icon_spread = icon_spread, .tile = tile_it->tile } );
      ++tile_it;
    }
    return res;
  }();
  return render_plan_for_tile_spread( tile_spreads );
}

} // namespace rn
