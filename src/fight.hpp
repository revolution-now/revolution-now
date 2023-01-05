/****************************************************************
**fight.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-14.
*
* Description: Handles statistics for individual combats.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace rn {

struct IRand;
struct NativeUnit;
struct Unit;

struct FightStatistics {
  bool attacker_wins{};
};

FightStatistics fight_statistics( IRand&      rand,
                                  Unit const& attacker,
                                  Unit const& defender );

FightStatistics fight_statistics( IRand&            rand,
                                  Unit const&       attacker,
                                  NativeUnit const& defender );

} // namespace rn
