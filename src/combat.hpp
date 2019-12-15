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
#include "analysis.hpp"
#include "fight.hpp"
#include "macros.hpp"
#include "sync-future.hpp"
#include "unit.hpp"

// C++ standard library
#include <variant>
#include <vector>

namespace rn {

enum class ND e_attack_good {
  eu_land_unit,
  ship
  /*native_unit,
  colony,
  village,
  on_privateer,
  with_privateer*/
};

enum class ND e_attack_error {
  unit_cannot_attack,
  land_unit_attack_ship,
  ship_attack_land_unit,
  attack_from_ship
};

using unit_combat_verdict =
    std::variant<e_attack_good, e_attack_error>;
NOTHROW_MOVE( unit_combat_verdict );

struct CombatAnalysis : public OrdersAnalysis<CombatAnalysis> {
  CombatAnalysis( UnitId id_, orders_t orders_,
                  Vec<UnitId> units_to_prioritize_,
                  Coord attack_src_, Coord attack_target_,
                  unit_combat_verdict  desc_,
                  Opt<UnitId>          target_unit_,
                  Opt<FightStatistics> fight_stats_ )
    : parent_t( id_, orders_,
                std::move( units_to_prioritize_ ) ),
      attack_src( attack_src_ ),
      attack_target( attack_target_ ),
      desc( desc_ ),
      target_unit( target_unit_ ),
      fight_stats( fight_stats_ ) {}

  // ------------------------ Data -----------------------------

  // The square on which the unit resides.
  Coord attack_src{};

  // The square toward which the attack is aimed; this is the
  // same as the square of the unit being attacked.
  Coord attack_target{};

  // Description of what would happen if the move were carried
  // out. This can also serve as a binary indicator of whether
  // the move is possible by checking the type held, as the can_-
  // move() function does as a convenience.
  unit_combat_verdict desc{};

  // Unit being attacked.
  Opt<UnitId> target_unit{};

  // If the fight is allowed then this will hold the numerical
  // breakdown of the statistics contributing to the final proba-
  // bilities.
  Opt<FightStatistics> fight_stats{};

  // ---------------- "Virtual" Methods ------------------------

  bool              allowed_() const;
  sync_future<bool> confirm_explain_() const;
  void              affect_orders_() const;

  static Opt<CombatAnalysis> analyze_( UnitId   id,
                                       orders_t orders );
};
NOTHROW_MOVE( CombatAnalysis );

} // namespace rn
