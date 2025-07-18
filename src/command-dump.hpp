/****************************************************************
**command-dump.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-09.
*
* Description: Carries out commands to dump cargo overboard from
*              a ship or wagon train.
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
    Player& player, UnitId id, command::dump const& dump );

} // namespace rn
