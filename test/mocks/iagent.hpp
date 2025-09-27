/****************************************************************
**iagent.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: For dependency injection in unit tests.
*
*****************************************************************/
#pragma once

// Revolution Now
#include "src/anim-builder.rds.hpp"
#include "src/capture-cargo.rds.hpp"
#include "src/iagent.hpp"

// ss
#include "src/ss/player.rds.hpp"
#include "src/ss/unit.hpp"

// mock
#include "src/mock/mock.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/to-str-ext-chrono.hpp"
#include "base/to-str-ext-std.hpp"

#define MOCK_SIGNAL_HANDLER( sig )           \
  MOCK_METHOD( SIGNAL_RESULT( sig ), handle, \
               (signal::sig const&), () )

namespace rn {

/****************************************************************
** MockIAgent
*****************************************************************/
struct MockIAgent : IAgent {
  MockIAgent( e_player player ) : IAgent( player ) {}

 public:
  MOCK_METHOD( Player const&, player, (), () );
  MOCK_METHOD( bool, human, (), ( const ) );
  MOCK_METHOD( void, dump_last_message, (), ( const ) );
  MOCK_METHOD( wait<>, message_box, (std::string const&), () );
  MOCK_METHOD( wait<e_declare_war_on_natives>,
               meet_tribe_ui_sequence,
               ( MeetTribe const&, gfx::point ), () );
  MOCK_METHOD( wait<>, show_woodcut, ( e_woodcut ), () );
  MOCK_METHOD( wait<base::heap_value<CapturableCargoItems>>,
               select_commodities_to_capture,
               (UnitId, UnitId, CapturableCargo const&), () );
  MOCK_METHOD( wait<>, notify_captured_cargo,
               (Player const&, Player const&, Unit const&,
                Commodity const&),
               () );
  MOCK_METHOD( wait<std::string>, name_new_world, (), () );
  MOCK_METHOD( wait<ui::e_confirm>,
               should_king_transport_treasure,
               (std::string const&), () );
  MOCK_METHOD( wait<ui::e_confirm>,
               should_explore_ancient_burial_mounds, (), () );
  MOCK_METHOD( wait<std::chrono::microseconds>, wait_for,
               ( std::chrono::milliseconds ), () );
  MOCK_METHOD( wait<>, pan_tile, ( gfx::point ), () );
  MOCK_METHOD( wait<>, pan_unit, ( UnitId ), () );
  MOCK_METHOD( command, ask_orders, ( UnitId ), () );
  MOCK_METHOD( wait<ui::e_confirm>, kiss_pinky_ring,
               (std::string const&, ColonyId, e_commodity, int),
               () );
  MOCK_METHOD( wait<ui::e_confirm>,
               attack_with_partial_movement_points, ( UnitId ),
               () );
  MOCK_METHOD( wait<ui::e_confirm>, should_attack_natives,
               ( e_tribe ), () );
  MOCK_METHOD( wait<ui::e_confirm>, confirm_disband_unit,
               ( UnitId ), () );
  MOCK_METHOD( wait<ui::e_confirm>, confirm_build_inland_colony,
               (), () );
  MOCK_METHOD( wait<maybe<std::string>>, name_colony, (), () );
  MOCK_METHOD( wait<ui::e_confirm>, should_make_landfall, (bool),
               () );
  MOCK_METHOD( wait<ui::e_confirm>, should_sail_high_seas, (),
               () );
  MOCK_METHOD( EvolveGoto, evolve_goto, ( UnitId ), () );

  using CommoditySlotMap = std::map<int, Commodity>;
  MOCK_METHOD( wait<maybe<int>>, pick_dump_cargo,
               (CommoditySlotMap const&), () );

  using LandGrabNamesMap =
      refl::enum_map<e_native_land_grab_result, std::string>;
  using LandGrabDisabledMap =
      refl::enum_map<e_native_land_grab_result, bool>;
  MOCK_METHOD( wait<e_native_land_grab_result>,
               should_take_native_land,
               (std::string const&, LandGrabNamesMap const&,
                LandGrabDisabledMap const&),
               () );

 public: // Signals
  MOCK_SIGNAL_HANDLER( ChooseImmigrant );
  MOCK_SIGNAL_HANDLER( ColonyDestroyedByNatives );
  MOCK_SIGNAL_HANDLER( ColonyDestroyedByStarvation );
  MOCK_SIGNAL_HANDLER( ColonySignal );
  MOCK_SIGNAL_HANDLER( ColonySignalTransient );
  MOCK_SIGNAL_HANDLER( ForestClearedNearColony );
  MOCK_SIGNAL_HANDLER( ImmigrantArrived );
  MOCK_SIGNAL_HANDLER( NoSpotForShip );
  MOCK_SIGNAL_HANDLER( PioneerExhaustedTools );
  MOCK_SIGNAL_HANDLER( PriceChange );
  MOCK_SIGNAL_HANDLER( RebelSentimentChanged );
  MOCK_SIGNAL_HANDLER( RefUnitAdded );
  MOCK_SIGNAL_HANDLER( ShipFinishedRepairs );
  MOCK_SIGNAL_HANDLER( TaxRateWillChange );
  MOCK_SIGNAL_HANDLER( TeaParty );
  MOCK_SIGNAL_HANDLER( TreasureArrived );
  MOCK_SIGNAL_HANDLER( TribeWipedOut );
};

static_assert( !std::is_abstract_v<MockIAgent> );

} // namespace rn
