/****************************************************************
**iagent.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Interface for asking for orders and behaviors for
*              european (non-ref) units.
*
*****************************************************************/
#include "iagent.hpp"

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
** IAgent
*****************************************************************/
IAgent::IAgent( e_player player_type )
  : player_type_( player_type ) {}

/****************************************************************
** NoopAgent
*****************************************************************/
NoopAgent::NoopAgent( SSConst const& ss, e_player player )
  : IAgent( player ), ss_( ss ) {}

/****************************************************************
** Signals.
*****************************************************************/
using SignalHandlerT = NoopAgent;

wait<maybe<int>> NoopAgent::handle(
    signal::ChooseImmigrant const& ) {
  co_return nothing;
}

EMPTY_SIGNAL( ColonyDestroyedByNatives );
EMPTY_SIGNAL( ColonyDestroyedByStarvation );
EMPTY_SIGNAL( ColonySignal );
EMPTY_SIGNAL( ColonySignalTransient );
EMPTY_SIGNAL( ForestClearedNearColony );
EMPTY_SIGNAL( ImmigrantArrived );
EMPTY_SIGNAL( NoSpotForShip );
EMPTY_SIGNAL( PioneerExhaustedTools );
EMPTY_SIGNAL( PriceChange );
EMPTY_SIGNAL( RebelSentimentChanged );
EMPTY_SIGNAL( RefUnitAdded );
EMPTY_SIGNAL( ShipFinishedRepairs );
EMPTY_SIGNAL( TaxRateWillChange );
EMPTY_SIGNAL( TeaParty );
EMPTY_SIGNAL( TreasureArrived );
EMPTY_SIGNAL( TribeWipedOut );

/****************************************************************
** Named signals.
*****************************************************************/
wait<> NoopAgent::message_box( string const& ) { co_return; }

wait<e_declare_war_on_natives> NoopAgent::meet_tribe_ui_sequence(
    MeetTribe const&, point const ) {
  co_return e_declare_war_on_natives::no;
}

wait<> NoopAgent::show_woodcut( e_woodcut ) { co_return; }

wait<base::heap_value<CapturableCargoItems>>
NoopAgent::select_commodities_to_capture(
    UnitId const, UnitId const, CapturableCargo const& ) {
  co_return {};
}

wait<> NoopAgent::notify_captured_cargo( Player const&,
                                         Player const&,
                                         Unit const&,
                                         Commodity const& ) {
  co_return;
}

Player const& NoopAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

bool NoopAgent::human() const { return false; }

void NoopAgent::dump_last_message() const {}

wait<> NoopAgent::pan_tile( point const ) { co_return; }

wait<> NoopAgent::pan_unit( UnitId const ) { co_return; }

wait<std::string> NoopAgent::name_new_world() {
  co_return "none";
}

wait<ui::e_confirm> NoopAgent::should_king_transport_treasure(
    std::string const& ) {
  co_return ui::e_confirm::no;
}

wait<chrono::microseconds> NoopAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return us;
}

wait<ui::e_confirm>
NoopAgent::should_explore_ancient_burial_mounds() {
  co_return ui::e_confirm::no;
}

command NoopAgent::ask_orders( UnitId const ) {
  return command::forfeight{};
}

wait<ui::e_confirm> NoopAgent::kiss_pinky_ring( string const&,
                                                ColonyId,
                                                e_commodity,
                                                int const ) {
  co_return ui::e_confirm::yes;
}

wait<ui::e_confirm>
NoopAgent::attack_with_partial_movement_points( UnitId const ) {
  co_return ui::e_confirm::no;
}

wait<ui::e_confirm> NoopAgent::should_attack_natives(
    e_tribe const ) {
  co_return ui::e_confirm::yes;
}

wait<maybe<int>> NoopAgent::pick_dump_cargo(
    map<int /*slot*/, Commodity> const& ) {
  co_return nothing;
}

wait<e_native_land_grab_result>
NoopAgent::should_take_native_land(
    string const&,
    refl::enum_map<e_native_land_grab_result, string> const&,
    refl::enum_map<e_native_land_grab_result, bool> const& ) {
  co_return e_native_land_grab_result::cancel;
}

wait<ui::e_confirm> NoopAgent::confirm_disband_unit(
    UnitId const ) {
  co_return ui::e_confirm::no;
}

wait<ui::e_confirm> NoopAgent::confirm_build_inland_colony() {
  co_return ui::e_confirm::no;
}
wait<ui::e_confirm> NoopAgent::confirm_build_island_colony() {
  co_return ui::e_confirm::no;
}

wait<maybe<std::string>> NoopAgent::name_colony() {
  co_return nothing;
}

wait<ui::e_confirm> NoopAgent::should_make_landfall(
    bool const /*some_units_already_moved*/ ) {
  co_return ui::e_confirm::no;
}

wait<ui::e_confirm> NoopAgent::should_sail_high_seas(
    UnitId const ) {
  co_return ui::e_confirm::no;
}

EvolveGoto NoopAgent::evolve_goto( UnitId const ) {
  return EvolveGoto::abort{};
}

EvolveTradeRoute NoopAgent::evolve_trade_route( UnitId const ) {
  return EvolveTradeRoute::abort{};
}

} // namespace rn
