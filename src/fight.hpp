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

struct Unit;

struct FightStatistics {
  bool attacker_wins{};
};

FightStatistics fight_statistics( Unit const& attacker,
                                  Unit const& defender );

} // namespace rn
