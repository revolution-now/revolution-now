/****************************************************************
**human-agent.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IAgent for human players.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "goto-registry.hpp"
#include "iagent.hpp"

namespace rn {

struct IEngine;
struct IGotoMapViewer;
struct IGui;
struct ILandViewPlane;
struct Planes;
struct SS;

/****************************************************************
** HumanAgent
*****************************************************************/
// This is an implementation that will consult with a human user
// via GUI actions or input in order to fulfill the requests.
struct HumanAgent final : IAgent {
  HumanAgent( e_player player, IEngine& engine, SS& ss,
              IGui& gui, Planes& planes );

 public: // IAgent.
  wait<> message_box( std::string const& msg ) override;

  Player const& player() override;

  void dump_last_message() const override;

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

  wait<maybe<int>> pick_dump_cargo(
      std::map<int /*slot*/, Commodity> const& options )
      override;

  wait<e_native_land_grab_result> should_take_native_land(
      std::string const& msg,
      refl::enum_map<e_native_land_grab_result,
                     std::string> const& names,
      refl::enum_map<e_native_land_grab_result, bool> const&
          disabled ) override;

  wait<ui::e_confirm> confirm_disband_unit(
      UnitId unit_id ) override;

  wait<ui::e_confirm> confirm_build_inland_colony() override;

  wait<maybe<std::string>> name_colony() override;

  wait<ui::e_confirm> should_make_landfall(
      bool some_units_already_moved ) override;

  wait<ui::e_confirm> should_sail_high_seas() override;

  EvolveGoto evolve_goto( UnitId unit_id ) override;

 public: // Signals.
  OVERRIDE_SIGNAL( ChooseImmigrant );
  OVERRIDE_SIGNAL( ColonyDestroyedByNatives );
  OVERRIDE_SIGNAL( ColonyDestroyedByStarvation );
  OVERRIDE_SIGNAL( ColonySignal );
  OVERRIDE_SIGNAL( ColonySignalTransient );
  OVERRIDE_SIGNAL( ForestClearedNearColony );
  OVERRIDE_SIGNAL( ImmigrantArrived );
  OVERRIDE_SIGNAL( NoSpotForShip );
  OVERRIDE_SIGNAL( PioneerExhaustedTools );
  OVERRIDE_SIGNAL( PriceChange );
  OVERRIDE_SIGNAL( RebelSentimentChanged );
  OVERRIDE_SIGNAL( RefUnitAdded );
  OVERRIDE_SIGNAL( ShipFinishedRepairs );
  OVERRIDE_SIGNAL( TaxRateWillChange );
  OVERRIDE_SIGNAL( TeaParty );
  OVERRIDE_SIGNAL( TreasureArrived );
  OVERRIDE_SIGNAL( TribeWipedOut );

 private:
  ILandViewPlane& land_view() const;

  void new_goto( IGotoMapViewer const& viewer, UnitId unit_id,
                 goto_target const& target );

  IEngine& engine_;
  SS& ss_;
  IGui& gui_;
  Planes& planes_;

  GotoRegistry goto_registry_;
};

} // namespace rn
