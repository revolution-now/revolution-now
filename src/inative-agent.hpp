/****************************************************************
**inative-agent.hpp
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
#include "inative-agent.rds.hpp"

// Revolution Now
#include "iagent.hpp"

// ss
#include "ss/unit-id.hpp"

// C++ standard library
#include <set>

namespace rn {

struct BraveAttackColonyEffect;
struct CombatBraveAttackColony;
struct CombatBraveAttackEuro;

enum class e_tribe;

/****************************************************************
** INativeAgent
*****************************************************************/
struct INativeAgent : IAgent {
  INativeAgent( e_tribe tribe_type )
    : tribe_type_( tribe_type ) {}
  virtual ~INativeAgent() override = default;

  e_tribe tribe_type() const { return tribe_type_; }

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

 private:
  e_tribe tribe_type_ = {};
};

/****************************************************************
** NoopNativeAgent
*****************************************************************/
struct NoopNativeAgent final : INativeAgent {
  NoopNativeAgent( e_tribe tribe_type )
    : INativeAgent( tribe_type ) {}

 public: // IAgent.
  wait<> message_box( std::string const& msg ) override;

 public: // INativeAgent.
  NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) override;

  NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) override;

  void on_attack_colony_finished(
      CombatBraveAttackColony const&,
      BraveAttackColonyEffect const& ) override;

  void on_attack_unit_finished(
      CombatBraveAttackEuro const& ) override;
};

} // namespace rn
