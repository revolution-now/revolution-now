/****************************************************************
**icombat.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-02-04.
*
* Description: For dependency injection in unit test.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/icombat.hpp"

// ss
#include "src/ss/colony.rds.hpp"
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/unit.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

namespace rn {

/****************************************************************
** MockICombat
*****************************************************************/
struct MockICombat : ICombat {
  MOCK_METHOD( CombatEuroAttackEuro, euro_attack_euro,
               (Unit const&, Unit const&), () );
  MOCK_METHOD( CombatShipAttackShip, ship_attack_ship,
               (Unit const&, Unit const&), () );
  MOCK_METHOD( CombatEuroAttackUndefendedColony,
               euro_attack_undefended_colony,
               (Unit const&, Unit const&, Colony const&), () );
  MOCK_METHOD( CombatEuroAttackBrave, euro_attack_brave,
               (Unit const&, NativeUnit const&), () );
  MOCK_METHOD( CombatEuroAttackDwelling, euro_attack_dwelling,
               (Unit const&, Dwelling const&), () );
};

static_assert( !std::is_abstract_v<MockICombat> );

} // namespace rn
