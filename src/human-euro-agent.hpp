/****************************************************************
**human-euro-agent.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IEuroAgent for human players.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "ieuro-agent.hpp"

namespace rn {

struct IGui;
struct ILandViewPlane;
struct Planes;
struct SS;

/****************************************************************
** HumanEuroAgent
*****************************************************************/
// This is an implementation that will consult with a human user
// via GUI actions or input in order to fulfill the requests.
struct HumanEuroAgent final : IEuroAgent {
  HumanEuroAgent( e_player player, SS& ss, IGui& gui,
                  Planes& planes );

 public: // IAgent.
  wait<> message_box( std::string const& msg ) override;

 public: // IEuroAgent.
  Player const& player() override;

  bool human() const override { return true; }

  wait<e_declare_war_on_natives> meet_tribe_ui_sequence(
      MeetTribe const& meet_tribe, gfx::point tile ) override;

  wait<> show_woodcut( e_woodcut woodcut ) override;

  wait<base::heap_value<CapturableCargoItems>>
  select_commodities_to_capture(
      UnitId src, UnitId dst,
      CapturableCargo const& items ) override;

  wait<> notify_captured_cargo(
      Player const& src_player, Player const& dst_player,
      Unit const& dst_unit, Commodity const& stolen ) override;

  wait<std::string> name_new_world() override;

  wait<ui::e_confirm> should_king_transport_treasure(
      std::string const& msg ) override;

  wait<ui::e_confirm> should_explore_ancient_burial_mounds()
      override;

  wait<std::chrono::microseconds> wait_for(
      std::chrono::milliseconds us ) override;

  wait<> pan_tile( gfx::point tile ) override;

  wait<> pan_unit( UnitId unit_id ) override;

  command ask_orders( UnitId unit_id ) override;

  wait<ui::e_confirm> kiss_pinky_ring(
      std::string const& msg, ColonyId colony_id,
      e_commodity type, int tax_increase ) override;

  wait<ui::e_confirm> attack_with_partial_movement_points(
      UnitId unit_id ) override;

  wait<ui::e_confirm> should_attack_natives(
      e_tribe tribe ) override;

 public: // Signals.
  void handle( signal::ColonySignalTransient const& ) override;

  wait<maybe<int>> handle(
      signal::ChooseImmigrant const& ) override;

 private:
  ILandViewPlane& land_view() const;

  SS& ss_;
  IGui& gui_;
  Planes& planes_;
};

} // namespace rn
