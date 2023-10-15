/****************************************************************
**combat.hpp
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
#include "icombat.hpp"

// ss
#include "ss/ref.hpp"

namespace rn {

struct Colony;
struct Dwelling;
struct IRand;
struct NativeUnit;
struct SSConst;
struct Unit;

/****************************************************************
** RealCombat
*****************************************************************/
struct RealCombat : public ICombat {
  RealCombat( SSConst const& ss, IRand& rand )
    : ss_( ss ), rand_( rand ) {}

  // Implement ICombat.
  CombatEuroAttackEuro euro_attack_euro(
      Unit const& attacker, Unit const& defender ) override;

  // Implement ICombat.
  CombatShipAttackShip ship_attack_ship(
      Unit const& attacker, Unit const& defender ) override;

  // Implement ICombat.
  CombatEuroAttackUndefendedColony euro_attack_undefended_colony(
      Unit const& attacker, Unit const& defender,
      Colony const& colony ) override;

  // Implement ICombat.
  CombatEuroAttackBrave euro_attack_brave(
      Unit const&       attacker,
      NativeUnit const& defender ) override;

  // Implement ICombat.
  CombatEuroAttackDwelling euro_attack_dwelling(
      Unit const& attacker, Dwelling const& dwelling ) override;

  // Implement ICombat.
  CombatBraveAttackEuro brave_attack_euro(
      NativeUnit const& attacker,
      Unit const&       defender ) override;

  // Implement ICombat.
  CombatBraveAttackColony brave_attack_colony(
      NativeUnit const& attacker, Unit const& defender,
      Colony const& colony ) override;

 private:
  SSConst const& ss_;
  IRand&         rand_;
};

} // namespace rn
