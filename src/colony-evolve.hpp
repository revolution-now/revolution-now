/****************************************************************
**colony-evolve.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-04.
*
* Description: Evolves one colony one turn.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "colony-evolve.rds.hpp"

namespace rn {

struct Colony;
struct Player;
struct SS;
struct TS;

/****************************************************************
** IColonyEvolver
*****************************************************************/
// Evolve the colony by one turn. This is not a coroutine for a
// few reasons: 1) ease of testability, 2) we want it to also be
// used for the AI players, 3) we want to be able to have a way
// to evolve a colony (e.g. for cheat mode) where we can control
// what is shown to the user.
ColonyEvolution evolve_colony_one_turn( SS& ss, TS& ts,
                                        Colony& colony );

// This generates the text messages that are actually shown to
// the player.
ColonyNotificationMessage generate_colony_notification_message(
    Colony const&             colony,
    ColonyNotification const& notification );

} // namespace rn
