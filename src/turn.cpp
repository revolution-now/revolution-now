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

// Revolution Now
#include "cheat.hpp"
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "fathers.hpp"
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
#include "orders.hpp"
#include "panel.hpp"
#include "plane-stack.hpp"
#include "plane.hpp"
#include "plow.hpp"
#include "road.hpp"
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
#include "config/turn.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/colonies.hpp"
#include "ss/land-view.rds.hpp"
#include "ss/players.hpp"
#include "ss/ref.hpp"
#include "ss/turn.rds.hpp"
#include "ss/units.hpp"

// Rds
#include "menu.rds.hpp"

// refl
#include "refl/to-str.hpp"

// base
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
bool g_doing_eot = false;

// Globals relevant to end of turn.
namespace eot {

struct next_turn_t {};

using UserInput = base::variant< //
    e_menu_item,                 //
    LandViewPlayerInput_t,       //
    next_turn_t                  //
    >;

} // namespace eot

/****************************************************************
** Save-Game State
*****************************************************************/
NationTurnState new_nation_turn_obj( e_nation nat ) {
  return NationTurnState{
      .nation       = nat,
      .started      = false,
      .did_colonies = false,
      .did_units    = false, // FIXME: do we need this?
      .units        = {},
      .need_eot     = true,
  };
}

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

