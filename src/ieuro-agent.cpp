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

namespace {

using ::gfx::point;

}

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
NoopEuroAgent::meet_tribe_ui_sequence( MeetTribe const&,
                                       point const ) {
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

bool NoopEuroAgent::human() const { return false; }

wait<maybe<int>> NoopEuroAgent::handle(
    signal::ChooseImmigrant const& ) {
  co_return nothing;
}

wait<> NoopEuroAgent::pan_tile( point const ) { co_return; }

wait<> NoopEuroAgent::pan_unit( UnitId const ) { co_return; }

wait<std::string> NoopEuroAgent::name_new_world() {
  co_return "none";
}

wait<ui::e_confirm>
NoopEuroAgent::should_king_transport_treasure(
    std::string const& ) {
  co_return ui::e_confirm::no;
}

wait<chrono::microseconds> NoopEuroAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return us;
}

wait<ui::e_confirm>
NoopEuroAgent::should_explore_ancient_burial_mounds() {
  co_return ui::e_confirm::no;
}

command NoopEuroAgent::ask_orders( UnitId const ) {
  return command::forfeight{};
}

wait<ui::e_confirm> NoopEuroAgent::kiss_pinky_ring(
    string const&, ColonyId, e_commodity, int const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm>
NoopEuroAgent::attack_with_partial_movement_points(
    UnitId const ) {
  co_return ui::e_confirm::no;
}

wait<ui::e_confirm> NoopEuroAgent::should_attack_natives(
    e_tribe const ) {
  co_return ui::e_confirm::yes;
}

wait<maybe<int>> NoopEuroAgent::pick_dump_cargo(
    map<int /*slot*/, Commodity> const& ) {
  co_return nothing;
}

} // namespace rn
