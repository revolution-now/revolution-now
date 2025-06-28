/****************************************************************
**ieuro-agent.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Interface for asking for orders and behaviors for
*              european (non-ref) units.
*
*****************************************************************/
#include "ieuro-agent.hpp"

// Revolution Now
#include "capture-cargo.rds.hpp"
#include "co-wait.hpp"

// ss
#include "players.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IEuroMind
*****************************************************************/
IEuroMind::IEuroMind( e_player player_type )
  : player_type_( player_type ) {}

/****************************************************************
** NoopEuroMind
*****************************************************************/
NoopEuroMind::NoopEuroMind( SSConst const& ss, e_player player )
  : IEuroMind( player ), ss_( ss ) {}

wait<> NoopEuroMind::message_box( string const& ) { co_return; }

wait<e_declare_war_on_natives>
NoopEuroMind::meet_tribe_ui_sequence( MeetTribe const& ) {
  co_return e_declare_war_on_natives::no;
}

wait<> NoopEuroMind::show_woodcut( e_woodcut ) { co_return; }

wait<base::heap_value<CapturableCargoItems>>
NoopEuroMind::select_commodities_to_capture(
    UnitId const, UnitId const, CapturableCargo const& ) {
  co_return {};
}

wait<> NoopEuroMind::notify_captured_cargo( Player const&,
                                            Player const&,
                                            Unit const&,
                                            Commodity const& ) {
  co_return;
}

Player const& NoopEuroMind::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

} // namespace rn
