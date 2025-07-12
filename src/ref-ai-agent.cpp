/****************************************************************
**ref-ai-agent.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-07.
*
* Description: Implementation of IEuroAgent for AI REF players.
*
*****************************************************************/
#include "ref-ai-agent.hpp"

// Revolution Now
#include "capture-cargo.rds.hpp"
#include "co-wait.hpp"

// ss
#include "players.hpp"
#include "ss/player.hpp"
#include "ss/ref.hpp"

// refl
#include "refl/query-enum.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::refl::cycle_enum;

}

/****************************************************************
** RefAIEuroAgent::State
*****************************************************************/
struct RefAIEuroAgent::State {
  unordered_map<UnitId, int> unit_moves;
};

/****************************************************************
** RefAIEuroAgent
*****************************************************************/
RefAIEuroAgent::RefAIEuroAgent( e_player const player, SS& ss )
  : IEuroAgent( player ),
    ss_( ss ),
    state_( make_unique<State>() ) {}

RefAIEuroAgent::~RefAIEuroAgent() = default;

wait<> RefAIEuroAgent::message_box( string const& ) {
  co_return;
}

wait<e_declare_war_on_natives>
RefAIEuroAgent::meet_tribe_ui_sequence( MeetTribe const& ) {
  co_return e_declare_war_on_natives::no;
}

wait<> RefAIEuroAgent::show_woodcut( e_woodcut ) { co_return; }

wait<base::heap_value<CapturableCargoItems>>
RefAIEuroAgent::select_commodities_to_capture(
    UnitId const, UnitId const, CapturableCargo const& ) {
  co_return {};
}

wait<> RefAIEuroAgent::notify_captured_cargo(
    Player const&, Player const&, Unit const&,
    Commodity const& ) {
  co_return;
}

Player const& RefAIEuroAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

bool RefAIEuroAgent::human() const { return false; }

wait<maybe<int>> RefAIEuroAgent::handle(
    signal::ChooseImmigrant const& ) {
  SHOULD_NOT_BE_HERE;
}

wait<> RefAIEuroAgent::pan_tile( point const ) { co_return; }

wait<> RefAIEuroAgent::pan_unit( UnitId const ) { co_return; }

wait<std::string> RefAIEuroAgent::name_new_world() {
  SHOULD_NOT_BE_HERE;
}

wait<ui::e_confirm>
RefAIEuroAgent::should_king_transport_treasure(
    std::string const& ) {
  SHOULD_NOT_BE_HERE;
}

wait<chrono::microseconds> RefAIEuroAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return us;
}

wait<ui::e_confirm>
RefAIEuroAgent::should_explore_ancient_burial_mounds() {
  co_return ui::e_confirm::no;
}

command RefAIEuroAgent::ask_orders( UnitId const ) {
  static e_direction d = {};

  d = cycle_enum( d );
  return command::move{ .d = d };
  // co_return command::forfeight{};
}

wait<ui::e_confirm> RefAIEuroAgent::kiss_pinky_ring(
    string const&, ColonyId, e_commodity, int const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm>
RefAIEuroAgent::attack_with_partial_movement_points(
    UnitId const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIEuroAgent::should_attack_natives(
    e_tribe const ) {
  co_return ui::e_confirm::yes;
}

} // namespace rn
