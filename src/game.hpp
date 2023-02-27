/****************************************************************
**game.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-11.
*
* Description: Overall game flow of an individual game.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Rds
#include "game.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IGui;
struct Planes;

wait<> run_game_with_mode( Planes&          planes,
                           StartMode const& mode );

} // namespace rn
