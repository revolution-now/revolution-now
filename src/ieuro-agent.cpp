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
** IEuroAgent
*****************************************************************/
IEuroAgent::IEuroAgent( e_player player_type )
  : player_type_( player_type ) {}

/****************************************************************
** NoopEuroAgent
*****************************************************************/
NoopEuroAgent::NoopEuroAgent( SSConst const& ss,
                              e_player player )
  : IEuroAgent( player ), ss_( ss ) {}

wait<> NoopEuroAgent::message_box( string const& ) { co_return; }

wait<e_declare_war_on_natives>
NoopEuroAgent::meet_tribe_ui_sequence( MeetTribe const& ) {
  co_return e_declare_war_on_natives::no;
}

wait<> NoopEuroAgent::show_woodcut( e_woodcut ) { co_return; }

wait<base::heap_value<CapturableCargoItems>>
NoopEuroAgent::select_commodities_to_capture(
    UnitId const, UnitId const, CapturableCargo const& ) {
  co_return {};
}

wait<> NoopEuroAgent::notify_captured_cargo( Player const&,
                                             Player const&,
                                             Unit const&,
                                             Commodity const& ) {
  co_return;
}

Player const& NoopEuroAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

bool NoopEuroAgent::handle( signal::Foo const& ) {
  return false;
}

wait<int> NoopEuroAgent::handle( signal::Bar const& ) {
  co_return 0;
}

} // namespace rn
