/****************************************************************
**native-owned.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-13.
*
* Description: Handles things related to native-owned land.
*
*****************************************************************/
#include "native-owned.hpp"

// ss
#include "ss/natives.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"

// base
#include "base/keyval.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** Public API
*****************************************************************/
maybe<DwellingId> is_land_native_owned_after_meeting(
    SSConst const& ss, Player const& player, Coord coord ) {
  if( player.fathers.has[e_founding_father::peter_minuit] )
    // Effectively no native land ownership when we have Minuit.
    return nothing;
  return base::lookup( ss.natives.owned_land_without_minuit(),
                       coord );
}

maybe<DwellingId> is_land_native_owned( SSConst const& ss,
                                        Player const&  player,
                                        Coord          coord ) {
  maybe<DwellingId> const dwelling_id =
      is_land_native_owned_after_meeting( ss, player, coord );
  if( !dwelling_id.has_value() ) return nothing;
  Dwelling const& dwelling =
      ss.natives.dwelling_for( *dwelling_id );
  Tribe const& tribe = ss.natives.tribe_for( dwelling.tribe );
  if( !tribe.relationship[player.nation].has_value() )
    return nothing;
  return *dwelling_id;
}

refl::enum_map<e_direction, maybe<DwellingId>>
native_owned_land_around_square( SSConst const& ss,
                                 Player const&  player,
                                 Coord          loc ) {
  refl::enum_map<e_direction, maybe<DwellingId>> res;
  for( e_direction d : refl::enum_values<e_direction> )
    res[d] = is_land_native_owned( ss, player, loc.moved( d ) );
  return res;
}

} // namespace rn
