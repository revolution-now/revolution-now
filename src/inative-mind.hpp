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

// Revolution Now
#include "imind.hpp"

// ss
#include "ss/unit-id.hpp"

// C++ standard library
#include <set>

namespace rn {

struct BraveAttackColonyEffect;
struct CombatBraveAttackColony;
struct CombatBraveAttackEuro;

/****************************************************************
** INativeMind
*****************************************************************/
struct INativeMind : IMind {
  virtual ~INativeMind() override = default;

  // Select which unit is to receive orders next. The set should
  // be non-empty and contain only units that have some movement
  // points remaining. Default implementation just selects the
  // first one in the (ordered) set.
  virtual NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) = 0;

  // Give a command to a unit.
  virtual NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) = 0;

  // After a native attack on a colony this will notify of the
  // result. This can be used e.g. to adjust tribal alarm. De-
  // fault implementation does nothing.
  virtual void on_attack_colony_finished(
      CombatBraveAttackColony const& combat,
      BraveAttackColonyEffect const& side_effect ) = 0;

  // After a native attack on a european unit this will notify of
  // the result. This can be used e.g. to adjust tribal alarm.
  // Default implementation does nothing.
  virtual void on_attack_unit_finished(
      CombatBraveAttackEuro const& combat ) = 0;
};

/****************************************************************
** NoopNativeMind
*****************************************************************/
struct NoopNativeMind final : INativeMind {
  NoopNativeMind() = default;

  // Implement IMind.
  wait<> message_box( std::string const& msg ) override;

  // Implement INativeMind.
  NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) override;

  // Implement INativeMind.
  NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) override;

  // Implement INativeMind.
  void on_attack_colony_finished(
      CombatBraveAttackColony const&,
      BraveAttackColonyEffect const& ) override;

  // Implement INativeMind.
  void on_attack_unit_finished(
      CombatBraveAttackEuro const& ) override;
};

} // namespace rn
