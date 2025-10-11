/****************************************************************
**human-agent.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-31.
*
* Description: Implementation of IAgent for human players.
*
*****************************************************************/
#include "human-agent.hpp"

// Revolution Now
#include "capture-cargo.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "commodity.hpp"
#include "disband.hpp"
#include "goto-viewer.hpp"
#include "goto.hpp"
#include "iengine.hpp"
#include "igui.hpp"
#include "land-view.hpp"
#include "meet-natives.hpp"
#include "plane-stack.hpp"
#include "roles.hpp"
#include "unit-mgr.hpp"
#include "visibility.hpp"

// config
#include "config/command.rds.hpp"
#include "config/nation.rds.hpp"
#include "config/natives.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/player.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/conv.hpp"
#include "base/logger.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::gfx::size;
using ::refl::enum_map;

valid_or<string> is_valid_colony_name_msg(
    ColoniesState const& colonies_state,
    string_view const name ) {
  auto const res =
      is_valid_new_colony_name( colonies_state, name );
  if( res ) return valid;
  switch( res.error() ) {
    case e_new_colony_name_err::spaces:
      return invalid(
          "Colony name must not start or end with spaces."s );
    case e_new_colony_name_err::already_exists:
      return invalid(
          "There is already a colony with that name!"s );
  }
}

} // namespace

/****************************************************************
** HumanAgent
*****************************************************************/
HumanAgent::HumanAgent( e_player player, IEngine& engine, SS& ss,
                        IGui& gui, Planes& planes )
  : IAgent( player ),
    engine_( engine ),
    ss_( ss ),
    gui_( gui ),
    planes_( planes ) {}

/****************************************************************
** Signals.
*****************************************************************/
using SignalHandlerT = HumanAgent;

void HumanAgent::handle(
    signal::ColonySignalTransient const& ctx ) {
  gui_.transient_message_box( ctx.msg );
}

