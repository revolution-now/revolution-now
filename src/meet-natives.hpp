/****************************************************************
**meet-natives.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-11-09.
*
* Description: Handles the sequence of events that happen when
*              first encountering a native tribe.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "meet-natives.rds.hpp"

// Revolution Now
#include "wait.hpp"

// ss
#include "ss/native-enums.rds.hpp"

// gfx
#include "gfx/coord.hpp"

// C++ standard library
#include <vector>

namespace rn {

enum class e_tribe;

struct IGui;
struct Player;
struct SS;
struct SSConst;

// Checks if there are any tribes in the immediate vicinity of
// the given square that the player has not yet met. The square
// is interpreted as the location of a hypothetical unit that
// would do the encountering. As such, the returned list will be
// empty if the square is a water square, since ships (and units
// on ships) cannot meet the natives for the first time unless
// they are on land.
std::vector<MeetTribe> check_meet_tribes( SSConst const& ss,
                                          Player const&  player,
                                          Coord square );

// Checks if there are any europeans in the immediate vicinity of
// the given square that the tribe has not yet met. The square is
// interpreted as the location of a hypothetical native unit that
// would do the encountering. As such, the returned list will not
// include europeans that are only adjacent via ships on water
// squares, since ships (and units on ships) cannot meet the na-
// tives for the first time unless they are on land.
std::vector<MeetTribe> check_meet_europeans(
    SSConst const& ss, e_tribe tribe_type, Coord native_square );

wait<e_declare_war_on_natives> perform_meet_tribe_ui_sequence(
    SS& ss, IGui& ts, MeetTribe const& meet_tribe );

// This will actually perform the actions (non-UI actions) that
// happen when the player meets a tribe for the first time.
// Namely, it will create the relationship state, change any land
// ownership, etc. It should be called after the UI sequence is
// finished.
void perform_meet_tribe( SS& ss, Player const& player,
                         MeetTribe const&         meet_tribe,
                         e_declare_war_on_natives declare_war );

} // namespace rn
