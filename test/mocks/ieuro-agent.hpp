/****************************************************************
**ieuro-agent.hpp
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
#include "src/ieuro-agent.hpp"

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

#define MOCK_SIGNAL_HANDLER( ret, sig ) \
  MOCK_METHOD( ret, handle, (signal::sig const&), () )

namespace rn {

/****************************************************************
** MockIEuroAgent
*****************************************************************/
struct MockIEuroAgent : IEuroAgent {
  MockIEuroAgent( e_player player ) : IEuroAgent( player ) {}

 public:
  MOCK_METHOD( Player const&, player, (), () );
  MOCK_METHOD( bool, human, (), ( const ) );
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

 public: // Signals
  MOCK_SIGNAL_HANDLER( wait<maybe<int>>, ChooseImmigrant );
  MOCK_SIGNAL_HANDLER( void, ColonySignalTransient );
};

static_assert( !std::is_abstract_v<MockIEuroAgent> );

} // namespace rn
