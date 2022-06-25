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

// Revolution Now
#include "unit-id.hpp"

// C++ standard library
#include <utility>

namespace rn {

struct FightStatistics {
  bool attacker_wins{};
};
NOTHROW_MOVE( FightStatistics );

FightStatistics fight_statistics( UnitId attacker_id,
                                  UnitId defender_id );

} // namespace rn
