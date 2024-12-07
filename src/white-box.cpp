/****************************************************************
**white-box.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-06.
*
* Description: Handles things related to view mode and/or the
*              white inspection square that can be moved around
*              the map.
*
*****************************************************************/
#include "white-box.hpp"

// Revolution Now
#include "unit-mgr.hpp"

// ss
#include "ss/land-view.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::gfx::point;
using ::gfx::rect;

auto& white_box_ref( auto& ss ) {
  // NOTE: this is the only place where this should appear.
  return ss.land_view.white_box;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
gfx::point white_box_tile( SSConst const& ss ) {
  return white_box_ref( ss );
}

void set_white_box_tile( SS& ss, point const tile ) {
  white_box_ref( ss ) = tile;
}

point find_a_good_white_box_location(
    SSConst const& ss, base::maybe<UnitId> const last_unit_input,
    rect const covered_tiles ) {
  point res;

  auto tile_visible = [&]( point const p ) {
    // The dec-size is needed otherwise the tiles just to the
    // right and under the viewport will be considered visible.
    return p.is_inside( covered_tiles.with_dec_size() );
  };

  // Try #1
  res = white_box_tile( ss );
  if( tile_visible( res ) ) return res;

  // Try #2
  if( last_unit_input.has_value() ) {
    UnitId const last_unit_id = *last_unit_input;
    if( ss.units.exists( last_unit_id ) ) {
      auto const unit_tile =
          coord_for_unit_multi_ownership( ss, last_unit_id );
      if( unit_tile.has_value() ) { res = *unit_tile; }
    }
  }
  if( tile_visible( res ) ) return res;

  // Last resort.
  res = covered_tiles.center();

  return res;
}

} // namespace rn
