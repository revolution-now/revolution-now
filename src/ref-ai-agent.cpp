/****************************************************************
**ref-ai-agent.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-07-07.
*
* Description: Implementation of IAgent for AI REF players.
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
** RefAIAgent::State
*****************************************************************/
struct RefAIAgent::State {
  unordered_map<UnitId, int> unit_moves;
};

/****************************************************************
** RefAIAgent
*****************************************************************/
RefAIAgent::RefAIAgent( e_player const player, SS& ss )
  : IAgent( player ),
    ss_( ss ),
    state_( make_unique<State>() ) {}

RefAIAgent::~RefAIAgent() = default;

wait<> RefAIAgent::message_box( string const& ) { co_return; }

wait<e_declare_war_on_natives>
RefAIAgent::meet_tribe_ui_sequence( MeetTribe const&,
                                    point const ) {
  co_return e_declare_war_on_natives::no;
}

wait<> RefAIAgent::show_woodcut( e_woodcut ) { co_return; }

wait<base::heap_value<CapturableCargoItems>>
RefAIAgent::select_commodities_to_capture(
    UnitId const, UnitId const, CapturableCargo const& ) {
  co_return {};
}

wait<> RefAIAgent::notify_captured_cargo( Player const&,
                                          Player const&,
                                          Unit const&,
                                          Commodity const& ) {
  co_return;
}

Player const& RefAIAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

bool RefAIAgent::human() const { return false; }

wait<maybe<int>> RefAIAgent::handle(
    signal::ChooseImmigrant const& ) {
  SHOULD_NOT_BE_HERE;
}

wait<> RefAIAgent::pan_tile( point const ) { co_return; }

wait<> RefAIAgent::pan_unit( UnitId const ) { co_return; }

wait<std::string> RefAIAgent::name_new_world() {
  SHOULD_NOT_BE_HERE;
}

wait<ui::e_confirm> RefAIAgent::should_king_transport_treasure(
    std::string const& ) {
  SHOULD_NOT_BE_HERE;
}

wait<chrono::microseconds> RefAIAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return us;
}

wait<ui::e_confirm>
RefAIAgent::should_explore_ancient_burial_mounds() {
  co_return ui::e_confirm::no;
}

command RefAIAgent::ask_orders( UnitId const ) {
  static e_direction d = {};

  d = cycle_enum( d );
  return command::move{ .d = d };
  // co_return command::forfeight{};
}

wait<ui::e_confirm> RefAIAgent::kiss_pinky_ring( string const&,
                                                 ColonyId,
                                                 e_commodity,
                                                 int const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm>
RefAIAgent::attack_with_partial_movement_points( UnitId const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIAgent::should_attack_natives(
    e_tribe const ) {
  co_return ui::e_confirm::yes;
}

wait<maybe<int>> RefAIAgent::pick_dump_cargo(
    map<int /*slot*/, Commodity> const& ) {
  co_return nothing;
}

wait<e_native_land_grab_result>
RefAIAgent::should_take_native_land(
    string const&,
    refl::enum_map<e_native_land_grab_result, string> const&,
    refl::enum_map<e_native_land_grab_result, bool> const& ) {
  co_return e_native_land_grab_result::cancel;
}

wait<ui::e_confirm> RefAIAgent::confirm_disband_unit(
    UnitId const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIAgent::confirm_build_inland_colony() {
  co_return ui::e_confirm::no;
}

wait<maybe<std::string>> RefAIAgent::name_colony() {
  co_return nothing;
}

wait<ui::e_confirm> RefAIAgent::should_make_landfall(
    bool const /*some_units_already_moved*/ ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm> RefAIAgent::should_sail_high_seas() {
  co_return ui::e_confirm::no;
}

} // namespace rn
