/****************************************************************
**inative-agent.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Interface for asking for orders and behaviors for
*              native units.
*
*****************************************************************/
#include "inative-agent.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** NoopNativeAgent
*****************************************************************/
NativeUnitCommand NoopNativeAgent::command_for( NativeUnitId ) {
  return NativeUnitCommand::forfeight{};
}

void NoopNativeAgent::on_attack_colony_finished(
    CombatBraveAttackColony const&,
    BraveAttackColonyEffect const& ) {}

void NoopNativeAgent::on_attack_unit_finished(
    CombatBraveAttackEuro const& ) {}

NativeUnitId NoopNativeAgent::select_unit(
    set<NativeUnitId> const& units ) {
  CHECK( !units.empty() );
  return *units.begin();
}

} // namespace rn
