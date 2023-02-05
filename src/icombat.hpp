/****************************************************************
**icombat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-04.
*
* Description: Interface for resolving battles.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "icombat.rds.hpp"

namespace rn {

struct Colony;
struct Dwelling;
struct NativeUnit;
struct SSConst;
struct TS;
struct Unit;

/****************************************************************
** ICombat
*****************************************************************/
// There are potentially multiple random numbers generating in
// order to resolve a combat. This interface will allow us to
// mock those to make unit test easier on code that depends on
// being able to resolve combat.
struct ICombat {
  virtual ~ICombat() = default;

  virtual CombatEuroAttackEuro euro_attack_euro(
      Unit const& attacker, Unit const& defender ) = 0;

  virtual CombatShipAttackShip ship_attack_ship(
      Unit const& attacker, Unit const& defender ) = 0;

  virtual CombatEuroAttackUndefendedColony
  euro_attack_undefended_colony( Unit const&   attacker,
                                 Unit const&   defender,
                                 Colony const& colony ) = 0;

  virtual CombatEuroAttackBrave euro_attack_brave(
      Unit const& attacker, NativeUnit const& defender ) = 0;

  virtual CombatEuroAttackDwelling euro_attack_dwelling(
      Unit const& attacker, Dwelling const& dwelling ) = 0;
};

/****************************************************************
** TrappingCombat
*****************************************************************/
// This is a no-op combat implementation for when we don't actu-
// ally need to use it; it will check-fail if any of its methods
// are called.
struct TrappingCombat : public ICombat {
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
};

} // namespace rn