wait<maybe<int>> HumanAgent::handle(
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

EMPTY_SIGNAL( ColonyDestroyedByNatives );
EMPTY_SIGNAL( ColonyDestroyedByStarvation );
EMPTY_SIGNAL( ColonySignal );
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
wait<> HumanAgent::message_box( string const& msg ) {
  co_await gui_.message_box( msg );
}

void HumanAgent::dump_last_message() const {}

wait<e_declare_war_on_natives>
HumanAgent::meet_tribe_ui_sequence( MeetTribe const& meet_tribe,
                                    point const tile ) {
  co_await land_view().ensure_visible( tile );
  co_return co_await perform_meet_tribe_ui_sequence(
      ss_, *this, gui_, meet_tribe );
}

wait<> HumanAgent::show_woodcut( e_woodcut woodcut ) {
  co_await gui_.display_woodcut( woodcut );
}

wait<base::heap_value<CapturableCargoItems>>
HumanAgent::select_commodities_to_capture(
    UnitId const src, UnitId const dst,
    CapturableCargo const& capturable ) {
  co_return co_await select_items_to_capture_ui(
      ss_.as_const, gui_, src, dst, capturable );
}

wait<> HumanAgent::notify_captured_cargo(
    Player const& src_player, Player const& dst_player,
    Unit const& dst_unit, Commodity const& stolen ) {
  co_await notify_captured_cargo_human(
      gui_, src_player, dst_player, dst_unit, stolen );
}

Player const& HumanAgent::player() {
  return player_for_player_or_die( as_const( ss_.players ),
                                   player_type() );
}

ILandViewPlane& HumanAgent::land_view() const {
  return planes_.get().get_bottom<ILandViewPlane>();
}

wait<> HumanAgent::pan_tile( point const tile ) {
  co_await land_view().ensure_visible( tile );
}

wait<> HumanAgent::pan_unit( UnitId const unit_id ) {
  co_await land_view().ensure_visible_unit( unit_id );
}

wait<string> HumanAgent::name_new_world() {
  co_return co_await gui_.required_string_input(
      { .msg = "You've discovered the new world!  What shall "
               "we call this land, Your Excellency?",
        .initial_text = config_nation.nations[player().nation]
                            .new_world_name } );
}

wait<ui::e_confirm> HumanAgent::should_king_transport_treasure(
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

wait<chrono::microseconds> HumanAgent::wait_for(
    chrono::milliseconds const us ) {
  co_return co_await gui_.wait_for( us );
}

wait<ui::e_confirm>
HumanAgent::should_explore_ancient_burial_mounds() {
  ui::e_confirm const res = co_await gui_.required_yes_no(
      { .msg = "You stumble across some mysterious ancient "
               "burial mounds.  Explore them?",
        .yes_label      = "Let us search for treasure!",
        .no_label       = "Leave them alone.",
        .no_comes_first = false } );
  co_return res;
}

command HumanAgent::ask_orders( UnitId const ) {
  SHOULD_NOT_BE_HERE;
}

wait<ui::e_confirm> HumanAgent::kiss_pinky_ring(
    string const& msg, ColonyId const colony_id,
    e_commodity const type, int const tax_increase ) {
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
  // Instead of just asking once as the OG does, we add a confir-
  // mation box on the default choise (Kiss Pinky Ring) to avoid
  // the player accidentally hitting enter on the box without
  // reading it, as typically happens with the many notifications
  // that pop up throughout a turn. For this particular one, we
  // want the player to really think about it each time it hap-
  // pens, especially because a tax increase (the default choice)
  // cannot be reversed.
  while( true ) {
    ui::e_confirm const answer =
        co_await gui_.required_yes_no( config );
    switch( answer ) {
      case ui::e_confirm::no:
        co_return answer;
      case ui::e_confirm::yes: {
        YesNoConfig const config{
          .msg =
              format( "The King applauds your loyalty and has "
                      "sent a contract containing the terms of "
                      "this [{}% tax increase].  Once accepted, "
                      "you may then kiss the Pinky Ring.",
                      tax_increase ),
          .yes_label      = "Accept",
          .no_label       = "Wait! Let us reconsider...",
          .no_comes_first = true,
        };
        if( co_await gui_.required_yes_no( config ) ==
            ui::e_confirm::yes )
          co_return ui::e_confirm::yes;
        break;
      }
    }
  }
}

wait<ui::e_confirm>
HumanAgent::attack_with_partial_movement_points(
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

wait<ui::e_confirm> HumanAgent::should_attack_natives(
    e_tribe const tribe ) {
  YesNoConfig const config{
    .msg = fmt::format(
        "Shall we attack the [{}]?",
        config_natives.tribes[tribe].name_singular ),
    .yes_label      = "Attack",
    .no_label       = "Cancel",
    .no_comes_first = true };
  maybe<ui::e_confirm> const proceed =
      co_await gui_.optional_yes_no( config );
  co_return proceed.value_or( ui::e_confirm::no );
}

wait<maybe<int>> HumanAgent::pick_dump_cargo(
    map<int /*slot*/, Commodity> const& options ) {
  ChoiceConfig config{
    .msg     = "What cargo would you like to dump overboard?",
    .options = {},
  };

  for( auto const& [slot, comm] : options ) {
    // FIXME: need to put these names into a config file with
    // both singular and plural versions.
    string const text = fmt::format(
        "{} {}", comm.quantity,
        lowercase_commodity_display_name( comm.type ) );
    ChoiceConfigOption option{
      .key          = fmt::to_string( slot ),
      .display_name = text,
    };
    config.options.push_back( option );
  }

  maybe<string> const selection =
      co_await gui_.optional_choice( config );
  if( !selection.has_value() ) co_return nothing;
  UNWRAP_CHECK_T( int const slot, base::stoi( *selection ) );
  co_return slot;
}

wait<e_native_land_grab_result>
HumanAgent::should_take_native_land(
    string const& msg,
    enum_map<e_native_land_grab_result, string> const& names,
    enum_map<e_native_land_grab_result, bool> const& disabled ) {
  EnumChoiceConfig config;
  config.msg = msg;
  auto const res =
      co_await gui_
          .optional_enum_choice<e_native_land_grab_result>(
              config, names, disabled );
  co_return res.value_or( e_native_land_grab_result::cancel );
}

wait<ui::e_confirm> HumanAgent::confirm_disband_unit(
    UnitId const unit_id ) {
  auto const viz_ = create_visibility_for(
      ss_, player_for_role( ss_, e_player_role::viewer ) );
  DisbandingPermissions const perms{
    .disbandable = { .units = { unit_id } } };
  auto const entities = co_await disband_tile_ui_interaction(
      ss_.as_const, gui_, engine_.textometer(), player(), *viz_,
      perms );
  co_return !entities.units.empty() ? ui::e_confirm::yes
                                    : ui::e_confirm::no;
}

wait<ui::e_confirm> HumanAgent::confirm_build_inland_colony() {
  YesNoConfig const config{
    .msg =
        "Your Excellency, this square does not have [ocean "
        "access].  This means that we will not be able to "
        "access it by ship and thus we will have to build a "
        "wagon train to transport goods to and from it.",
    .yes_label      = "Yes, that is exactly what I had in mind.",
    .no_label       = "Nevermind, I forgot about that.",
    .no_comes_first = true };
  maybe<ui::e_confirm> const answer =
      co_await gui_.optional_yes_no( config );
  co_return answer.value_or( ui::e_confirm::no );
}

wait<maybe<std::string>> HumanAgent::name_colony() {
  maybe<string> colony_name;
  while( true ) {
    colony_name = co_await gui_.optional_string_input(
        { .msg =
              "What shall this colony be named, your majesty?",
          .initial_text = colony_name.value_or( "" ) } );
    if( !colony_name.has_value() ) co_return nothing;
    valid_or<string> is_valid =
        is_valid_colony_name_msg( ss_.colonies, *colony_name );
    if( is_valid ) co_return *colony_name;
    co_await gui_.message_box( is_valid.error() );
  }
}

wait<ui::e_confirm> HumanAgent::should_make_landfall(
    bool const some_units_already_moved ) {
  string msg = "Would you like to make landfall?";
  if( some_units_already_moved )
    msg =
        "Some units onboard have already moved this turn. Would "
        "you like the remaining units to make landfall anyway?";
  maybe<ui::e_confirm> const answer =
      co_await gui_.optional_yes_no(
          { .msg            = msg,
            .yes_label      = "Make landfall",
            .no_label       = "Stay with ships",
            .no_comes_first = true } );
  co_return answer.value_or( ui::e_confirm::no );
}

wait<ui::e_confirm> HumanAgent::should_sail_high_seas() {
  auto const res = co_await gui_.optional_yes_no(
      { .msg       = "Would you like to sail the high seas?",
        .yes_label = "Yes, steady as she goes!",
        .no_label  = "No, let us remain in these waters." } );
  co_return res.value_or( ui::e_confirm::no );
}

void HumanAgent::new_goto( IGotoMapViewer const& viewer,
                           UnitId const unit_id,
                           goto_target const& target ) {
  Unit& unit = ss_.units.unit_for( unit_id );
  lg.info( "goto: {}", target );
  goto_registry_.paths.erase( unit_id );
  unit.clear_orders();
  // This should be validated when loading the save, namely
  // that a unit in goto mode must be either directly on the
  // map or in the cargo of another unit that is on the map.
  point const src =
      coord_for_unit_indirect_or_die( ss_.units, unit_id );
  SWITCH( target ) {
    CASE( map ) {
      point const dst = map.tile;
      auto const goto_path =
          compute_goto_path( viewer, src, dst );
      if( goto_path.reverse_path.empty() ) break;
      goto_registry_.paths[unit_id] = GotoExecution{
        .target = target, .path = std::move( goto_path ) };
      unit.orders() = unit_orders::go_to{ .target = target };
      break;
    }
    CASE( harbor ) {
      auto const goto_path =
          compute_harbor_goto_path( viewer, src );
      if( goto_path.reverse_path.empty() ) break;
      goto_registry_.paths[unit_id] = GotoExecution{
        .target = target, .path = std::move( goto_path ) };
      unit.orders() = unit_orders::go_to{ .target = target };
      break;
    }
  }
}

EvolveGoto HumanAgent::evolve_goto( UnitId const unit_id ) {
  Unit& unit = ss_.units.unit_for( unit_id );
  CHECK( unit.orders().holds<unit_orders::go_to>() );

  auto const abort = [&] {
    unit.clear_orders();
    goto_registry_.paths.erase( unit_id );
    return EvolveGoto::abort{};
  };

  if( unit_has_reached_goto_target( ss_, unit ) ) return abort();

  // Copy this for safety because we may end up changing it.
  auto const go_to = unit.orders().get<unit_orders::go_to>();
  // See if we need to get rid of an old goto target if the
  // target has changed. This can happen when a unit is given a
  // new goto order when it didn't complete a previous one. Doing
  // it this way allows us to not need an IAgent method to clear
  // the goto path when a new goto order is given.
  if( goto_registry_.paths.contains( unit_id ) &&
      goto_registry_.paths[unit_id].target != go_to.target )
    goto_registry_.paths.erase( unit_id );

  // This should be validated when loading the save, namely
  // that a unit in goto mode must be either directly on the
  // map or in the cargo of another unit that is on the map.
  point const src =
      coord_for_unit_indirect_or_die( ss_.units, unit_id );

  // NOTE: This is the visibility that is used to plot the path,
  // it is not necessarily what the player sees (because of the
  // "omniscient" option). This is important because we do not
  // always want to use this visibility in all cases.
  auto const goto_path_viz =
      create_visibility_for( ss_, [&] -> maybe<e_player> {
        // In the OG this is true, in the NG it defaults to
        // false.
        if( config_command.go_to.omniscient_path_finding )
          return nothing;
        if( !player_for_role( ss_, e_player_role::viewer )
                 .has_value() )
          // If the entire map is currently visible then we allow
          // the unit to use that, regardless of player.
          return nothing;
        // The entire map is not visible, so use the one of the
        // unit that is actually moving.
        return this->player_type();
      }() );
  CHECK( goto_path_viz );

  GotoMapViewer const goto_viewer( ss_, *goto_path_viz,
                                   player_type(), unit.type() );

  // This is the one we will use when we want to see exactly what
  // the player is seeing on screen.
  auto const real_viz = create_visibility_for(
      ss_, player_for_role( ss_, e_player_role::viewer ) );
  CHECK( real_viz );

  // Here we try twice, and this has two purposes. First, if the
  // unit's goto orders are new and its path hasn't been computed
  // yet, then the first attempt will fail and then we'll compute
  // the path and try again. But it is also needed for a unit
  // that already has a goto path, since as a unit explores
  // hidden tiles it may discover that its current path is no
  // longer viable and may need to recompute a path. But if the
  // second attempt to compute a path still does not succeed then
  // there is not further viable path and we cancel.
  auto const go_or_reattempt =
      [&] [[nodiscard]] (
          auto const& direction_fn ) -> EvolveGoto {
    if( auto const d = direction_fn(); d.has_value() )
      return EvolveGoto::move{ .to = *d };
    new_goto( goto_viewer, unit_id, go_to.target );
    if( auto const d = direction_fn(); d.has_value() )
      return EvolveGoto::move{ .to = *d };
    return abort();
  };

  SWITCH( go_to.target ) {
    CASE( map ) {
      auto const direction = [&] -> maybe<e_direction> {
        if( !goto_registry_.paths.contains( unit_id ) )
          return nothing;
        auto& reverse_path =
            goto_registry_.paths[unit_id].path.reverse_path;
        if( reverse_path.empty() ) return nothing;
        point const dst = reverse_path.back();
        reverse_path.pop_back();
        auto const d = src.direction_to( dst );
        if( !d.has_value() ) return nothing;
        // This means that, whatever the target tile is, we will
        // allow the unit to at least attempt to enter it. This
        // allows e.g. a ship to make landfall or a land unit to
        // attack a dwelling which are actions that would nor-
        // mally not be allowed because those units would not
        // normally be able to traverse those tiles en-route to
        // their target. In the event that the unit is not al-
        // lowed onto the tile then the goto orders will be
        // cleared, but that is ok because the user specifically
        // chose the target.
        if( dst == map.tile ) return *d;
        if( goto_viewer.can_enter_tile( dst ) ) return *d;
        return nothing;
      };

      if( src.direction_to( map.tile ).has_value() ) {
        // We are adjacent to the destination tile, so let's make
        // sure that the destination tile contains what we
        // thought it did when it was initially chosen by the
        // player. This avoids surprising such as going to an un-
        // explored tile and then finding there is a brave on
        // that tile and automatically attacking it. This should
        // not check-fail because our unit is adjacent to this
        // tile and so it should be clear.
        UNWRAP_CHECK_T( GotoTargetSnapshot const new_snapshot,
                        compute_goto_target_snapshot(
                            ss_, *real_viz, this->player().type,
                            map.tile ) );
        if( !is_new_goto_snapshot_allowed( map.snapshot,
                                           new_snapshot ) ) {
          lg.info(
              "cancelling goto command for {} because the "
              "destination tile contents have changed from [{}] "
              "to [{}] since the command was issued.",
              unit.type(), map.snapshot, new_snapshot );
          return abort();
        }
      }

      return go_or_reattempt( direction );
    }
    CASE( harbor ) {
      auto const direction = [&] -> maybe<e_direction> {
        if( auto const d =
                goto_viewer.is_sea_lane_launch_point( src );
            d.has_value() )
          return *d;
        if( !goto_registry_.paths.contains( unit_id ) )
          return nothing;
        auto& reverse_path =
            goto_registry_.paths[unit_id].path.reverse_path;
        if( reverse_path.empty() ) return nothing;
        point const dst = reverse_path.back();
        reverse_path.pop_back();
        auto const d = src.direction_to( dst );
        if( !d.has_value() ) return nothing;
        if( goto_viewer.can_enter_tile( dst ) ) return *d;
        return nothing;
      };

      // This is an optimization that tries to take advantage of
      // any new sea lane tiles that the ship reveals as it is
      // making it way along its previously computed path to the
      // sea lane. If there are any sea lane tiles within the
      // ship's visibility at the moment (meaning that they might
      // have been revealed on its last move) we will drop the
      // path which will cause it to be recomputed. We could have
      // computed a new path and then only used it if it were
      // better than the previous one, but that doesn't get us
      // anything because either way we're computing a new op-
      // timal path and ending up with the optimal path.
      if( goto_registry_.paths.contains( unit_id ) ) {
        vector<Coord> const visible = unit_visible_squares(
            ss_.as_const, player().type, unit.type(), src );
        lg.debug( "re-exploring {} tiles for sea lane.",
                  visible.size() );
        for( point const p : visible ) {
          if( goto_viewer.can_enter_tile( p ) &&
              goto_viewer.is_sea_lane_launch_point( p ) ) {
            lg.debug( "invalidating sea lane search." );
            goto_registry_.paths.erase( unit_id );
            break;
          }
        }
      }

      return go_or_reattempt( direction );
    }
  }
}

} // namespace rn
