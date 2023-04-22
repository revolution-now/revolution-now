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

using namespace std;

namespace rn {

/****************************************************************
** INativeMind
*****************************************************************/
NativeUnitId INativeMind::select_unit(
    set<NativeUnitId> const& units ) {
  CHECK( !units.empty() );
  return *units.begin();
}

/****************************************************************
** NoopNativeMind
*****************************************************************/
// Implement INativeMind.
NativeUnitCommand NoopNativeMind::command_for( NativeUnitId ) {
  return NativeUnitCommand::forfeight{};
}

} // namespace rn
