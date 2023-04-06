/****************************************************************
**inative-mind.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Interface for asking for orders and behaviors for
*              native units.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// rds
#include "inative-mind.rds.hpp"

// ss
#include "ss/unit-id.hpp"

// C++ standard library
#include <set>

namespace rn {

/****************************************************************
** INativeMind
*****************************************************************/
struct INativeMind {
  virtual ~INativeMind() = default;

  // Select which unit is to receive orders next. The set should
  // be non-empty and contain only units that have some movement
  // points remaining.
  virtual NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) = 0;

  // Give a command to a unit.
  virtual NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) = 0;
};

/****************************************************************
** NoopNativeMind
*****************************************************************/
struct NoopNativeMind final : INativeMind {
  NoopNativeMind() = default;

  // Implement INativeMind.
  NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) override;

  // Implement INativeMind.
  NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) override;
};

} // namespace rn
