/****************************************************************
**player.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-29.
*
* Description: Data structure representing a human or AI player.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "nation.hpp"

// base-util
#include "base-util/non-copyable.hpp"

namespace rn {

class Player : public util::movable_only {
public:
  Player( NationDesc const& nation, bool is_human, int money );
  NationDesc const& nation() const { return nation_; }
  int               money() const { return money_; }
  bool              is_human() const { return human_; }

private:
  NationDesc const& nation_;
  bool              human_{true};
  int               money_{0};
};

Player& player_for_nation( e_nation nation );

} // namespace rn
