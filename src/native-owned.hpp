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
#include "wait.hpp"

// ss
#include "ss/dwelling-id.hpp"
#include "ss/native-enums.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// refl
#include "refl/enum-map.hpp"

// base
#include "base/vocab.hpp"

namespace rn {

struct Player;
struct SS;
struct SSConst;
struct TS;

// This is the only function that should be used to determine
// whether a square is owned by the natives since it takes into
// account whether the player has Peter Minuit and whether the
// player has encountered the relevant tribe, and also whether
// there is a friendly colony on the square.
maybe<DwellingId> is_land_native_owned( SSConst const& ss,
                                        Player const&  player,
                                        Coord          coord );

// Same as above but returns the result that would be returned
// assuming that the player has already encountered the tribe.
maybe<DwellingId> is_land_native_owned_after_meeting(
    SSConst const& ss, Player const& player, Coord coord );

// Same as above but returns the result that would be returned
// assuming that any colonies the player has were removed.
maybe<DwellingId>
is_land_native_owned_after_meeting_without_colonies(
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

// This will show the prompt that gets shown when the player per-
// forms an action that would occupy native-owned land. It will
// allow the player to cancel the action, pay the tribe (and
// hence get the land) or just take it. Returns true if the
// player has acquired the land in some way. The land must be
// owned by a tribe from the perspective of the player otherwise
// check-fail.
wait<base::NoDiscard<bool>> prompt_player_for_taking_native_land(
    SS& ss, TS& ts, Player& player, Coord tile,
    e_native_land_grab_type context );

} // namespace rn
