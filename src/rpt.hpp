/****************************************************************
**rpt.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-20.
*
* Description: Implementation of the Recruit/Purchase/Train
*              buttons in the harbor view.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "wait.hpp"

namespace rn {

struct IGui;
struct IRand;
struct Player;
struct SS;

wait<> click_recruit( SS& ss, IGui& gui, IRand& rand,
                      Player& player );

wait<> click_purchase( SS& ss, IGui& gui, Player& player );

wait<> click_train( SS& ss, IGui& gui, Player& player );

} // namespace rn
