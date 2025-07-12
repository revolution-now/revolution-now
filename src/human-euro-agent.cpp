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
#include "commodity.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "meet-natives.hpp"
#include "plane-stack.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

using ::gfx::point;

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

ILandViewPlane& HumanEuroAgent::land_view() const {
  return planes_.get().get_bottom<ILandViewPlane>();
}

wait<> HumanEuroAgent::pan_tile( point const tile ) {
  co_await land_view().ensure_visible( tile );
}

wait<> HumanEuroAgent::pan_unit( UnitId const unit_id ) {
  co_await land_view().ensure_visible_unit( unit_id );
}

wait<string> HumanEuroAgent::name_new_world() {
  co_return co_await gui_.required_string_input(
      { .msg = "You've discovered the new world!  What shall "
               "we call this land, Your Excellency?",
        .initial_text = config_nation.nations[player().nation]
                            .new_world_name } );
}

wait<ui::e_confirm>
HumanEuroAgent::should_king_transport_treasure(
    std::string const& msg ) {
  YesNoConfig const config{
    .msg            = msg,
    .yes_label      = "Accept.",
    .no_label       = "Decline.",
    .no_comes_first = false,
  };
  maybe<ui::e_confirm> const choice =
      co_await gui_.optional_yes_no( config );
  co_return choice.value_or( ui::e_confirm::no );
}

wait<chrono::microseconds> HumanEuroAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return co_await gui_.wait_for( us );
}

wait<ui::e_confirm>
HumanEuroAgent::should_explore_ancient_burial_mounds() {
  ui::e_confirm const res = co_await gui_.required_yes_no(
      { .msg = "You stumble across some mysterious ancient "
               "burial mounds.  Explore them?",
        .yes_label      = "Let us search for treasure!",
        .no_label       = "Leave them alone.",
        .no_comes_first = false } );
  co_return res;
}

command HumanEuroAgent::ask_orders( UnitId const ) {
  SHOULD_NOT_BE_HERE;
}

wait<ui::e_confirm> HumanEuroAgent::kiss_pinky_ring(
    string const& msg, ColonyId const colony_id,
    e_commodity const type, int const /*tax_increase*/ ) {
  string const party =
      fmt::format( "Hold '[{} {} party]'!",
                   ss_.colonies.colony_for( colony_id ).name,
                   uppercase_commodity_display_name( type ) );
  YesNoConfig const config{
    .msg            = msg,
    .yes_label      = "Kiss pinky ring.",
    .no_label       = party,
    .no_comes_first = false,
  };
  co_return co_await gui_.required_yes_no( config );
}

wait<ui::e_confirm>
HumanEuroAgent::attack_with_partial_movement_points(
    UnitId const unit_id ) {
  Unit const& unit = ss_.units.unit_for( unit_id );
  auto const res   = co_await gui_.optional_yes_no(
      { .msg = fmt::format(
            "This unit only has [{}] movement points and so "
              "will not be fighting at full strength. Continue?",
            unit.movement_points() ),
          .yes_label = "Yes, let us proceed with full force!",
          .no_label  = "No, do not attack." } );
  co_return res.value_or( ui::e_confirm::no );
}

} // namespace rn
