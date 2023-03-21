/****************************************************************
**turn.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Main loop that processes a turn.
*
*****************************************************************/
#include "turn.hpp"

#include "turn-impl.rds.hpp"

// Revolution Now
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "command.hpp"
#include "fathers.hpp"
#include "game-options.hpp"
#include "gui.hpp"
#include "harbor-units.hpp"
#include "harbor-view.hpp"
#include "imap-updater.hpp"
#include "interrupts.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-edit.hpp"
#include "market.hpp"
#include "menu.hpp"
#include "on-map.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "plow.hpp"
#include "road.hpp"
#include "roles.hpp"
#include "save-game.hpp"
#include "sound.hpp"
#include "tax.hpp"
#include "ts.hpp"
#include "turn-plane.hpp"
#include "unit-mgr.hpp"
#include "unit.hpp"
#include "visibility.hpp"
#include "woodcut.hpp"

// config
#include "config/nation.hpp"
#include "config/turn.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/turn.rds.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"
#include "base/lambda.hpp"
#include "base/scope-exit.hpp"
#include "base/to-str-ext-std.hpp"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <algorithm>
#include <deque>
#include <queue>

using namespace std;

namespace rn {

struct IMapUpdater;

namespace {

/****************************************************************
** Global State
*****************************************************************/
// Globals relevant to end of turn.
namespace eot {

struct next_turn_t {};

using UserInput = base::variant< //
    e_menu_item,                 //
    LandViewPlayerInput,         //
    next_turn_t                  //
    >;

} // namespace eot

/****************************************************************
** Save-Game State
*****************************************************************/
// To be called once per turn.
wait<> advance_time( IGui& gui, TurnTimePoint& time_point ) {
  ++time_point.turns;
  if( time_point.year == 1600 &&
      time_point.season == e_season::spring )
    co_await gui.message_box(
        "Starting in the year [1600] the time scale "
        "changes.  Henceforth there will be both a "
        "[Spring] and a [Fall] turn each year." );
  bool const two_turns = ( time_point.year >= 1600 );
  switch( time_point.season ) {
    case e_season::winter:
      // We're not currently supporting four seasons per year, so
      // just revert it to the spring/fall cycle.
      time_point.season = e_season::spring;
      break;
    case e_season::spring:
      if( two_turns ) {
        // Two seasons per year.
        time_point.season = e_season::autumn;
      } else {
        // Stay in Spring and just go to the next year.
        ++time_point.year;
      }
      break;
    case e_season::summer:
      // We're not currently supporting four seasons per year, so
      // just revert it to the spring/fall cycle.
      time_point.season = e_season::autumn;
      break;
    case e_season::autumn:
      // Two seasons per year.
      time_point.season = e_season::spring;
      ++time_point.year;
      break;
  }
}

/****************************************************************
** Helpers
*****************************************************************/
wait<> proceed_to_exit( SSConst const& ss, TS& ts ) {
  YesNoConfig const          config{ .msg       = "Exit to DOS?",
                                     .yes_label = "Yes",
                                     .no_label  = "No",
                                     .no_comes_first = true };
  maybe<ui::e_confirm> const answer =
      co_await ts.gui.optional_yes_no( config );
  if( answer != ui::e_confirm::yes ) co_return;
  bool const can_proceed = co_await check_ask_save( ss, ts );
  if( can_proceed ) throw game_quit_interrupt{};
}

// If the element is present in the deque then it will be erased.
// The function will return true if the value was found (and
// erased). This will erase multiple instances of the element if
// there are multiple in the queue. Note that this is order N.
bool erase_from_deque_if_present( deque<UnitId>& q, UnitId id ) {
  bool found = false;
  while( true ) {
    auto it = find( q.begin(), q.end(), id );
    if( it == q.end() ) break;
    found = true;
    q.erase( it );
  }
  return found;
}

void prioritize_unit( deque<UnitId>& q, UnitId id ) {
  erase_from_deque_if_present( q, id );
  q.push_front( id );
}

// We use movement points for all units to track whether they've
// been advanced this turn, even for those that are not on the
// map. That is so that we don't have to introduce another piece
// of state to track that.
void finish_turn( Unit& unit ) { unit.forfeight_mv_points(); }

[[nodiscard]] bool finished_turn( Unit const& unit ) {
  return unit.mv_pts_exhausted();
}

bool should_remove_unit_from_queue( Unit const& unit ) {
  if( finished_turn( unit ) ) return true;
  switch( unit.orders().to_enum() ) {
    using e = unit_orders::e;
    case e::fortified:
      return true;
    case e::fortifying:
      return false;
    case e::sentry:
      return true;
    case e::road:
      return false;
    case e::plow:
      return false;
    case e::none:
      return false;
    case e::damaged:
      return false;
  }
}

// Get all units in the eight squares that surround coord.
vector<UnitId> surrounding_euro_units(
    UnitsState const& units_state, Coord const& coord ) {
  vector<UnitId> res;
  for( e_direction d : refl::enum_values<e_direction> )
    for( GenericUnitId id :
         units_state.from_coord( coord.moved( d ) ) )
      if( units_state.unit_kind( id ) == e_unit_kind::euro )
        res.push_back( units_state.check_euro_unit( id ) );
  return res;
}

// See if `unit` needs to be unsentry'd due to surrounding for-
// eign units.
void try_unsentry_unit( SS& ss, Unit& unit ) {
  if( !unit.orders().holds<unit_orders::sentry>() ) return;
  // Don't use the "indirect" version here because we don't want
  // to e.g. wake up units that are sentry'd on ships.
  maybe<Coord> loc = ss.units.maybe_coord_for( unit.id() );
  if( !loc.has_value() ) return;
  for( UnitId id : surrounding_euro_units( ss.units, *loc ) ) {
    if( ss.units.unit_for( id ).nation() != unit.nation() ) {
      unit.clear_orders();
      return;
    }
  }
}

// See if any foreign units in the vicinity of src_id need to be
// unsentry'd.
void unsentry_surroundings( UnitsState& units_state,
                            Unit const& src_unit ) {
  Coord src_loc = coord_for_unit_indirect_or_die(
      units_state, src_unit.id() );
  for( UnitId id :
       surrounding_euro_units( units_state, src_loc ) ) {
    Unit& unit = units_state.unit_for( id );
    if( !unit.orders().holds<unit_orders::sentry>() ) continue;
    if( unit.nation() == src_unit.nation() ) continue;
    unit.clear_orders();
  }
}

vector<UnitId> euro_units_all( UnitsState const& units_state,
                               e_nation          n ) {
  vector<UnitId> res;
  res.reserve( units_state.all().size() );
  for( auto const& p : units_state.euro_all() )
    if( n == p.second->unit.nation() ) res.push_back( p.first );
  return res;
}

// Apply a function to all units. The function may mutate the
// units.
void map_all_euro_units(
    UnitsState&                       units_state,
    base::function_ref<void( Unit& )> func ) {
  for( auto& p : units_state.euro_all() )
    func( units_state.unit_for( p.first ) );
}

// This will map only over those that haven't yet moved this
// turn.
void map_active_euro_units(
    UnitsState& units_state, e_nation nation,
    base::function_ref<void( Unit& )> func ) {
  for( auto& p : units_state.euro_all() ) {
    Unit& unit = units_state.unit_for( p.first );
    if( unit.mv_pts_exhausted() ) continue;
    if( unit.nation() == nation ) func( unit );
  }
}

// This won't actually prioritize the units, it will just check
// the request and return the units that can be prioritized.
wait<vector<UnitId>> process_unit_prioritization_request(
    SSConst const& ss, TS& ts,
    LandViewPlayerInput::prioritize const& request ) {
  // Move some units to the front of the queue.
  auto prioritize = request.units;
  erase_if( prioritize, [&]( UnitId id ) {
    return finished_turn( ss.units.unit_for( id ) );
  } );
  auto orig_size = request.units.size();
  auto curr_size = prioritize.size();
  CHECK( curr_size <= orig_size );
  if( curr_size == 0 )
    co_await ts.gui.message_box(
        "The selected unit(s) have already moved this turn." );
  else if( curr_size < orig_size )
    co_await ts.gui.message_box(
        "Some of the selected units have already moved this "
        "turn." );
  co_return std::move( prioritize );
}

/****************************************************************
** Menu Handlers
*****************************************************************/
wait<> menu_handler( SS& ss, TS& ts, Player& player,
                     e_menu_item item ) {
  switch( item ) {
    case e_menu_item::exit: {
      co_await proceed_to_exit( ss, ts );
      break;
    }
    case e_menu_item::save: {
      co_await save_game_menu( ss, ts );
      break;
    }
    case e_menu_item::load: {
      bool const can_proceed = co_await check_ask_save( ss, ts );
      if( !can_proceed ) break;
      game_load_interrupt load;
      load.slot = co_await choose_load_slot( ts );
      if( load.slot.has_value() ) throw load;
      break;
    }
    case e_menu_item::revolution: {
      // TODO: requires 50% rebel sentiment.
      ChoiceConfig config{
          .msg     = "Declare Revolution?",
          .options = {
              { .key = "no", .display_name = "Not Yet..." },
              { .key          = "yes",
                .display_name = "Give me liberty or give me "
                                "death!" } } };
      maybe<string> const answer =
          co_await ts.gui.optional_choice( config );
      co_await ts.gui.message_box( "You selected: {}", answer );
      if( answer == "yes" )
        player.revolution_status = e_revolution_status::declared;
      break;
    }
    case e_menu_item::harbor_view: {
      co_await show_harbor_view( ss, ts, player,
                                 /*selected_unit=*/nothing );
      break;
    }
    case e_menu_item::cheat_explore_entire_map: {
      cheat_explore_entire_map( ss, ts );
      break;
    }
    case e_menu_item::cheat_map_editor: {
      // Need to co_await so that the map_updater stays alive.
      co_await run_map_editor( ss, ts );
      break;
    }
    case e_menu_item::cheat_edit_fathers: {
      co_await cheat_edit_fathers( ss, ts, player );
      break;
    }
    case e_menu_item::game_options: {
      co_await open_game_options_box( ss, ts );
      break;
    }
    default: {
      FATAL( "Not supposed to be handling menu item {} here.",
             item );
    }
  }
}

wait<e_menu_item> wait_for_menu_selection(
    MenuPlane& menu_plane ) {
  TurnPlane turn_plane( menu_plane );
  co_return co_await turn_plane.next_menu_action();
}

/****************************************************************
** Processing Player Input (End of Turn).
*****************************************************************/
namespace eot {

wait<EndOfTurnResult> process_player_input( e_menu_item item,
                                            SS& ss, TS& ts,
                                            Player& player ) {
  // In the future we might need to put logic here that is spe-
  // cific to the end-of-turn, but for now this is sufficient.
  co_await menu_handler( ss, ts, player, item );
  co_return EndOfTurnResult::not_done_yet{};
}

wait<EndOfTurnResult> process_player_input(
    LandViewPlayerInput const& input, SS& ss, TS& ts,
    Player& player ) {
  switch( input.to_enum() ) {
    using e = LandViewPlayerInput::e;
    case e::colony: {
      e_colony_abandoned const abandoned =
          co_await ts.colony_viewer.show(
              ts, input.get<LandViewPlayerInput::colony>().id );
      if( abandoned == e_colony_abandoned::yes )
        // Nothing really special to do here.
        break;
      break;
    }
    case e::european_status: {
      co_await show_harbor_view( ss, ts, player,
                                 /*selected_unit=*/nothing );
      break;
    }
    case e::next_turn:
      co_return EndOfTurnResult::proceed{};
    case e::exit:
      co_await proceed_to_exit( ss, ts );
      break;
    case e::give_command:
      break;
    case e::prioritize: {
      auto& val = input.get<LandViewPlayerInput::prioritize>();
      vector<UnitId> const units =
          co_await process_unit_prioritization_request( ss, ts,
                                                        val );
      if( units.empty() ) break;
      // Unlike during the turn, we don't actually prioritize the
      // list of units that are returned by this function here
      // because there is no active unit queue. So instead we
      // will just let them be as they are (they will have had
      // their orders cleared) and we will just signal to the
      // caller that we want to go back to the units turn, at
      // which point the units with cleared orders will be found
      // (assuming they have movement points left) and they will
      // ask for orders. That said, we do specify the unit that
      // was most recently activated so that it can be communi-
      // cated back to the units phase and that unit can ask for
      // orders first. Otherwise, when the player clears the or-
      // ders of multiple units and then activates the final one
      // so that they start asking for orders, a "random" one
      // would be selected to actually ask for orders first and
      // not the one that the player last clicked on. This would
      // seem strange to the player, and this avoids that.
      co_return EndOfTurnResult::return_to_units{
          .first_to_ask = units.back() };
    }
  }
  co_return EndOfTurnResult::not_done_yet{};
}

wait<EndOfTurnResult> process_player_input( next_turn_t, SS&,
                                            TS&, Player& ) {
  lg.debug( "end of turn button clicked." );
  co_return EndOfTurnResult::proceed{};
}

wait<EndOfTurnResult> process_input( SS& ss, TS& ts,
                                     Player& player ) {
  auto wait_for_button =
      co::fmap( [] λ( next_turn_t{} ),
                ts.planes.panel().wait_for_eot_button_click() );
  // The reason that we want to use co::first here instead of in-
  // terleaving the three streams is because as soon as one be-
  // comes ready (and we start processing it) we want all the
  // others to be automatically be cancelled, which will have the
  // effect of disabling further input on them (e.g., disabling
  // menu items), which is what we want for a good user experi-
  // ence.
  UserInput command = co_await co::first(          //
      wait_for_menu_selection( ts.planes.menu() ), //
      ts.planes.land_view().eot_get_next_input(),  //
      std::move( wait_for_button )                 //
  );
  co_return co_await rn::visit(
      command, LC( process_player_input( _, ss, ts, player ) ) );
}

} // namespace eot

// Enters the EOT phase and processes a single input then re-
// turns.
wait<EndOfTurnResult> end_of_turn( SS& ss, TS& ts,
                                   Player& player ) {
  co_return co_await eot::process_input( ss, ts, player );
}

/****************************************************************
** Processing Player Input (During Turn).
*****************************************************************/
wait<> process_player_input( UnitId, e_menu_item item, SS& ss,
                             TS& ts, Player& player,
                             NationTurnState::units& ) {
  // In the future we might need to put logic here that is spe-
  // cific to the mid-turn scenario, but for now this is suffi-
  // cient.
  return menu_handler( ss, ts, player, item );
}

wait<> process_player_input(
    UnitId id, LandViewPlayerInput const& input, SS& ss, TS& ts,
    Player& player, NationTurnState::units& nat_units ) {
  auto& st = nat_units;
  auto& q  = st.q;
  switch( input.to_enum() ) {
    using e = LandViewPlayerInput::e;
    case e::next_turn: {
      // The land view should never send us a 'next turn' command
      // when we are not at the end of a turn.
      SHOULD_NOT_BE_HERE;
    }
    case e::exit:
      co_await proceed_to_exit( ss, ts );
      break;
    case e::colony: {
      e_colony_abandoned const abandoned =
          co_await ts.colony_viewer.show(
              ts, input.get<LandViewPlayerInput::colony>().id );
      if( abandoned == e_colony_abandoned::yes )
        // Nothing really special to do here.
        co_return;
      break;
    }
    case e::european_status: {
      co_await show_harbor_view( ss, ts, player,
                                 /*selected_unit=*/nothing );
      break;
    }
    // We have some orders for the current unit.
    case e::give_command: {
      auto& command =
          input.get<LandViewPlayerInput::give_command>().cmd;
      if( command.holds<command::wait>() ) {
        // Just remove it form the queue, and it'll get picked up
        // in the next iteration. We don't want to push this unit
        // onto the back of the queue since that will cause it to
        // appear multiple times in the queue, which is actually
        // OK, but not ideal from a UX perspective because it
        // will cause the unit to ask for orders multiple times
        // during a single cycle of "wait" commands, i.e., the
        // user just pressing "wait" through all of the available
        // units. FIXME: this may still not be working ideally,
        // since it can still theoretically cause the same unit
        // to ask for orders twice in a row, if for example some
        // units were prioritized (effectively rearranging the
        // order of the queue, then the player hits wait a few
        // times, then the last unit to ask for orders happens to
        // be the first in the queue on the next round. I think
        // this will probably only happen when there are only a
        // few active units, but it is quite noticeable in that
        // situation and would be nice to fix if possible (though
        // it may be nontrivial) since it causes a weird user ex-
        // perience when it happens.
        CHECK( q.front() == id );
        q.pop_front();
        break;
      }
      if( command.holds<command::forfeight>() ) {
        ss.units.unit_for( id ).forfeight_mv_points();
        break;
      }

      co_await ts.planes.land_view().ensure_visible_unit( id );
      unique_ptr<CommandHandler> handler =
          command_handler( ss, ts, player, id, command );
      CHECK( handler );
      Coord old_loc =
          coord_for_unit_indirect_or_die( ss.units, id );

      auto run_result = co_await handler->run();
      if( !run_result.order_was_run ) break;

      // !! The unit may no longer exist at this point, e.g. if
      // they were disbanded or if they lost a battle to the na-
      // tives.

      // If the unit still exists, check if it has moved squares
      // in any fashion, and if so then wake up any foreign sen-
      // try'd neighbors.
      if( ss.units.exists( id ) ) {
        maybe<Coord> new_loc =
            coord_for_unit_indirect( ss.units, id );
        if( new_loc && *new_loc != old_loc )
          unsentry_surroundings( ss.units,
                                 ss.units.unit_for( id ) );
      }

      for( auto id : run_result.units_to_prioritize )
        prioritize_unit( q, id );
      break;
    }
    case e::prioritize: {
      auto& val = input.get<LandViewPlayerInput::prioritize>();
      vector<UnitId> const prioritize =
          co_await process_unit_prioritization_request( ss, ts,
                                                        val );
      for( UnitId id_to_add : prioritize )
        prioritize_unit( q, id_to_add );
      break;
    }
  }
}

wait<LandViewPlayerInput> landview_player_input(
    ILandViewPlane&         land_view_plane,
    NationTurnState::units& nat_units,
    UnitsState const& units_state, UnitId id ) {
  LandViewPlayerInput response;
  if( auto maybe_command = pop_unit_command( id ) ) {
    response = LandViewPlayerInput::give_command{
        .cmd = *maybe_command };
  } else {
    lg.debug( "asking orders for: {}",
              debug_string( units_state, id ) );
    nat_units.skip_eot = true;
    response = co_await land_view_plane.get_next_input( id );
  }
  co_return response;
}

wait<> query_unit_input( UnitId id, SS& ss, TS& ts,
                         Player&                 player,
                         NationTurnState::units& nat_units ) {
  auto command = co_await co::first(
      wait_for_menu_selection( ts.planes.menu() ),
      landview_player_input( ts.planes.land_view(), nat_units,
                             ss.units, id ) );
  co_await overload_visit( command, [&]( auto const& action ) {
    return process_player_input( id, action, ss, ts, player,
                                 nat_units );
  } );
  // A this point we should return because we want to in general
  // allow for the possibility and any action executed above
  // might affect the status of the unit asking for orders, and
  // so returning will cause the unit to be re-examined.
}

/****************************************************************
** Advancing Units.
*****************************************************************/
// Returns true if the unit needs to ask the user for input.
wait<bool> advance_unit( SS& ss, TS& ts, Player& player,
                         UnitId id ) {
  Unit& unit = ss.units.unit_for( id );
  CHECK( !should_remove_unit_from_queue( unit ) );

  if( unit.orders().holds<unit_orders::fortifying>() ) {
    // Any units that are in the "fortifying" state at the start
    // of their turn get "promoted" for the "fortified" status,
    // which means they are actually fortified and get those ben-
    // efits. The OG consumes movement points both at the moment
    // that the player initially fortifies the unit, then once on
    // the following turn (where we are now) where the unit is
    // transitioned from "fortifying" to "fortified." But note
    // that thereafter, the fortified unit will not have its
    // movement points consumed at the start of their turn; this
    // allows the player to wake a fortified unit and move it
    // that turn. It's just that the player can't do that for the
    // first two turns, unless they happen to activate the unit
    // while it is in the "fortifying" state but before we do
    // this next step of transitioning it to "fortified" (this is
    // also the behavior of the OG).
    unit.orders() = unit_orders::fortified{};
    unit.forfeight_mv_points();
    co_return false;
  }

  if( auto damaged =
          unit.orders().get_if<unit_orders::damaged>();
      damaged.has_value() ) {
    if( --damaged->turns_until_repair == 0 ) {
      unit.clear_orders();
      if( ss.units.maybe_coord_for( unit.id() ) )
        // Unit is in a colony being repaired, so we can just let
        // it ask for orders.
        co_return true;
      else {
        // Unit is in the harbor. We could make it sail back to
        // the new world, but probably best to just let the
        // player decide.
        co_await ts.gui.message_box(
            "Our [{}] has finished its repairs in [{}].",
            unit.desc().name,
            nation_obj( player.nation ).harbor_city_name );
        co_await show_harbor_view( ss, ts, player, unit.id() );
        co_return false;
      }
    }
    // Need to forfeign movement points here to mark that we've
    // evolved this unit this turn (decreased its remaining
    // turns). If we didn't do this then this unit could keep
    // getting cycled through (e.g. `wait` cycles or after a
    // save-game reload) and would have its count decreased mul-
    // tiple times per turn.
    unit.forfeight_mv_points();
    co_return false;
  }

  if( unit.orders().holds<unit_orders::road>() ) {
    perform_road_work( ss.units, ss.terrain, as_const( player ),
                       ts.map_updater, unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders().holds<unit_orders::none>() );
      co_await ts.planes.land_view().ensure_visible_unit( id );
      co_await ts.gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( !unit.orders().holds<unit_orders::road>() );
  }

  if( unit.orders().holds<unit_orders::plow>() ) {
    PlowResult const plow_result = perform_plow_work(
        ss, as_const( player ), ts.map_updater, unit );
    if( auto o =
            plow_result.get_if<PlowResult::cleared_forest>();
        o.has_value() && o->yield.has_value() ) {
      LumberYield const& yield = *o->yield;
      string const       msg   = fmt::format(
          "Forest cleared near [{}].  [{}] lumber "
                  "added to colony's stockpile.",
          ss.colonies.colony_for( yield.colony_id ).name,
          yield.yield_to_add_to_colony );
      co_await ts.gui.message_box( msg );
    }
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders().holds<unit_orders::none>() );
      co_await ts.planes.land_view().ensure_visible_unit( id );
      co_await ts.gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( !unit.orders().holds<unit_orders::plow>() );
  }

