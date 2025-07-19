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

// rds
#include "inative-agent.rds.hpp"

// Revolution Now
#include "wait.hpp"

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
struct INativeAgent {
  INativeAgent( e_tribe tribe_type )
    : tribe_type_( tribe_type ) {}
  virtual ~INativeAgent() = default;

  e_tribe tribe_type() const { return tribe_type_; }

  virtual wait<> message_box( std::string const& msg ) = 0;

  // For convenience.  Should not be overridden.
  template<typename Arg, typename... Rest>
  wait<> message_box(
      // The type_identity prevents the compiler from using the
      // first arg to try to infer Arg/Rest (which would fail);
      // it will defer that, then when it gets to the end it will
      // have inferred those parameters through other args.
      fmt::format_string<std::type_identity_t<Arg>, Rest...> fmt,
      Arg&& arg, Rest&&... rest ) {
    return message_box(
        fmt::format( fmt, std::forward<Arg>( arg ),
                     std::forward<Rest>( rest )... ) );
  }

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
