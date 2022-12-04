/****************************************************************
**native-owned.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-13.
*
* Description: Handles things related to native-owned land.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "native-owned.rds.hpp"

// Revolution Now
#include "maybe.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/native-enums.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/enum-map.hpp"

namespace rn {

struct Player;
struct SSConst;

// This is the only function that should be used to determine
// whether a square is owned by the natives since it takes into
// account whether the player has Peter Minuit and whether the
// player has encountered the relevant tribe.
maybe<DwellingId> is_land_native_owned( SSConst const& ss,
                                        Player const&  player,
                                        Coord          coord );

// Same as above but returns the result that would be returned
// assuming that the player has already encountered the tribe.
maybe<DwellingId> is_land_native_owned_after_meeting(
    SSConst const& ss, Player const& player, Coord coord );

// Determines whether each of the eight squares around `loc` are
// native-owned and, if so, which dwellings own them.
refl::enum_map<e_direction, maybe<DwellingId>>
native_owned_land_around_square( SSConst const& ss,
                                 Player const&  player,
                                 Coord          loc );

// If the square is owned by a native tribe the it will return
// the tribe and price. The player must have met the tribe in
// order to call this, otherwise there is no price. If the player
// has not met the tribe then nothing is returned.
maybe<LandPrice> price_for_native_owned_land(
    SSConst const& ss, Player const& player, Coord coord );

} // namespace rn
