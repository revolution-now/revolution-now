/****************************************************************
**command-plow.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-27.
*
* Description: Carries out command to plow.
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
    command::plow const& plow );

} // namespace rn
