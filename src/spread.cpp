/****************************************************************
**spread.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Algorithm for doing the icon spread.
*
*****************************************************************/
#include "spread.hpp"

// Revolution Now
#include "tiles.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

int bounds_for_output( IconSpreadSpec const& spec,
                       IconSpread const& spread ) {
  if( spread.count == 0 ) return 0;
  return ( spread.count - 1 ) * spread.spacing + spec.width;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
IconSpreads compute_icon_spread( IconSpreadSpecs const& specs ) {
  IconSpreads spreads;
  spreads.group_spacing = specs.group_spacing;

  // Initially set all to max count and space.
  for( IconSpreadSpec const& spec : specs.specs ) {
    auto& spread   = spreads.spreads.emplace_back();
    spread.count   = spec.count;
    spread.spacing = spec.width + 1;
    spread.width   = spec.width;
  }

  CHECK_EQ( specs.specs.size(), spreads.spreads.size() );

  auto const total_bounds = [&] {
    int total = 0;
    for( auto&& [spec, spread] :
         rv::zip( specs.specs, spreads.spreads ) ) {
      total += bounds_for_output( spec, spread );
    }
    total += specs.group_spacing *
             std::max( spreads.spreads.size() - 1, 0ul );
    return total;
  };

  auto const decrement_spacing_for_largest = [&] {
    IconSpread* largest = {};
    int largest_bounds  = 0;
    for( auto&& [spec, spread] :
         rv::zip( specs.specs, spreads.spreads ) ) {
      int const bounds = bounds_for_output( spec, spread );
      if( bounds > largest_bounds ) {
        largest_bounds = bounds;
        largest        = &spread;
      }
    }
    CHECK( largest );
    if( largest->spacing == 1 ) {
      if( largest->count == 1 ) return false;
      --largest->count;
      return true;
    }
    --largest->spacing;
    return true;
  };

  while( total_bounds() > specs.bounds ) {
    CHECK( decrement_spacing_for_largest() );
  }

  CHECK_LE( total_bounds(), specs.bounds );

  return spreads;
}

void render_icon_spread(
    rr::Renderer& renderer, point where,
    RenderableIconSpreads const& rspreads ) {
  point p = where;
  auto const tiled =
      rv::zip( rspreads.spreads.spreads, rspreads.icons );
  for( auto const [spread, tile] : tiled ) {
    for( int i = 0; i < spread.count; ++i ) {
      render_sprite( renderer, p, tile );
      p.x += spread.spacing;
    }
    if( spread.count > 0 )
      p.x += std::max( ( spread.width - spread.spacing ), 0 );
    p.x += rspreads.spreads.group_spacing;
  }
}

int spread_width_for_tile( e_tile const tile ) {
  return sprite_size( tile ).w;
}

} // namespace rn
