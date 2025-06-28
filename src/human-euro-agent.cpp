/****************************************************************
**human-euro-agent.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IEuroAgent for human players.
*
*****************************************************************/
#include "human-euro-agent.hpp"

// Revolution Now
#include "capture-cargo.hpp"
#include "co-wait.hpp"
#include "igui.hpp"
#include "meet-natives.hpp"

// ss
#include "players.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"

using namespace std;

namespace rn {

/****************************************************************
** HumanEuroAgent
*****************************************************************/
HumanEuroAgent::HumanEuroAgent( e_player player, SS& ss,
                                IGui& gui )
  : IEuroAgent( player ), ss_( ss ), gui_( gui ) {}

wait<> HumanEuroAgent::message_box( string const& msg ) {
  co_await gui_.message_box( msg );
}

wait<e_declare_war_on_natives>
HumanEuroAgent::meet_tribe_ui_sequence(
    MeetTribe const& meet_tribe ) {
  co_return co_await perform_meet_tribe_ui_sequence(
      ss_, *this, gui_, meet_tribe );
}

wait<> HumanEuroAgent::show_woodcut( e_woodcut woodcut ) {
  co_await gui_.display_woodcut( woodcut );
}

wait<base::heap_value<CapturableCargoItems>>
HumanEuroAgent::select_commodities_to_capture(
    UnitId const src, UnitId const dst,
    CapturableCargo const& capturable ) {
  co_return co_await select_items_to_capture_ui(
      ss_.as_const, gui_, src, dst, capturable );
}

wait<> HumanEuroAgent::notify_captured_cargo(
    Player const& src_player, Player const& dst_player,
    Unit const& dst_unit, Commodity const& stolen ) {
  co_await notify_captured_cargo_human(
      gui_, src_player, dst_player, dst_unit, stolen );
}

Player const& HumanEuroAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

} // namespace rn
