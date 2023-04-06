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
** NoopNativeMind
*****************************************************************/
NativeUnitId NoopNativeMind::select_unit(
    std::set<NativeUnitId> const& units ) {
  CHECK( !units.empty() );
  return *units.begin();
}

// Implement INativeMind.
NativeUnitCommand NoopNativeMind::command_for( NativeUnitId ) {
  return NativeUnitCommand::forfeight{};
}

} // namespace rn