void reset_turn_obj( PlayersState const& players_state,
                     TurnState&          st ) {
  queue<e_nation> remainder;
  for( e_nation nation : refl::enum_values<e_nation> )
    if( players_state.players[nation].has_value() )
      remainder.push( nation );
  st.started   = false;
  st.nation    = nothing;
  st.remainder = std::move( remainder );
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
  switch( unit.orders() ) {
    case e_unit_orders::fortified:
      return true;
    case e_unit_orders::fortifying:
      return true;
    case e_unit_orders::sentry:
      return true;
    case e_unit_orders::road:
      return false;
    case e_unit_orders::plow:
      return false;
    case e_unit_orders::none:
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
  if( unit.orders() != e_unit_orders::sentry ) return;
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

void fortify_units( Unit& unit ) {
  if( unit.orders() != e_unit_orders::fortifying ) return;
  unit.fortify();
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
    if( unit.orders() != e_unit_orders::sentry ) continue;
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

/****************************************************************
** Menu Handlers
*****************************************************************/
wait<> menu_handler( Planes& planes, SS& ss, TS& ts,
                     Player& player, e_menu_item item ) {
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
      co_await show_harbor_view( planes, ss, ts, player,
                                 /*selected_unit=*/nothing );
      break;
    }
    case e_menu_item::cheat_map_editor: {
      // Need to co_await so that the map_updater stays alive.
      co_await run_map_editor( planes, ss, ts );
      break;
    }
    case e_menu_item::cheat_edit_fathers: {
      co_await cheat_edit_fathers( planes, ss, ts, player );
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

wait<> process_player_input( e_menu_item item, Planes& planes,
                             SS& ss, TS& ts, Player& player ) {
  // In the future we might need to put logic here that is spe-
  // cific to the end-of-turn, but for now this is sufficient.
  return menu_handler( planes, ss, ts, player, item );
}

wait<> process_player_input( LandViewPlayerInput_t const& input,
                             Planes& planes, SS& ss, TS& ts,
                             Player& player ) {
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::colony: {
      e_colony_abandoned const abandoned =
          co_await show_colony_view(
              planes, ss, ts, player,
              ss.colonies.colony_for( input.get<colony>().id ) );
      if( abandoned == e_colony_abandoned::yes )
        // Nothing really special to do here.
        co_return;
      break;
    }
    case e::european_status: {
      co_await show_harbor_view( planes, ss, ts, player,
                                 /*selected_unit=*/nothing );
      break;
    }
    case e::next_turn:
      // This one is relevant but handled in the calling func-
      // tion.
      break;
    case e::exit:
      co_await proceed_to_exit( ss, ts );
      break;
    case e::give_orders:
    case e::prioritize: //
      break;
  }
}

wait<> process_player_input( next_turn_t, Planes&, SS&, TS&,
                             Player& ) {
  lg.debug( "end of turn button clicked." );
  co_return;
}

wait<> process_inputs( Planes& planes, SS& ss, TS& ts,
                       Player& player ) {
  while( true ) {
    auto wait_for_button =
        co::fmap( [] λ( next_turn_t{} ),
                  planes.panel().wait_for_eot_button_click() );
    // The reason that we want to use co::first here instead of
    // interleaving the three streams is because as soon as one
    // becomes ready (and we start processing it) we want all the
    // others to be automatically be cancelled, which will have
    // the effect of disabling further input on them (e.g., dis-
    // abling menu items), which is what we want for a good user
    // experience.
    UserInput command = co_await co::first(       //
        wait_for_menu_selection( planes.menu() ), //
        planes.land_view().eot_get_next_input(),  //
        std::move( wait_for_button )              //
    );
    co_await rn::visit(
        command, LC( process_player_input( _, planes, ss, ts,
                                           player ) ) );
    if( command.holds<next_turn_t>() ||
        command.get_if<LandViewPlayerInput_t>()
            .fmap( L( _.template holds<
                      LandViewPlayerInput::next_turn>() ) )
            .is_value_truish() )
      co_return;
  }
}

} // namespace eot

wait<> end_of_turn( Planes& planes, SS& ss, TS& ts,
                    Player& player ) {
  SCOPED_SET_AND_CHANGE( g_doing_eot, true, false );
  return eot::process_inputs( planes, ss, ts, player );
}

/****************************************************************
** Processing Player Input (During Turn).
*****************************************************************/
wait<> process_player_input( UnitId, e_menu_item item,
                             Planes& planes, SS& ss, TS& ts,
                             Player& player, NationTurnState& ) {
  // In the future we might need to put logic here that is spe-
  // cific to the mid-turn scenario, but for now this is suffi-
  // cient.
  return menu_handler( planes, ss, ts, player, item );
}

wait<> process_player_input( UnitId                       id,
                             LandViewPlayerInput_t const& input,
                             Planes& planes, SS& ss, TS& ts,
                             Player&          player,
                             NationTurnState& nat_turn_st ) {
  auto& st = nat_turn_st;
  auto& q  = st.units;
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
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
          co_await show_colony_view(
              planes, ss, ts, player,
              ss.colonies.colony_for( input.get<colony>().id ) );
      if( abandoned == e_colony_abandoned::yes )
        // Nothing really special to do here.
        co_return;
      break;
    }
    case e::european_status: {
      co_await show_harbor_view( planes, ss, ts, player,
                                 /*selected_unit=*/nothing );
      break;
    }
    // We have some orders for the current unit.
    case e::give_orders: {
      auto& orders = input.get<give_orders>().orders;
      if( orders.holds<orders::wait>() ) {
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
      if( orders.holds<orders::forfeight>() ) {
        ss.units.unit_for( id ).forfeight_mv_points();
        break;
      }

      co_await planes.land_view().ensure_visible_unit( id );
      unique_ptr<OrdersHandler> handler =
          orders_handler( planes, ss, ts, player, id, orders );
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
      auto& val = input.get<prioritize>();
      // Move some units to the front of the queue.
      auto prioritize = val.units;
      erase_if( prioritize, [&]( UnitId id ) {
        return finished_turn( ss.units.unit_for( id ) );
      } );
      auto orig_size = val.units.size();
      auto curr_size = prioritize.size();
      CHECK( curr_size <= orig_size );
      if( curr_size == 0 )
        co_await ts.gui.message_box(
            "The selected unit(s) have already moved this "
            "turn." );
      else if( curr_size < orig_size )
        co_await ts.gui.message_box(
            "Some of the selected units have already moved this "
            "turn." );
      for( UnitId id_to_add : prioritize )
        prioritize_unit( q, id_to_add );
      break;
    }
  }
}

wait<LandViewPlayerInput_t> landview_player_input(
    ILandViewPlane&  land_view_plane,
    NationTurnState& nat_turn_st, UnitsState const& units_state,
    UnitId id ) {
  LandViewPlayerInput_t response;
  if( auto maybe_orders = pop_unit_orders( id ) ) {
    response = LandViewPlayerInput::give_orders{
        .orders = *maybe_orders };
  } else {
    lg.debug( "asking orders for: {}",
              debug_string( units_state, id ) );
    nat_turn_st.need_eot = false;
    response = co_await land_view_plane.get_next_input( id );
  }
  co_return response;
}

wait<> query_unit_input( UnitId id, Planes& planes, SS& ss,
                         TS& ts, Player& player,
                         NationTurnState& nat_turn_st ) {
  auto command = co_await co::first(
      wait_for_menu_selection( planes.menu() ),
      landview_player_input( planes.land_view(), nat_turn_st,
                             ss.units, id ) );
  co_await overload_visit( command, [&]( auto const& action ) {
    return process_player_input( id, action, planes, ss, ts,
                                 player, nat_turn_st );
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
wait<bool> advance_unit( Planes& planes, SS& ss, TS& ts,
                         Player& player, UnitId id ) {
  Unit& unit = ss.units.unit_for( id );
  CHECK( !should_remove_unit_from_queue( unit ) );

  if( unit.orders() == e_unit_orders::road ) {
    perform_road_work( ss.units, ss.terrain, as_const( player ),
                       ts.map_updater, unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders() == e_unit_orders::none );
      co_await planes.land_view().ensure_visible_unit( id );
      co_await ts.gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( unit.orders() != e_unit_orders::road );
  }

  if( unit.orders() == e_unit_orders::plow ) {
    PlowResult_t const plow_result = perform_plow_work(
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
      CHECK( unit.orders() == e_unit_orders::none );
      co_await planes.land_view().ensure_visible_unit( id );
      co_await ts.gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( unit.orders() != e_unit_orders::plow );
  }

  if( is_unit_in_port( ss.units, id ) ) {
    finish_turn( unit );
    co_return false; // do not ask for orders.
  }

  // If it is a ship on the high seas then advance it. If it has
  // arrived in the old world then jump to the old world screen.
  if( is_unit_inbound( ss.units, id ) ||
      is_unit_outbound( ss.units, id ) ) {
    e_high_seas_result res = advance_unit_on_high_seas(
        ss.terrain, ss.units, player, id );
    switch( res ) {
      case e_high_seas_result::still_traveling:
        finish_turn( unit );
        co_return false; // do not ask for orders.
      case e_high_seas_result::arrived_in_new_world: {
        lg.debug( "unit has arrived in new world." );
        maybe<Coord> const dst_coord =
            find_new_world_arrival_square(
                ss, player,
                ss.units.harbor_view_state_of( id )
                    .sailed_from );
        if( !dst_coord.has_value() ) {
          co_await ts.gui.message_box(
              "Unfortunately, while our [{}] has arrived "
              "in the new world, there are no appropriate water "
              "squares on which to place it.  We will try again "
              "next turn.",
              ss.units.unit_for( id ).desc().name );
          finish_turn( unit );
          break;
        }
        ss.units.unit_for( id ).clear_orders();
        maybe<UnitDeleted> unit_deleted =
            co_await unit_to_map_square( ss, ts, id,
                                         *dst_coord );
        // This is not required, but it is for a good player ex-
        // perience. If there are more ships still in port then
        // select one of them, because ideally if there are ships
        // in port then when the player goes to the harbor view,
        // one of them should always be selected.
        update_harbor_selected_unit( ss.units, player );
        // There are no LCR tiles on water squares.
        CHECK( !unit_deleted.has_value() );
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
        co_await show_harbor_view( planes, ss, ts, player, id );
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

wait<> units_turn_one_pass( Planes& planes, SS& ss, TS& ts,
                            Player&          player,
                            NationTurnState& nat_turn_st,
                            deque<UnitId>&   q ) {
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
        co_await advance_unit( planes, ss, ts, player, id );
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
    co_await query_unit_input( id, planes, ss, ts, player,
                               nat_turn_st );
    // !! The unit may no longer exist at this point, e.g. if
    // they were disbanded or if they lost a battle to the na-
    // tives.
  }
}

wait<> units_turn( Planes& planes, SS& ss, TS& ts,
                   Player&          player,
                   NationTurnState& nat_turn_st ) {
  auto& st = nat_turn_st;
  auto& q  = st.units;

  // Unsentry any units that are sentried but have foreign units
  // in an adjacent square. FIXME: move this to the the function
  // that is called to put a unit on a new map square.
  map_active_euro_units( ss.units, st.nation, [&]( Unit& unit ) {
    return try_unsentry_unit( ss, unit );
  } );

  // Any units that are in the "fortifying" state at the start of
  // their turn get "promoted" for the "fortified" status, which
  // means they are actually fortified and get those benefits. We
  // only do this to the active units, i.e. the ones that still
  // have movement points. The reason for this is so that if we
  // save the game just after hitting 'F' on a unit, then reload
  // it, we won't (incorrectly) try to transition the unit to the
  // "fortified" state. Instead we will (correctly) do so on the
  // following turn.
  //
  // FIXME: it is probably better put this into a dedicated func-
  // tion that will run only once at the start of each turn, re-
  // gardless of save/load patterns. Then we could just map over
  // all of this nation's units without worrying about whether
  // they are active or not (and also by the way it is not good
  // to begin with that we have to rely on the movement points
  // indicator for this because it ideally should be an arbitrary
  // game logic decision as to whether a unit's movement points
  // get consumed when they initiate a fortify). Maybe we could
  // go even further an implement a general mechanism for holding
  // the state of the turn in serialized form so that we don't
  // rely on a bunch of ad hoc bool flags.
  map_active_euro_units( ss.units, st.nation, fortify_units );

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
    co_await units_turn_one_pass( planes, ss, ts, player,
                                  nat_turn_st, q );
    CHECK( q.empty() );
    // Refill the queue.
    vector<UnitId> units = euro_units_all( ss.units, st.nation );
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
wait<> colonies_turn( Planes& planes, SS& ss, TS& ts,
                      Player& player ) {
  co_await evolve_colonies_for_player( planes, ss, ts, player );
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

void set_nation_map_visibility( Planes& planes, SS& ss, TS& ts,
                                e_nation nation ) {
  if( maybe<MapRevealed_t const&> revealed =
          ss.land_view.map_revealed;
      revealed.has_value() &&
      revealed->holds<MapRevealed::entire>() ) {
    // We have "entire" map visibility, and we're going to keep
    // it that way, but we should call this anyway just to make
    // sure this is what is currently in effect.
    set_map_visibility( planes, ss, ts,
                        MapRevealed_t{ MapRevealed::entire{} },
                        /*default_nation=*/nothing );
    return;
  }
  // At this point the map reveal status is not "entire map," and
  // so since we're about to do the turn of a player, whatever
  // player it's currently set to we're going to override, be-
  // cause it wouldn't really make sense to e.g. start blinking
  // units of another nation when they are not visible.
  set_map_visibility(
      planes, ss, ts,
      MapRevealed_t{ MapRevealed::nation{ .nation = nation } },
      /*default_nation=*/nothing );
}

wait<> nation_turn( Planes& planes, SS& ss, TS& ts,
                    NationTurnState& nat_turn_st ) {
  auto& st = nat_turn_st;

  UNWRAP_CHECK( player, ss.players.players[st.nation] );

  if( !player.human ) co_return; // TODO: Until we have AI.

  // `visibility` determines from whose point of view the map is
  // drawn with respect to which tiles are hidden.
  set_nation_map_visibility( planes, ss, ts, st.nation );

  // Starting.
  if( !st.started ) {
    print_bar( '-', fmt::format( "[ {} ]", st.nation ) );
    co_await nation_start_of_turn( ss, ts, player );
    st.started = true;
  }

  // Colonies.
  if( !st.did_colonies ) {
    co_await colonies_turn( planes, ss, ts, player );
    co_await post_colonies( ss, ts, player );
    st.did_colonies = true;
  }

  if( !st.did_units ) {
    co_await units_turn( planes, ss, ts, player, nat_turn_st );
    st.did_units = true;
  }
  CHECK( st.units.empty() );

  // Ending.
  if( st.need_eot )
    // FIXME: in the original game, a unit that hasn't moved this
    // turn (say, one that has remained fortified) can be acti-
    // vated and moved during the end of turn. Then after it
    // moves the turn will end with no end-of-turn (although
    // other units can be activated while the first one is blink-
    // ing, in which case they can move as well). This should not
    // be to difficult to implement (and might actually simplify
    // some of the land-view logic in eliminating the distinction
    // between end of turn and mid-turn in unit selection); any
    // units that still have movement points remaining can be se-
    // lected, then we just go back into the units_turn to
    // process them. We can do that here just once, because we
    // know that after that process is over a) the turn will end,
    // and b) we won't need an end-of-turn state, because the
    // user has already had one.
    co_await end_of_turn( planes, ss, ts, player );
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

wait<> next_turn( Planes& planes, SS& ss, TS& ts ) {
  planes.land_view().start_new_turn();
  auto& st = ss.turn;

  // Starting.
  if( !st.started ) {
    print_bar( '=', "[ Starting Turn ]" );
    reset_units( ss );
    reset_turn_obj( ss.players, st );
    start_of_turn_cycle( ss );
    st.started = true;
  }

  // Body.
  if( st.nation.has_value() ) {
    co_await nation_turn( planes, ss, ts, *st.nation );
    st.nation.reset();
  }

  while( !st.remainder.empty() ) {
    st.nation = new_nation_turn_obj( st.remainder.front() );
    st.remainder.pop();
    co_await nation_turn( planes, ss, ts, *st.nation );
    st.nation.reset();
  }

  reset_turn_obj( ss.players, st );
  co_await advance_time( ts.gui, st.time_point );

  // Autosave.
  if( should_autosave( st.time_point.turns ) )
    autosave( ss, ts );
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
wait<> turn_loop( Planes& planes, SS& ss, TS& ts ) {
  while( true ) co_await next_turn( planes, ss, ts );
}

} // namespace rn
