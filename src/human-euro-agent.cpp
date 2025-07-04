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
#include "land-view.hpp"
#include "meet-natives.hpp"
#include "plane-stack.hpp"

// config
#include "config/unit-type.hpp"

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
                                IGui& gui, Planes& planes )
  : IEuroAgent( player ),
    ss_( ss ),
    gui_( gui ),
    planes_( planes ) {}

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

bool HumanEuroAgent::handle( signal::Foo const& ) {
  // TODO
  return false;
}

wait<int> HumanEuroAgent::handle( signal::Bar const& ) {
  // TODO
  co_return 0;
}

void HumanEuroAgent::handle(
    signal::ColonySignalTransient const& ctx ) {
  gui_.transient_message_box( ctx.msg );
}

wait<maybe<int>> HumanEuroAgent::handle(
    signal::ChooseImmigrant const& ctx ) {
  auto const& pool = ctx.types;
  vector<ChoiceConfigOption> options{
    { .key = "0", .display_name = unit_attr( pool[0] ).name },
    { .key = "1", .display_name = unit_attr( pool[1] ).name },
    { .key = "2", .display_name = unit_attr( pool[2] ).name },
  };
  ChoiceConfig const config{
    .msg     = ctx.msg,
    .options = options,
  };
  maybe<string> const res =
      co_await gui_.optional_choice( config );
  if( !res.has_value() ) co_return nothing;
  if( res == "0" ) co_return 0;
  if( res == "1" ) co_return 1;
  if( res == "2" ) co_return 2;
  FATAL(
      "unexpected selection result: {} (should be '0', '1', or "
      "'2')",
      res );
}

wait<> HumanEuroAgent::handle( signal::PanTile const& ctx ) {
  co_await planes_.get()
      .get_bottom<ILandViewPlane>()
      .ensure_visible( ctx.tile );
}

} // namespace rn
