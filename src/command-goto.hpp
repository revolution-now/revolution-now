/****************************************************************
**command-goto.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-09-21.
*
* Description: Carries out the command to go to a location.
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
    Player& player, UnitId id, command::go_to const& go_to );

} // namespace rn
