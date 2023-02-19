/****************************************************************
**command-move.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-04-16.
*
* Description: Carries out commands wherein a unit is asked to
*              move onto an adjacent square.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "command.hpp"

namespace rn {

struct Player;
struct SS;
struct TS;

std::unique_ptr<CommandHandler> handle_command(
    SS& ss, TS& ts, Player& player, UnitId id,
    command::move const& mv );

} // namespace rn
