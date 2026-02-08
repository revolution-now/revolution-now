/****************************************************************
**game-setup.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-12-20.
*
* Description: Creates UI sequences used to game-setup the game.
*
*****************************************************************/
#pragma once

// rds
#include "game-setup.rds.hpp"

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
GameSetup create_classic_game_setup(
    IRand& rand, ClassicGameSetupParamsEvaluated const& params );

GameSetup create_default_game_setup(
    IRand& rand, ClassicGameSetupParamsCommon const& common );

GameSetup create_classic_customized_game_setup(
    IRand& rand, ClassicGameSetupParams const& params );

GameSetup create_america_game_setup(
    IRand& rand, ClassicGameSetupParamsCommon const& common );

wait<maybe<ClassicGameSetupParamsCommon>>
create_classic_game_common_params( IEngine& engine,
                                   Planes& planes, IGui& gui );

wait<maybe<ClassicGameSetupParamsCustom>>
create_classic_game_custom_params( IEngine& engine,
                                   Planes& planes, IGui& gui );

wait<maybe<GameSetup>> create_customized_game_setup(
    IEngine& engine, Planes& planes, IGui& gui,
    e_customization_mode mode );

base::valid_or<std::string> validate_game_setup(
    GameSetup const& setup );

} // namespace rn
