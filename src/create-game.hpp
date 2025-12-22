/****************************************************************
**create-game.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Creates UI sequences used to create-game the game.
*
*****************************************************************/
#pragma once

// rds
#include "create-game.rds.hpp"

// Revolution Now
#include "game-setup.rds.hpp"
#include "maybe.hpp"
#include "wait.hpp"

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct IGui;
struct IRand;
struct IEngine;
struct Planes;

/****************************************************************
** Public API.
*****************************************************************/
wait<maybe<GameSetup>> create_default_game_setup(
    IEngine& engine, Planes& planes, IGui& gui, IRand& rand,
    lua::state& lua );

wait<maybe<GameSetup>> create_america_game_setup(
    IEngine& engine, Planes& planes, IGui& gui, IRand& rand,
    lua::state& lua );

wait<maybe<GameSetup>> create_customized_game_setup(
    IEngine& engine, Planes& planes, IGui& gui,
    e_customization_mode mode );

} // namespace rn
