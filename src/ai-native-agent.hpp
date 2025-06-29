/****************************************************************
**ai-native-agent.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: AI for natives.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "inative-agent.hpp"

namespace rn {

struct IRand;
struct SS;

/****************************************************************
** AiNativeAgent
*****************************************************************/
struct AiNativeAgent final : INativeAgent {
  // We don't take TS here because would create a circular depen-
  // dency.
  AiNativeAgent( SS& ss, IRand& rand, e_tribe tribe_type );

 public: // IAgent.
  wait<> message_box( std::string const& msg ) override;

 public: // INativeAgent.
  NativeUnitId select_unit(
      std::set<NativeUnitId> const& units ) override;

  NativeUnitCommand command_for(
      NativeUnitId native_unit_id ) override;

  void on_attack_colony_finished(
      CombatBraveAttackColony const& combat,
      BraveAttackColonyEffect const& side_effect ) override;

  void on_attack_unit_finished(
      CombatBraveAttackEuro const& ) override;

 private:
  SS& ss_; // can this be SSConst?
  IRand& rand_;
};

} // namespace rn
