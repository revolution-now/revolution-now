/****************************************************************
**harbor-view-ships.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-06.
*
* Description: Common things for the views in the harbor that
*              contain ships.
*
*****************************************************************/
#include "harbor-view-ships.hpp"

// Revolution Now
#include "tiles.hpp"

// config
#include "config/ui.rds.hpp"
#include "config/unit-type.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::gfx::size;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
// FIXME: for some reason clang warns about a missing prototype
// if we dont have this, but this should not be necessary here.
void render_unit_glow( rr::Renderer& renderer, point const where,
                       e_unit_type const type,
                       int const downsample_factor );

void render_unit_glow( rr::Renderer& renderer, point const where,
                       e_unit_type const type,
                       int const downsample_factor ) {
  UnitTypeAttributes const& desc = unit_attr( type );
  e_tile const tile              = desc.tile;
  render_sprite_silhouette(
      renderer, where - size{ .w = downsample_factor }, tile,
      config_ui.harbor.unit_highlight_color );
}

} // namespace rn
