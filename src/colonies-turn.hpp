/****************************************************************
**colonies-turn.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-04-02.
*
* Description: Top-level colony evolution logic run each turn.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IColonyEvolver;
struct IColonyNotificationGenerator;
struct IHarborViewer;
struct Player;
struct SS;
struct TS;

// Evolve the player's colonies by one turn.
wait<> evolve_colonies_for_player(
    SS& ss, TS& ts, Player& player,
    IColonyEvolver const& colony_evolver,
    IHarborViewer& harbor_viewer,
    IColonyNotificationGenerator const&
        colony_notification_generator );

} // namespace rn
