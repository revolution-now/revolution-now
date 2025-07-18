/****************************************************************
**command-road.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-25.
*
* Description: Carries out the command to build a road.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "command.hpp"

namespace rn {

struct IEngine;
struct IEuroAgent;
struct Player;
struct SS;
struct TS;

std::unique_ptr<CommandHandler> handle_command(
    IEngine& engine, SS& ss, TS& ts, IEuroAgent& agent,
    Player& player, UnitId id, command::road const& road );

} // namespace rn
