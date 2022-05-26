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
#include "co-combinator.hpp"
#include "co-wait.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "cstate.hpp"
#include "game-state.hpp"
#include "gs-units.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-edit.hpp"
#include "map-updater.hpp"
#include "menu.hpp"
#include "old-world-view.hpp"
#include "old-world.hpp"
#include "orders.hpp"
#include "panel.hpp" // FIXME
#include "plane-ctrl.hpp"
#include "plow.hpp"
#include "renderer.hpp" // FIXME: remove
#include "road.hpp"
#include "save-game.hpp"
#include "sound.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "window.hpp"

// Rds
#include "gs-turn.rds.hpp"
#include "turn-impl.rds.hpp"

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

namespace {

/****************************************************************
** Global State
*****************************************************************/
enum class e_menu_actions {
  exit,
  save,
  load,
  revolution,
  old_world_view,
  map_editor,
};

bool                       g_menu_commands_accepted = false;
bool                       g_doing_eot              = false;
co::stream<e_menu_actions> g_menu_actions;

// Globals relevant to end of turn.
namespace eot {

struct next_turn_t {};

using UserInput = base::variant< //
    e_menu_actions,              //
    LandViewPlayerInput_t,       //
    next_turn_t                  //
    >;

} // namespace eot

/****************************************************************
** Save-Game State
*****************************************************************/
NationTurn new_nation_turn_obj( e_nation nat ) {
  return NationTurn{
      .nation       = nat,
      .started      = false,
      .did_colonies = false,
      .did_units    = false, // FIXME: do we need this?
      .units        = {},
  };
}

TurnState new_turn() {
  queue<e_nation> remainder;
  remainder.push( e_nation::english );
  remainder.push( e_nation::french );
  remainder.push( e_nation::dutch );
  remainder.push( e_nation::spanish );
  return TurnState{
      .started   = false,
      .need_eot  = true,
      .nation    = nothing,
      .remainder = std::move( remainder ),
  };
}

/****************************************************************
** Menu Handlers
*****************************************************************/
wait<> menu_save_handler() {
  if( auto res = save_game( 0 ); !res ) {
    co_await ui::message_box(
        "There was a problem saving the game." );
    lg.error( "failed to save game: {}", res.error() );
  } else {
    co_await ui::message_box(
        fmt::format( "Successfully saved game to {}.", res ) );
    lg.info( "saved game to {}.", res );
  }
}

wait<> menu_revolution_handler() {
  e_revolution_confirmation answer =
      co_await ui::select_box_enum<e_revolution_confirmation>(
          "Declare Revolution?" );
  co_await ui::message_box( "You selected: {}", answer );
}

wait<> menu_old_world_view_handler() {
  co_await show_old_world_view();
}

wait<bool> proceed_to_leave_game() { co_return true; }

wait<> menu_exit_handler() {
  if( co_await proceed_to_leave_game() )
    throw game_quit_interrupt{};
}

wait<> menu_load_handler() {
  if( co_await proceed_to_leave_game() )
    throw game_load_interrupt{};
}

wait<> menu_map_editor_handler() {
  // FIXME: hack
  rr::Renderer& renderer =
      global_renderer_use_only_when_needed();
  MapUpdater map_updater( GameState::terrain(), renderer );
  // Need to co_await so that the map_updater stays alive.
  co_await map_editor( map_updater );
}

#define DEFAULT_TURN_MENU_ITEM_HANDLER( item )             \
  MENU_ITEM_HANDLER(                                       \
      item,                                                \
      [] { g_menu_actions.send( e_menu_actions::item ); }, \
      [] { return g_menu_commands_accepted; } )

DEFAULT_TURN_MENU_ITEM_HANDLER( exit );
DEFAULT_TURN_MENU_ITEM_HANDLER( save );
DEFAULT_TURN_MENU_ITEM_HANDLER( load );
DEFAULT_TURN_MENU_ITEM_HANDLER( revolution );
DEFAULT_TURN_MENU_ITEM_HANDLER( old_world_view );
DEFAULT_TURN_MENU_ITEM_HANDLER( map_editor );

#define CASE_MENU_HANDLER( item ) \
  case e_menu_actions::item: return menu_##item##_handler();

wait<> handle_menu_item( e_menu_actions action ) {
  switch( action ) {
    CASE_MENU_HANDLER( exit );
    CASE_MENU_HANDLER( save );
    CASE_MENU_HANDLER( load );
    CASE_MENU_HANDLER( revolution );
    CASE_MENU_HANDLER( old_world_view );
    CASE_MENU_HANDLER( map_editor );
  }
}

wait<e_menu_actions> wait_for_menu_selection() {
  SCOPED_SET_AND_CHANGE( g_menu_commands_accepted, true, false );
  co_return co_await g_menu_actions.next();
}

/****************************************************************
** Helpers
*****************************************************************/
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
void finish_turn( UnitId id ) {
  unit_from_id( id ).forfeight_mv_points();
}

[[nodiscard]] bool finished_turn( UnitId id ) {
  return unit_from_id( id ).mv_pts_exhausted();
}

bool should_remove_unit_from_queue( UnitId id ) {
  Unit& unit = unit_from_id( id );
  if( finished_turn( id ) ) return true;
  switch( unit.orders() ) {
    case e_unit_orders::fortified: return true;
    case e_unit_orders::sentry: return true;
    case e_unit_orders::road: return false;
    case e_unit_orders::plow: return false;
    case e_unit_orders::none: return false;
  }
}

// See if `unit` needs to be unsentry'd due to surrounding for-
// eign units.
void try_unsentry_unit( Unit& unit ) {
  if( unit.orders() != e_unit_orders::sentry ) return;
  // Don't use the "indirect" version here because we don't want
  // to e.g. wake up units that are sentry'd on ships.
  maybe<Coord> loc = coord_for_unit( unit.id() );
  if( !loc.has_value() ) return;
  for( UnitId id : surrounding_units( *loc ) ) {
    if( unit_from_id( id ).nation() != unit.nation() ) {
      unit.clear_orders();
      return;
    }
  }
}

// See if any foreign units in the vicinity of src_id need to be
// unsentry'd.
void unsentry_surroundings( UnitId src_id ) {
  Unit& src_unit = unit_from_id( src_id );
  Coord src_loc  = coord_for_unit_indirect_or_die( src_id );
  for( UnitId id : surrounding_units( src_loc ) ) {
    Unit& unit = unit_from_id( id );
    if( unit.orders() != e_unit_orders::sentry ) continue;
    if( unit.nation() == src_unit.nation() ) continue;
    unit.clear_orders();
  }
}

/****************************************************************
** Processing Player Input (End of Turn).
*****************************************************************/
namespace eot {

wait<> process_player_input( e_menu_actions action,
                             IMapUpdater& ) {
  // In the future we might need to put logic here that is spe-
  // cific to the end-of-turn, but for now this is sufficient.
  return handle_menu_item( action );
}

wait<> process_player_input( LandViewPlayerInput_t const& input,
                             IMapUpdater& map_updater ) {
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::colony: {
      co_await show_colony_view( input.get<colony>().id,
                                 map_updater );
      break;
    }
    default: break;
  }
}

wait<> process_player_input( next_turn_t, IMapUpdater& ) {
  lg.debug( "end of turn button clicked." );
  co_return;
}

wait<> process_inputs( IMapUpdater& map_updater ) {
  landview_reset_input_buffers();
  while( true ) {
    auto wait_for_button = co::fmap(
        [] λ( next_turn_t{} ), wait_for_eot_button_click() );
    // The reason that we want to use co::first here instead of
    // interleaving the three streams is because as soon as one
    // becomes ready (and we start processing it) we want all the
    // others to be automatically be cancelled, which will have
    // the effect of disabling further input on them (e.g., dis-
    // abling menu items), which is what we want for a good user
    // experience.
    UserInput command = co_await co::first( //
        wait_for_menu_selection(),          //
        landview_eot_get_next_input(),      //
        std::move( wait_for_button )        //
    );
    co_await rn::visit(
        command, LC( process_player_input( _, map_updater ) ) );
    if( command.holds<next_turn_t>() ||
        command.get_if<LandViewPlayerInput_t>()
            .fmap( L( _.template holds<
                      LandViewPlayerInput::next_turn>() ) )
            .is_value_truish() )
      co_return;
  }
}

} // namespace eot

