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
** HumanEuroMind
*****************************************************************/
HumanEuroMind::HumanEuroMind( e_player player, SS& ss,
                              IGui& gui )
  : IEuroMind( player ), ss_( ss ), gui_( gui ) {}

wait<> HumanEuroMind::message_box( string const& msg ) {
  co_await gui_.message_box( msg );
}

wait<e_declare_war_on_natives>
HumanEuroMind::meet_tribe_ui_sequence(
    MeetTribe const& meet_tribe ) {
  co_return co_await perform_meet_tribe_ui_sequence(
      ss_, *this, gui_, meet_tribe );
}

wait<> HumanEuroMind::show_woodcut( e_woodcut woodcut ) {
  co_await gui_.display_woodcut( woodcut );
}

wait<base::heap_value<CapturableCargoItems>>
HumanEuroMind::select_commodities_to_capture(
    UnitId const src, UnitId const dst,
    CapturableCargo const& capturable ) {
  co_return co_await select_items_to_capture_ui(
      ss_.as_const, gui_, src, dst, capturable );
}

wait<> HumanEuroMind::notify_captured_cargo(
    Player const& src_player, Player const& dst_player,
    Unit const& dst_unit, Commodity const& stolen ) {
  co_await notify_captured_cargo_human(
      gui_, src_player, dst_player, dst_unit, stolen );
}

Player const& HumanEuroMind::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

} // namespace rn
