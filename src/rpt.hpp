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

struct Player;
struct SS;
struct TS;

wait<> click_recruit( SS& ss, TS& ts, Player& player );

wait<> click_purchase( SS& ss, TS& ts, Player& player );

wait<> click_train( SS& ss, TS& ts, Player& player );

} // namespace rn
