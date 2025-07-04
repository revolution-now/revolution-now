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
      MeetTribe const& meet_tribe ) override;

  wait<> show_woodcut( e_woodcut woodcut ) override;

  wait<base::heap_value<CapturableCargoItems>>
  select_commodities_to_capture(
      UnitId src, UnitId dst,
      CapturableCargo const& items ) override;

  wait<> notify_captured_cargo(
      Player const& src_player, Player const& dst_player,
      Unit const& dst_unit, Commodity const& stolen ) override;

 public: // Signals.
  bool handle( signal::Foo const& foo ) override;

  wait<int> handle( signal::Bar const& foo ) override;

  void handle( signal::ColonySignalTransient const& ) override;

  wait<maybe<int>> handle(
      signal::ChooseImmigrant const& ) override;

  wait<> handle( signal::PanTile const& ) override;

 private:
  SS& ss_;
  IGui& gui_;
  Planes& planes_;
};

} // namespace rn
