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
#include "base/to-str-ext-std.hpp"

#define MOCK_HANDLER( ret, sig ) \
  MOCK_METHOD( ret, handle, (signal::sig const&), () )

namespace rn {

/****************************************************************
** MockIEuroAgent
*****************************************************************/
struct MockIEuroAgent : IEuroAgent {
  MockIEuroAgent( e_player player ) : IEuroAgent( player ) {}

  MOCK_METHOD( Player const&, player, (), () );

  MOCK_METHOD( wait<>, message_box, (std::string const&), () );

  MOCK_METHOD( wait<e_declare_war_on_natives>,
               meet_tribe_ui_sequence, (MeetTribe const&), () );

  MOCK_METHOD( wait<>, show_woodcut, ( e_woodcut ), () );

  MOCK_METHOD( wait<base::heap_value<CapturableCargoItems>>,
               select_commodities_to_capture,
               (UnitId, UnitId, CapturableCargo const&), () );

  MOCK_METHOD( wait<>, notify_captured_cargo,
               (Player const&, Player const&, Unit const&,
                Commodity const&),
               () );

 public: // Signals
  MOCK_HANDLER( bool, Foo );
  MOCK_HANDLER( wait<int>, Bar );
};

static_assert( !std::is_abstract_v<MockIEuroAgent> );

} // namespace rn
