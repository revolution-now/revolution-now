/****************************************************************
**command-fortify.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out commands to fortify or sentry a unit.
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
    Player& player, UnitId id, command::fortify const& fortify );

std::unique_ptr<CommandHandler> handle_command(
    IEngine& engine, SS& ss, TS& ts, IEuroAgent& agent,
    Player& player, UnitId id, command::sentry const& sentry );

} // namespace rn
