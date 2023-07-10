/****************************************************************
**icombat.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-04.
*
* Description: Interface for resolving battles.
*
*****************************************************************/
#include "icombat.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** TrappingCombat
*****************************************************************/
CombatEuroAttackEuro TrappingCombat::euro_attack_euro(
    Unit const&, Unit const& ) {
  SHOULD_NOT_BE_HERE;
}

CombatShipAttackShip TrappingCombat::ship_attack_ship(
    Unit const&, Unit const& ) {
  SHOULD_NOT_BE_HERE;
}

CombatEuroAttackUndefendedColony
TrappingCombat::euro_attack_undefended_colony( Unit const&,
                                               Unit const&,
                                               Colony const& ) {
  SHOULD_NOT_BE_HERE;
}

CombatEuroAttackBrave TrappingCombat::euro_attack_brave(
    Unit const&, NativeUnit const& ) {
  SHOULD_NOT_BE_HERE;
}

CombatEuroAttackDwelling TrappingCombat::euro_attack_dwelling(
    Unit const&, Dwelling const& ) {
  SHOULD_NOT_BE_HERE;
}

CombatBraveAttackEuro TrappingCombat::brave_attack_euro(
    NativeUnit const&, Unit const& ) {
  SHOULD_NOT_BE_HERE;
}

CombatBraveAttackColony TrappingCombat::brave_attack_colony(
    NativeUnit const&, Unit const&, Colony const& ) {
  SHOULD_NOT_BE_HERE;
}

} // namespace rn
