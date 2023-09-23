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
#include "road.hpp"
#include "ts.hpp"
#include "unit-mgr.hpp"
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
    SS& ss, TS& ts, DwellingId dwelling_id ) {
  // 1. Destroy free braves owned by this dwelling.
  //
  // We make a copy of this because it is probably not safe to
  // assume that we can iterate over the map reference (that this
  // function normally returns) while simultaneously deleting the
  // units it refers to.
  unordered_set<NativeUnitId> const braves =
      ss.units.braves_for_dwelling( dwelling_id );
  for( NativeUnitId unit_id : braves )
    ss.units.destroy_unit( unit_id );

  // 2. Destroy any missionaries owned by this dwelling.
  if( maybe<UnitId> const missionary =
          ss.units.missionary_from_dwelling( dwelling_id );
      missionary.has_value() )
    destroy_unit( ss, *missionary );

  // 3. Remove road under dwelling.
  clear_road( ts.map_updater,
              ss.natives.coord_for( dwelling_id ) );

  // 4. Remove the dwelling objects.
  ss.natives.destroy_dwelling( dwelling_id );
}

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void destroy_dwelling( SS& ss, TS& ts, DwellingId dwelling_id ) {
  ss.natives.mark_land_unowned_for_dwellings( { dwelling_id } );
  delete_dwelling_ignoring_owned_land( ss, ts, dwelling_id );
}

void destroy_tribe( SS& ss, TS& ts, e_tribe tribe ) {
  if( !ss.natives.tribe_exists( tribe ) ) return;
  UNWRAP_CHECK( dwellings,
                ss.natives.dwellings_for_tribe( tribe ) );
  // 1. Release all land owned by this tribe. Batch together all
  // dwellings in this call for efficiency.
  ss.natives.mark_land_unowned_for_dwellings( dwellings );

  // 2. Destroy the associated dwellings, braves, and missionar-
  // ies. Need to iterate over a copy because it is not safe to
  // iterate over this while destroying dwellings.
  auto dwellings_copy = dwellings;
  for( DwellingId const dwelling_id : dwellings_copy )
    delete_dwelling_ignoring_owned_land( ss, ts, dwelling_id );

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
  destroy_tribe( ss, ts, tribe );
  co_await tribe_wiped_out_message( ts, tribe );
}

void tribe_take_horses_from_destroyed_brave( Tribe& tribe ) {
  tribe.horses += 50;
}

void tribe_take_muskets_from_destroyed_brave( Tribe& tribe ) {
  tribe.muskets += 50;
}

} // namespace rn
