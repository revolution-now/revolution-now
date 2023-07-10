/****************************************************************
**raid.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-04-17.
*
* Description: Handles when braves raid a colony.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "raid.rds.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct Colony;
struct IEuroMind;
struct IRand;
struct SS;
struct SSConst;
struct TS;

/****************************************************************
** Public API
*****************************************************************/
BraveAttackColonyEffect select_brave_attack_colony_effect(
    SSConst const& ss, IRand& rand, Colony const& colony );

void perform_brave_attack_colony_effect(
    SS& ss, TS& ts, Colony& colony,
    BraveAttackColonyEffect const& effect );

wait<> display_brave_attack_colony_effect_msg(
    SSConst const& ss, IEuroMind& mind, Colony const& colony,
    BraveAttackColonyEffect const& effect );

} // namespace rn