  if( is_unit_in_port( ss.units, id ) ) {
    finish_turn( unit );
    co_return false; // do not ask for orders.
  }

  // If it is a ship on the high seas then advance it. If it has
  // arrived in the old world then jump to the old world screen.
  if( is_unit_inbound( ss.units, id ) ||
      is_unit_outbound( ss.units, id ) ) {
    e_high_seas_result res =
        advance_unit_on_high_seas( ss, player, id );
    switch( res ) {
      case e_high_seas_result::still_traveling:
        finish_turn( unit );
        co_return false; // do not ask for orders.
      case e_high_seas_result::arrived_in_new_world: {
        lg.debug( "unit has arrived in new world." );
        maybe<Coord> const dst_coord =
            find_new_world_arrival_square(
                ss, ts, player,
                ss.units.harbor_view_state_of( id )
                    .sailed_from );
        if( !dst_coord.has_value() ) {
          co_await ts.gui.message_box(
              "Unfortunately, while our [{}] has arrived in the "
              "new world, there are no appropriate water "
              "squares on which to place it.  We will try again "
              "next turn.",
              ss.units.unit_for( id ).desc().name );
          finish_turn( unit );
          break;
        }
        ss.units.unit_for( id ).clear_orders();
        maybe<UnitDeleted> const unit_deleted =
            co_await unit_ownership_change(
                ss, id,
                EuroUnitOwnershipChangeTo::world{
                    .ts = &ts, .target = *dst_coord } );
        // There are no LCR tiles on water squares.
        CHECK( !unit_deleted.has_value() );
        // This is not required, but it is for a good player ex-
        // perience. If there are more ships still in port then
        // select one of them, because ideally if there are ships
        // in port then when the player goes to the harbor view,
        // one of them should always be selected.
        update_harbor_selected_unit( ss.units, player );
        unsentry_surroundings( ss.units,
                               ss.units.unit_for( id ) );
        co_return true; // needs to ask for orders.
      }
      case e_high_seas_result::arrived_in_harbor: {
        lg.debug( "unit has arrived in old world." );
        finish_turn( unit );
        if( unit.cargo()
                .count_items_of_type<Cargo::commodity>() > 0 )
          co_await display_woodcut_if_needed(
              ts, player, e_woodcut::cargo_from_the_new_world );
        co_await show_harbor_view( ss, ts, player, id );
        co_return false; // do not ask for orders.
      }
    }
  }

