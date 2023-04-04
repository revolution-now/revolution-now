/****************************************************************
**human-euro-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IEuroMind for human players.
*
*****************************************************************/
#include "human-euro-mind.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "igui.hpp"
#include "meet-natives.hpp"

using namespace std;

namespace rn {

/****************************************************************
** HumanEuroMind
*****************************************************************/
HumanEuroMind::HumanEuroMind( e_nation nation, SS& ss,
                              IGui& gui )
  : IEuroMind( nation ), ss_( ss ), gui_( gui ) {}

wait<> HumanEuroMind::message_box( string const& msg ) {
  co_await gui_.message_box( msg );
}

wait<e_declare_war_on_natives>
HumanEuroMind::meet_tribe_ui_sequence(
    MeetTribe const& meet_tribe ) {
  co_return co_await perform_meet_tribe_ui_sequence(
      ss_, gui_, meet_tribe );
}

} // namespace rn
