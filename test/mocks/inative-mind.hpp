/****************************************************************
**inative-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/icombat.rds.hpp"
#include "src/inative-mind.hpp"
#include "src/raid-effects.rds.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-std.hpp"

namespace rn {

/****************************************************************
** MockIRand
*****************************************************************/
struct MockINativeMind final : INativeMind {
  MockINativeMind( e_tribe tribe_type )
    : INativeMind( tribe_type ) {}

  MOCK_METHOD( NativeUnitId, select_unit,
               (std::set<NativeUnitId> const&), () );

  MOCK_METHOD( wait<>, message_box, (std::string const&), () );

  MOCK_METHOD( NativeUnitCommand, command_for, ( NativeUnitId ),
               () );

  MOCK_METHOD( void, on_attack_colony_finished,
               (CombatBraveAttackColony const&,
                BraveAttackColonyEffect const&),
               () );

  MOCK_METHOD( void, on_attack_unit_finished,
               (CombatBraveAttackEuro const&), () );
};

} // namespace rn
