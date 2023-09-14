/****************************************************************
**ieuro-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Interface for asking for orders and behaviors for
*              european (non-ref) units.
*
*****************************************************************/
#include "ieuro-mind.hpp"

// Revolution Now
#include "co-wait.hpp"

using namespace std;

namespace rn {

/****************************************************************
** IEuroMind
*****************************************************************/
IEuroMind::IEuroMind( e_nation nation ) : nation_( nation ) {}

/****************************************************************
** NoopEuroMind
*****************************************************************/
NoopEuroMind::NoopEuroMind( e_nation nation )
  : IEuroMind( nation ) {}

wait<> NoopEuroMind::message_box( string const& ) { co_return; }

wait<e_declare_war_on_natives>
NoopEuroMind::meet_tribe_ui_sequence( MeetTribe const& ) {
  co_return e_declare_war_on_natives::no;
}

wait<> NoopEuroMind::show_woodcut( e_woodcut ) { co_return; }

} // namespace rn
