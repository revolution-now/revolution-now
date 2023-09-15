/****************************************************************
**inative-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Interface for asking for orders and behaviors for
*              native units.
*
*****************************************************************/
#include "inative-mind.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** NoopNativeMind
*****************************************************************/
wait<> NoopNativeMind::message_box( string const& ) {
  co_return;
}

NativeUnitCommand NoopNativeMind::command_for( NativeUnitId ) {
  return NativeUnitCommand::forfeight{};
}

void NoopNativeMind::on_attack_colony_finished(
    CombatBraveAttackColony const&,
    BraveAttackColonyEffect const& ) {}

void NoopNativeMind::on_attack_unit_finished(
    CombatBraveAttackEuro const& ) {}

NativeUnitId NoopNativeMind::select_unit(
    set<NativeUnitId> const& units ) {
  CHECK( !units.empty() );
  return *units.begin();
}

} // namespace rn