wait<> end_of_turn( IMapUpdater& map_updater ) {
  SCOPED_SET_AND_CHANGE( g_doing_eot, true, false );
  return eot::process_inputs( map_updater );
}

/****************************************************************
** Processing Player Input (During Turn).
*****************************************************************/
wait<> process_player_input( UnitId, e_menu_actions action,
                             IMapUpdater&, IGui& ) {
  // In the future we might need to put logic here that is spe-
  // cific to the mid-turn scenario, but for now this is suffi-
  // cient.
  return handle_menu_item( action );
}

wait<> process_player_input( UnitId                       id,
                             LandViewPlayerInput_t const& input,
                             IMapUpdater& map_updater,
                             IGui&        gui ) {
  CHECK( GameState::turn().nation );
  auto& st = *GameState::turn().nation;
  auto& q  = st.units;
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::next_turn: {
      // The land view should never send us a 'next turn' command
      // when we are not at the end of a turn.
      SHOULD_NOT_BE_HERE;
    }
    case e::colony: {
      co_await show_colony_view( input.get<colony>().id,
                                 map_updater );
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
        unit_from_id( id ).forfeight_mv_points();
        break;
      }

      unique_ptr<OrdersHandler> handler =
          orders_handler( id, orders, &map_updater, gui );
      CHECK( handler );
      Coord old_loc    = coord_for_unit_indirect_or_die( id );
      auto  run_result = co_await handler->run();

      // If we suspended at some point during the above process
      // (apart from animations), then that probably means that
      // the user was presented with a prompt, in which case it
      // seems like a good idea to clear the input buffers for an
      // intuitive user experience.
      if( run_result.suspended ) {
        lg.debug( "clearing land-view input buffers." );
        landview_reset_input_buffers();
      }
      if( !run_result.order_was_run ) break;
      // !! The unit may no longer exist at this point, e.g. if
      // they were disbanded or if they lost a battle to the na-
      // tives.

      // If the unit still exists, check if it has moved squares
      // in any fashion, and if so then wake up any foreign sen-
      // try'd neighbors.
      if( unit_exists( id ) ) {
        maybe<Coord> new_loc = coord_for_unit_indirect( id );
        if( new_loc && *new_loc != old_loc )
          unsentry_surroundings( id );
      }

      for( auto id : handler->units_to_prioritize() )
        prioritize_unit( q, id );
      break;
    }
    case e::prioritize: {
      auto& val = input.get<prioritize>();
      // Move some units to the front of the queue.
      auto prioritize = val.units;
      erase_if( prioritize, finished_turn );
      auto orig_size = val.units.size();
      auto curr_size = prioritize.size();
      CHECK( curr_size <= orig_size );
      if( curr_size == 0 )
        co_await ui::message_box(
            "The selected unit(s) have already moved this "
            "turn." );
      else if( curr_size < orig_size )
        co_await ui::message_box(
            "Some of the selected units have already moved this "
            "turn." );
      for( UnitId id_to_add : prioritize )
        prioritize_unit( q, id_to_add );
      break;
    }
  }
}