  if( !is_unit_on_map_indirect( ss.units, id ) ) {
    finish_turn( unit );
    co_return false;
  }

  // Unit needs to ask for orders.
  co_return true;
}

wait<> units_turn_one_pass( SS& ss, TS& ts, Player& player,
                            NationTurnState::units& nat_units,
                            deque<UnitId>&          q ) {
  while( !q.empty() ) {
    // lg.trace( "q: {}", q );
    UnitId id = q.front();
    // We need this check because units can be added into the
    // queue in this loop by user input. Also, the very first
    // check that we must do needs to be to check if the unit
    // still exists, which it might not if e.g. it was disbanded.
    if( !ss.units.exists( id ) ||
        should_remove_unit_from_queue(
            ss.units.unit_for( id ) ) ) {
      q.pop_front();
      continue;
    }

    bool should_ask =
        co_await advance_unit( ss, ts, player, id );
    if( !should_ask ) {
      q.pop_front();
      continue;
    }

    // We have a unit that needs to ask the user for orders. This
    // will open things up to player input not only to the unit
    // that needs orders, but to all units on the map, and will
    // update the queue with any changes that result. This func-
    // tion will return on any player input (e.g. clearing the
    // orders of another unit) and so we might need to circle
    // back to this line a few times in this while loop until we
    // get the order for the unit in question (unless the player
    // activates another unit).
    co_await query_unit_input( id, ss, ts, player, nat_units );
    // !! The unit may no longer exist at this point, e.g. if
    // they were disbanded or if they lost a battle to the na-
    // tives.
  }
}

wait<> units_turn( SS& ss, TS& ts, Player& player,
                   NationTurnState::units& nat_units ) {
  auto& st = nat_units;
  auto& q  = st.q;

  // Unsentry any units that are sentried but have foreign units
  // in an adjacent square. FIXME: move this to the the function
  // that is called to put a unit on a new map square.
  map_active_euro_units( ss.units, player.nation,
                         [&]( Unit& unit ) {
                           return try_unsentry_unit( ss, unit );
                         } );

  // Here we will keep reloading all of the units (that still
  // need to move) and making passes over them in order make sure
  // that we systematically get to all units that need to move
  // this turn, including the ones that were activated during the
  // turn. For example, during the course of moving units, the
  // user might activate a unit in the land view, or they might
  // open a colony view and active units there, or a new unit
  // might be created, etc. There are many ways that this can
  // happen, and this will ensure that we get to all of those
  // units in a systematic way without having to handle each sit-
  // uation specially.
  //
  // That said, we also need this to work right when there are
  // already some units in the queue on the first iteration, as
  // would be the case just after deserialization.
  while( true ) {
    co_await units_turn_one_pass( ss, ts, player, nat_units, q );
    CHECK( q.empty() );
    // Refill the queue.
    vector<UnitId> units =
        euro_units_all( ss.units, player.nation );
    util::sort_by_key( units, []( auto id ) { return id; } );
    erase_if( units, [&]( UnitId id ) {
      return should_remove_unit_from_queue(
          ss.units.unit_for( id ) );
    } );
    if( units.empty() ) co_return;
    for( UnitId id : units ) q.push_back( id );
  }
}

/****************************************************************
** Per-Colony Turn Processor
*****************************************************************/
wait<> colonies_turn( SS& ss, TS& ts, Player& player ) {
  co_await evolve_colonies_for_player( ss, ts, player );
}

// Here we do things that must be done once per turn but where we
// want the colonies to be evolved first.
wait<> post_colonies( SS& ss, TS& ts, Player& player ) {
  // Founding fathers.
  co_await pick_founding_father_if_needed( ss, ts, player );
  maybe<e_founding_father> const new_father =
      check_founding_fathers( ss, player );
  if( new_father.has_value() ) {
    co_await play_new_father_cut_scene( ts, player,
                                        *new_father );
    // This will affect any one-time changes that the new father
    // causes. E.g. for John Paul Jones it will create the
    // frigate.
    on_father_received( ss, ts, player, *new_father );
  }
}

/****************************************************************
** Per-Nation Turn Processor
*****************************************************************/
// Here we do things that must be done once at the start of each
// nation's turn but where the player can't save the game until
// they are complete.
wait<> nation_start_of_turn( SS& ss, TS& ts, Player& player ) {
  // Evolve market prices.
  if( ss.turn.time_point.turns >
      config_turn.turns_to_wait.market_evolution ) {
    // This will actually change the prices, then will return
    // info about which ones it changed.
    refl::enum_map<e_commodity, PriceChange> changes =
        evolve_player_prices( ss, player );
    for( e_commodity comm : refl::enum_values<e_commodity> )
      if( changes[comm].delta != 0 )
        co_await display_price_change_notification(
            ts, player, changes[comm] );
  }

  // Check for tax events (typically increases).
  co_await start_of_turn_tax_check( ss, ts, player );

  // TODO:
  //
  //   1. REF.
  //   2. Sending units. NOTE: when your home country does this
  //      there is a probability for an immediate large tax in-
  //      crease; see config/tax file.
  //   3. etc.
  //
}

// Processes the current state and returns the next state.
wait<maybe<NationTurnState>> nation_turn_iter(
    SS& ss, TS& ts, e_nation nation, NationTurnState& st ) {
  Player& player =
      player_for_nation_or_die( ss.players, nation );
  // `visibility` determines from whose point of view the map is
  // drawn with respect to which tiles are hidden. This will po-
  // tentially redraw the map (if necessary) to align with the
  // nation from whose perspective we are currently viewing the
  // map (if any) as specified in the land view state.
  update_map_visibility(
      ts, player_for_role( ss, e_player_role::viewer ) );

  SWITCH( st ) {
    CASE( not_started ) {
      print_bar( '-', fmt::format( "[ {} ]", nation ) );
      // TODO: Until we have AI.
      if( !ss.players.humans[nation] )
        co_return NationTurnState::finished{};
      co_await nation_start_of_turn( ss, ts, player );
      co_return NationTurnState::colonies{};
    }
    CASE( colonies ) {
      co_await colonies_turn( ss, ts, player );
      co_await post_colonies( ss, ts, player );
      co_return NationTurnState::units{};
    }
    CASE( units ) {
      co_await units_turn( ss, ts, player, o );
      CHECK( o.q.empty() );
      if( !o.skip_eot ) co_return NationTurnState::eot{};
      if( ss.settings.game_options
              .flags[e_game_flag_option::end_of_turn] )
        // As in the OG, this setting means "always stop on end
        // of turn," even we otherwise wouldn't have.
        co_return NationTurnState::eot{};
      co_return NationTurnState::finished{};
    }
    CASE( eot ) {
      SWITCH( co_await end_of_turn( ss, ts, player ) ) {
        CASE( not_done_yet ) {
          co_return NationTurnState::eot{};
        }
        CASE( proceed ) {
          co_return NationTurnState::finished{};
        }
        CASE( return_to_units ) {
          co_return NationTurnState::units{
              .q = { o.first_to_ask } };
        }
        END_CASES;
      }
      SHOULD_NOT_BE_HERE; // for gcc.
    }
    CASE( finished ) {
      // We've allowed the fog removal to accumulate during this
      // player's turn, i.e., as a unit moves, the fog the does
      // regenerate in its wake. This is ok because, even if we
      // were to regenerate the fog in its wake (which is tricky
      // to get right) that fog wouldn't be hiding anything from
      // the player because nothing on those tiles would change
      // until the player finishes its turn and other players
      // move. And if something on those tiles does change, it
      // will be caused by the player (because it is their turn)
      // and so the player will be able to see it anyway. The
      // point here is that there is nothing to gain by regener-
      // ating fog behind a unit as it moves on the map during
      // the course of a turn, and so we avoid doing it because a
      // first attempt revealed that it is very tricky to get
      // right. So what we do is we just let the units move and
      // remove fog, then at the end of the player's turn we re-
      // generate all of the fog on all squares except those
      // within the sighting radius of units and colonies, so by
      // the time the next player (or natives) move, the fog will
      // be as it would have been had to regenerated it in the
      // wake of the units as they moved.
      recompute_fog_for_nation( ss, ts, nation );
      co_return nothing;
    }
    END_CASES;
  }
}

wait<> nation_turn( SS& ss, TS& ts, e_nation nation,
                    NationTurnState& st ) {
  if( !ss.players.players[nation].has_value() ) co_return;
  while( true ) {
    maybe<NationTurnState> const next =
        co_await nation_turn_iter( ss, ts, nation, st );
    if( !next.has_value() ) break;
    st = *next;
  }
}

/****************************************************************
** Turn Processor
*****************************************************************/
// Here we do things that must be done once at the start of each
// full turn cycle but where the player can't save the game until
// they are complete.
void start_of_turn_cycle( SS& ss ) {
  // This will evolve the internal market model state of the
  // processed goods (rum, cigars, cloth, coats). It is done once
  // per turn cycle instead of once per player turn because said
  // model state is shared among all players. That said, the
  // price movement of these goods will happen at the start of
  // each player turn for that respective player.
  if( ss.turn.time_point.turns >
      config_turn.turns_to_wait.market_evolution )
    evolve_group_model_volumes( ss );
}

void reset_units( SS& ss ) {
  refl::enum_map<e_nation, Player const*> players;
  map_all_euro_units( ss.units, [&]( Unit& unit ) {
    UNWRAP_CHECK( player, ss.players.players[unit.nation()] );
    unit.new_turn( player );
  } );

  // TODO: handle native units.
}

// Processes teh current state and returns the next state.
wait<TurnCycle> next_turn_iter( SS& ss, TS& ts ) {
  TurnState& turn  = ss.turn;
  TurnCycle& cycle = ss.turn.cycle;
  SWITCH( cycle ) {
    CASE( not_started ) {
      reset_units( ss );
      start_of_turn_cycle( ss );
      co_return TurnCycle::natives{};
    }
    CASE( natives ) {
      // TODO
      co_return TurnCycle::nation{};
    }
    CASE( nation ) {
      co_await nation_turn( ss, ts, o.nation, o.st );
      auto& ns   = refl::enum_values<e_nation>;
      auto  next = base::find( ns, o.nation ) + 1;
      if( next != ns.end() )
        co_return TurnCycle::nation{ .nation = *next };
      co_return TurnCycle::end_cycle{};
    }
    CASE( end_cycle ) {
      co_await advance_time( ts.gui, turn.time_point );
      if( should_autosave( ss ) ) autosave( ss, ts );
      co_return TurnCycle::finished{};
    }
    CASE( finished ) { SHOULD_NOT_BE_HERE; }
    END_CASES;
  }
}

// Runs through the various phases of a single turn.
wait<> next_turn( SS& ss, TS& ts ) {
  TurnCycle& cycle = ss.turn.cycle;
  ts.planes.land_view().start_new_turn();
  print_bar( '=', "[ Starting Turn ]" );
  while( !cycle.holds<TurnCycle::finished>() )
    cycle = co_await next_turn_iter( ss, ts );
  // The default-constructed cycle represents a new turn where
  // nothing yet has been done. Do this at the end of the cycle
  // so that we don't destroy the turn state after having loaded
  // a saved game.
  cycle = {};
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
// Runs through multiple turns.
wait<> turn_loop( SS& ss, TS& ts ) {
  while( true ) co_await next_turn( ss, ts );
}

} // namespace rn
