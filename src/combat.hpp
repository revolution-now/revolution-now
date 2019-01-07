/****************************************************************
**combat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-06.
*
* Description: Handles combat statistics.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "unit.hpp"

// C++ standard library
#include <variant>
#include <vector>

namespace rn {

enum class ND e_attack_good {
  eu_unit,
  native_unit,
  colony,
  village,
  ship,
  on_privateer,
  with_privateer
};

enum class ND e_attack_error {
  unit_cannot_attack,
  land_unit_attack_ship,
  ship_attack_land_unit
};

using v_unit_attack_desc =
    std::variant<e_attack_good, e_attack_error>;

struct ProposedCombatAnalysisResult {
  v_unit_attack_desc desc;
  bool               attacker_wins;
  // Units that will be waiting for orders and which should be
  // prioritized in the "orders" loop after this move is made.
  // This field is only relevant for certain (valid) moves. NOTE:
  // units will be prioritized in reverse order of this vector,
  // i.e., the last unit will be up first.
  std::vector<UnitId> to_prioritize{};
  bool                allowed() const;
};

ND ProposedCombatAnalysisResult
   analyze_proposed_attack( UnitId id, e_direction d );

// Checks that the orders are possible (if not, returns false)
// and, if so, will check the type of orders and determine
// whether the player needs to be asked for any kind of
// confirmation. In addition, if the orders are not allowed, the
// player may be given an explantation as to why.
bool confirm_explain_attack(
    ProposedCombatAnalysisResult const& analysis );

void run_combat( ProposedCombatAnalysisResult const& analysis );

} // namespace rn
