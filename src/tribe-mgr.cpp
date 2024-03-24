/****************************************************************
**tribe-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-06.
*
* Description: High level actions to do with natives.
*
*****************************************************************/
#include "tribe-mgr.hpp"

// Revolution Now
#include "igui.hpp"
#include "imap-updater.hpp"
#include "road.hpp"
#include "tribe-arms.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"
#include "wait.hpp"

// config
#include "config/natives.rds.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {

// We don't delete the owned land in this function because we
// want the caller to be able to batch together multiple
// dwellings (if necessary) when doing that so that we don't have
// to iterate over the entire map multiple times to find each
// dwelling's land.
void delete_dwelling_ignoring_owned_land(
    SS& ss, IMapUpdater& map_updater, DwellingId dwelling_id ) {
  Coord const dwelling_coord =
      ss.natives.coord_for( dwelling_id );
  Tribe& tribe = ss.natives.tribe_for(
      ss.natives.tribe_type_for( dwelling_id ) );
  // Destroy free braves owned by this dwelling.
  //
  // We make a copy of this because it is probably not safe to
  // assume that we can iterate over the map reference (that this
  // function normally returns) while simultaneously deleting the
  // units it refers to.
  unordered_set<NativeUnitId> const braves =
      ss.units.braves_for_dwelling( dwelling_id );
  for( NativeUnitId unit_id : braves )
    ss.units.destroy_unit( unit_id );

  // Destroy any missionaries owned by this dwelling.
  if( maybe<UnitId> const missionary =
          ss.units.missionary_from_dwelling( dwelling_id );
      missionary.has_value() )
    UnitOwnershipChanger( ss, *missionary ).destroy();

  // Adjust the tribe's stockpile of muskets/horses.
  adjust_arms_on_dwelling_destruction( as_const( ss ), tribe );

  // Remove the dwelling objects.
  ss.natives.destroy_dwelling( dwelling_id );

  // Remove road under dwelling. There may not be any good reason
  // that we need to do this, but that's what the OG does.
  clear_road( map_updater, dwelling_coord );

  // This is actually not necessary since the tile will get re-
  // drawn anyway because the tile loses the road above, so the
  // map updater understands that the tile has to be redrawn.
  // However, even if there were not a road being lost, we would
  // still in general need to redraw the tile anyway since a
  // prime resource might get revealed. So, just out of principle
  // so that we're not relying on the presence of a road under
  // the dwelling to render a prime resource, we will force a re-
  // draw of the tile.
  map_updater.force_redraw_tiles( { dwelling_coord } );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void destroy_dwelling( SS& ss, IMapUpdater& map_updater,
                       DwellingId dwelling_id ) {
  ss.natives.mark_land_unowned_for_dwellings( { dwelling_id } );
  delete_dwelling_ignoring_owned_land( ss, map_updater,
                                       dwelling_id );
}

void destroy_tribe( SS& ss, IMapUpdater& map_updater,
                    e_tribe tribe ) {
  if( !ss.natives.tribe_exists( tribe ) ) return;
  unordered_set<DwellingId> const& dwellings =
      ss.natives.dwellings_for_tribe( tribe );
  // 1. Release all land owned by this tribe. Batch together all
  // dwellings in this call for efficiency.
  ss.natives.mark_land_unowned_for_dwellings( dwellings );

  // 2. Destroy the associated dwellings, braves, and missionar-
  // ies. Need to iterate over a copy because it is not safe to
  // iterate over this while destroying dwellings.
  auto dwellings_copy = dwellings;
  for( DwellingId const dwelling_id : dwellings_copy )
    delete_dwelling_ignoring_owned_land( ss, map_updater,
                                         dwelling_id );

  // 3. Destroy the tribe object.
  ss.natives.destroy_tribe_last_step( tribe );
}

wait<> tribe_wiped_out_message( TS& ts, e_tribe tribe ) {
  co_await ts.gui.message_box(
      "The [{}] tribe has been wiped out.",
      config_natives.tribes[tribe].name_singular );
}

wait<> destroy_tribe_interactive( SS& ss, TS& ts,
                                  e_tribe tribe ) {
  destroy_tribe( ss, ts.map_updater, tribe );
  co_await tribe_wiped_out_message( ts, tribe );
}

} // namespace rn