wait<LandViewPlayerInput_t> landview_player_input( UnitId id ) {
  LandViewPlayerInput_t response;
  if( auto maybe_orders = pop_unit_orders( id ) ) {
    response = LandViewPlayerInput::give_orders{
        .orders = *maybe_orders };
  } else {
    lg.debug( "asking orders for: {}", debug_string( id ) );
    GameState::turn().need_eot = false;
    response = co_await landview_get_next_input( id );
  }
  co_return response;
}

wait<> query_unit_input( UnitId id, IMapUpdater& map_updater,
                         IGui& gui ) {
  auto command = co_await co::first(
      wait_for_menu_selection(), landview_player_input( id ) );
  co_await overload_visit( command, [&]( auto const& action ) {
    return process_player_input( id, action, map_updater, gui );
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
wait<bool> advance_unit( IMapUpdater& map_updater, UnitId id ) {
  CHECK( !should_remove_unit_from_queue( id ) );
  Unit& unit = GameState::units().unit_for( id );

  if( unit.orders() == e_unit_orders::road ) {
    perform_road_work( GameState::units(), GameState::terrain(),
                       map_updater, unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders() == e_unit_orders::none );
      co_await landview_ensure_visible( id );
      co_await ui::message_box_basic(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( unit.orders() != e_unit_orders::road );
  }

  if( unit.orders() == e_unit_orders::plow ) {
    perform_plow_work( GameState::units(), GameState::terrain(),
                       map_updater, unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders() == e_unit_orders::none );
      co_await landview_ensure_visible( id );
      // TODO: if we were clearing a forest then we should pick a
      // colony in the vicinity and add a certain amount of
      // lumber to it (see strategy guide for formula) and give a
      // message to the user.
      co_await ui::message_box_basic(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return ( unit.orders() != e_unit_orders::plow );
  }

  if( is_unit_in_port( id ) ) {
    finish_turn( id );
    co_return false; // do not ask for orders.
  }

  // If it is a ship on the high seas then advance it. If it has
  // arrived in the old world then jump to the old world screen.
  if( is_unit_inbound( id ) || is_unit_outbound( id ) ) {
    e_high_seas_result res =
        advance_unit_on_high_seas( id, map_updater );
    switch( res ) {
      case e_high_seas_result::still_traveling:
        finish_turn( id );
        co_return false; // do not ask for orders.
      case e_high_seas_result::arrived_in_new_world:
        unsentry_surroundings( id );
        co_return true; // needs to ask for orders.
      case e_high_seas_result::arrived_in_old_world: {
        finish_turn( id );
        ui::e_confirm confirmed = co_await ui::yes_no(
            "Your excellency, our {} has arrived in the old "
            "world.  Go to port?",
            unit_from_id( id ).desc().name );
        if( confirmed == ui::e_confirm::yes ) {
          old_world_view_set_selected_unit( id );
          co_await show_old_world_view();
        }
        co_return false; // do not ask for orders.
      }
    }
  }

  if( !is_unit_on_map_indirect( id ) ) {
    finish_turn( id );
    co_return false;
  }

  // Unit needs to ask for orders.
  co_return true;
}

wait<> units_turn_one_pass( IMapUpdater& map_updater, IGui& gui,
                            deque<UnitId>& q ) {
  while( !q.empty() ) {
    // lg.trace( "q: {}", q );
    UnitId id = q.front();
    // We need this check because units can be added into the
    // queue in this loop by user input. Also, the very first
    // check that we must do needs to be to check if the unit
    // still exists, which it might not if e.g. it was disbanded.
    if( !unit_exists( id ) ||
        should_remove_unit_from_queue( id ) ) {
      q.pop_front();
      continue;
    }

    bool should_ask = co_await advance_unit( map_updater, id );
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
    co_await query_unit_input( id, map_updater, gui );
    // !! The unit may no longer exist at this point, e.g. if
    // they were disbanded or if they lost a battle to the na-
    // tives.
  }
}

wait<> units_turn( IMapUpdater& map_updater, IGui& gui ) {
  CHECK( GameState::turn().nation );
  auto& st = *GameState::turn().nation;
  auto& q  = st.units;

  // Unsentry any units that are sentried but have foreign units
  // in an adjacent square.
  map_units( st.nation, try_unsentry_unit );

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
    co_await units_turn_one_pass( map_updater, gui, q );
    CHECK( q.empty() );
    // Refill the queue.
    auto units = units_all( st.nation );
    util::sort_by_key( units, []( auto id ) { return id._; } );
    erase_if( units, should_remove_unit_from_queue );
    if( units.empty() ) co_return;
    for( UnitId id : units ) q.push_back( id );
  }
}

/****************************************************************
** Per-Colony Turn Processor
*****************************************************************/
wait<> colonies_turn( IMapUpdater& map_updater, IGui& gui ) {
  CHECK( GameState::turn().nation );
  auto& st = *GameState::turn().nation;
  lg.info( "processing colonies for the {}.", st.nation );
  queue<ColonyId> colonies;
  for( ColonyId colony_id : colonies_all( st.nation ) )
    colonies.push( colony_id );
  while( !colonies.empty() ) {
    ColonyId colony_id = colonies.front();
    colonies.pop();
    co_await evolve_colony_one_turn( colony_id, map_updater,
                                     gui );
  }
}

/****************************************************************
** Per-Nation Turn Processor
*****************************************************************/
wait<> nation_turn( IMapUpdater& map_updater, IGui& gui ) {
  CHECK( GameState::turn().nation );
  auto& st = *GameState::turn().nation;

  // Starting.
  if( !st.started ) {
    print_bar( '-', fmt::format( "[ {} ]", st.nation ) );
    st.started = true;
  }

  // Colonies.
  if( !st.did_colonies ) {
    co_await colonies_turn( map_updater, gui );
    st.did_colonies = true;
  }

  if( !st.did_units ) {
    co_await units_turn( map_updater, gui );
    st.did_units = true;
  }
  CHECK( st.units.empty() );
}

/****************************************************************
** Turn Processor
*****************************************************************/
wait<> next_turn_impl( IMapUpdater& map_updater, IGui& gui ) {
  landview_start_new_turn();
  auto& st = GameState::turn();

  // Starting.
  if( !st.started ) {
    print_bar( '=', "[ Starting Turn ]" );
    map_units( []( Unit& unit ) { unit.new_turn(); } );
    st         = new_turn();
    st.started = true;
  }

  // Body.
  if( st.nation.has_value() ) {
    co_await nation_turn( map_updater, gui );
    st.nation.reset();
  }

  while( !st.remainder.empty() ) {
    st.nation = new_nation_turn_obj( st.remainder.front() );
    st.remainder.pop();
    co_await nation_turn( map_updater, gui );
    st.nation.reset();
  }

  // Ending.
  if( st.need_eot ) co_await end_of_turn( map_updater );

  st = new_turn();
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
wait<> next_turn( IMapUpdater& map_updater, IGui& gui ) {
  ScopedPlanePush pusher( e_plane_config::land_view );
  co_await next_turn_impl( map_updater, gui );
}

} // namespace rn
