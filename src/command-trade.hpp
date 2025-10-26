/****************************************************************
**command-trade.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-26.
*
* Description: Carries out the command to start a trade route.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "command.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IEngine;
struct IAgent;
struct Player;
struct SS;
struct TS;

/****************************************************************
** Command.
*****************************************************************/
std::unique_ptr<CommandHandler> handle_command(
    IEngine& engine, SS& ss, TS& ts, IAgent& agent,
    Player& player, UnitId id,
    command::trade_route const& trade_route );

} // namespace rn
