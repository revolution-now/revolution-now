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
#include "co-waitable.hpp"
#include "colony-mgr.hpp"
#include "colony-view.hpp"
#include "cstate.hpp"
#include "fb.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "menu.hpp"
#include "old-world-view.hpp"
#include "old-world.hpp"
#include "orders.hpp"
#include "panel.hpp" // FIXME
#include "plane-ctrl.hpp"
#include "save-game.hpp"
#include "sg-macros.hpp"
#include "sound.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "viewport.hpp"
#include "window.hpp"

// base
#include "base/lambda.hpp"
#include "base/scope-exit.hpp"

// Rds
#include "rds/turn-impl.hpp"

// Flatbuffers
#include "fb/sg-turn_generated.h"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <algorithm>
#include <deque>
#include <queue>

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Turn );

namespace {

/****************************************************************
** Global State
*****************************************************************/
enum class e_menu_actions {
  exit,
  save,
  load,
  revolution,
  old_world_view
};

bool                       g_menu_commands_accepted = false;
bool                       g_doing_eot              = false;
co::stream<e_menu_actions> g_menu_actions;

// Globals relevant to end of turn.
namespace eot {

struct button_click_t {};

using UserInput = base::variant< //
    e_menu_actions,              //
    LandViewPlayerInput_t,       //
    button_click_t               //
    >;

} // namespace eot

/****************************************************************
** Save-Game State
*****************************************************************/
struct NationState {
  NationState() = default;
  NationState( e_nation nat ) : NationState() { nation = nat; }
  valid_deserial_t check_invariants_safe() { return valid; }

  bool operator==( NationState const& ) const = default;

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, NationState,
  ( e_nation,      nation       ),
  ( bool,          started      ),
  ( bool,          did_colonies ),
  ( bool,          did_units    ), // FIXME: do we need this?
  ( deque<UnitId>, units        ));
  // clang-format on
};

struct TurnState {
  TurnState() = default;

  void new_turn() {
    started   = false;
    need_eot  = true;
    nation    = nothing;
    remainder = {};
    remainder.push( e_nation::english );
    remainder.push( e_nation::french );
    remainder.push( e_nation::dutch );
    remainder.push( e_nation::spanish );
  }

  bool operator==( TurnState const& ) const = default;
  valid_deserial_t check_invariants_safe() { return valid; }

  // clang-format off
  SERIALIZABLE_TABLE_MEMBERS( fb, TurnState,
  ( bool,                 started   ),
  ( bool,                 need_eot  ),
  ( maybe<NationState>,   nation    ),
  ( queue<e_nation>,      remainder ));
  // clang-format on
};

struct SAVEGAME_STRUCT( Turn ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( Turn,
  ( TurnState, turn ));
  // clang-format on

 public:
  // Fields that are derived from the serialized fields.

 private:
  SAVEGAME_FRIENDS( Turn );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    return valid;
  }
  // Called after all modules are deserialized.
  SAVEGAME_VALIDATE() { return valid; }
};
SAVEGAME_IMPL( Turn );

/****************************************************************
** Menu Handlers
*****************************************************************/
waitable<> menu_save_handler() {
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

waitable<> menu_revolution_handler() {
  e_revolution_confirmation answer =
      co_await ui::select_box_enum<e_revolution_confirmation>(
          "Declare Revolution?" );
  co_await ui::message_box( "You selected: {}", answer );
}

waitable<> menu_old_world_view_handler() {
  co_await show_old_world_view();
}

waitable<bool> proceed_to_leave_game() { co_return true; }

waitable<> menu_exit_handler() {
  if( co_await proceed_to_leave_game() )
    throw game_quit_interrupt{};
}

waitable<> menu_load_handler() {
  if( co_await proceed_to_leave_game() )
    throw game_load_interrupt{};
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

#define CASE_MENU_HANDLER( item ) \
  case e_menu_actions::item: return menu_##item##_handler();

waitable<> handle_menu_item( e_menu_actions action ) {
  switch( action ) {
    CASE_MENU_HANDLER( exit );
    CASE_MENU_HANDLER( save );
    CASE_MENU_HANDLER( load );
    CASE_MENU_HANDLER( revolution );
    CASE_MENU_HANDLER( old_world_view );
  }
}

waitable<e_menu_actions> wait_for_menu_selection() {
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
  switch( unit.orders() ) {
    case e_unit_orders::fortified: return true;
    case e_unit_orders::sentry: return true;
    case e_unit_orders::none: break;
  }
  if( finished_turn( id ) ) return true;
  return false;
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

waitable<> process_player_input( e_menu_actions action ) {
  // In the future we might need to put logic here that is spe-
  // cific to the end-of-turn, but for now this is sufficient.
  return handle_menu_item( action );
}

waitable<> process_player_input(
    LandViewPlayerInput_t const& input ) {
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::colony: {
      co_await show_colony_view( input.get<colony>().id );
      break;
    }
    default: break;
  }
}

waitable<> process_player_input( button_click_t ) {
  lg.debug( "end of turn button clicked." );
  co_return;
}

waitable<> process_inputs() {
  landview_reset_input_buffers();
  while( true ) {
    auto wait_for_button = co::fmap(
        [] λ( button_click_t{} ), wait_for_eot_button_click() );
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
    co_await rn::visit( command,
                        L( process_player_input( _ ) ) );
    if( command.holds<button_click_t>() ) co_return;
  }
}

} // namespace eot

waitable<> end_of_turn() {
  SCOPED_SET_AND_CHANGE( g_doing_eot, true, false );
  return eot::process_inputs();
}

/****************************************************************
** Processing Player Input (During Turn).
*****************************************************************/
waitable<> process_player_input( UnitId,
                                 e_menu_actions action ) {
  // In the future we might need to put logic here that is spe-
  // cific to the mid-turn scenario, but for now this is suffi-
  // cient.
  return handle_menu_item( action );
}

waitable<> process_player_input(
    UnitId id, LandViewPlayerInput_t const& input ) {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;
  auto& q  = st.units;
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::colony: {
      co_await show_colony_view( input.get<colony>().id );
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
          orders_handler( id, orders );
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

waitable<LandViewPlayerInput_t> landview_player_input(
    UnitId id ) {
  LandViewPlayerInput_t response;
  if( auto maybe_orders = pop_unit_orders( id ) ) {
    response = LandViewPlayerInput::give_orders{
        .orders = *maybe_orders };
  } else {
    lg.debug( "asking orders for: {}", debug_string( id ) );
    SG().turn.need_eot = false;
    response           = co_await landview_get_next_input( id );
  }
  co_return response;
}

waitable<> query_unit_input( UnitId id ) {
  auto command = co_await co::first(
      wait_for_menu_selection(), landview_player_input( id ) );
  co_await overload_visit( command, [&]( auto const& action ) {
    return process_player_input( id, action );
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
waitable<bool> advance_unit( UnitId id ) {
  CHECK( !should_remove_unit_from_queue( id ) );
  // - if it is it in `goto` mode focus on it and advance it
  //
  // - if it is in the old world then ignore it, or possibly re-
  //   mind the user it is there.
  //
  // - if it is performing an action, such as building a road,
  //   advance the state. If it finishes then mark it as active
  //   so that it will wait for orders in the next step.
  //
  // - if it is in an indian village then advance it, and mark it
  //   active if it is finished.
  //
  // - if unit is waiting for orders then focus on it, make it
  //   blink, and wait for orders.

  if( is_unit_in_port( id ) ) {
    finish_turn( id );
    co_return false; // do not ask for orders.
  }

  // If it is a ship on the high seas then advance it. If it has
  // arrived in the old world then jump to the old world screen.
  if( is_unit_inbound( id ) || is_unit_outbound( id ) ) {
    e_high_seas_result res = advance_unit_on_high_seas( id );
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

waitable<> units_turn_one_pass( deque<UnitId>& q ) {
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

    bool should_ask = co_await advance_unit( id );
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
    co_await query_unit_input( id );
    // !! The unit may no longer exist at this point, e.g. if
    // they were disbanded or if they lost a battle to the na-
    // tives.
  }
}

waitable<> units_turn() {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;
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
    co_await units_turn_one_pass( q );
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
waitable<> colonies_turn() {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;
  lg.info( "processing colonies for the {}.", st.nation );
  queue<ColonyId> colonies;
  for( ColonyId colony_id : colonies_all( st.nation ) )
    colonies.push( colony_id );
  while( !colonies.empty() ) {
    ColonyId colony_id = colonies.front();
    colonies.pop();
    co_await evolve_colony_one_turn( colony_id );
  }
}

/****************************************************************
** Per-Nation Turn Processor
*****************************************************************/
waitable<> nation_turn() {
  CHECK( SG().turn.nation );
  auto& st = *SG().turn.nation;

  // Starting.
  if( !st.started ) {
    print_bar( '-', fmt::format( "[ {} ]", st.nation ) );
    st.started = true;
  }

  // Colonies.
  if( !st.did_colonies ) {
    co_await colonies_turn();
    st.did_colonies = true;
  }

  if( !st.did_units ) {
    co_await units_turn();
    st.did_units = true;
  }
  CHECK( st.units.empty() );
}

/****************************************************************
** Turn Processor
*****************************************************************/
waitable<> next_turn_impl() {
  landview_start_new_turn();
  auto& st = SG().turn;

  // Starting.
  if( !st.started ) {
    print_bar( '=', "[ Starting Turn ]" );
    map_units( []( Unit& unit ) { unit.new_turn(); } );
    st.new_turn();
    st.started = true;
  }

  // Body.
  if( st.nation.has_value() ) {
    co_await nation_turn();
    st.nation.reset();
  }

  while( !st.remainder.empty() ) {
    st.nation = NationState( st.remainder.front() );
    st.remainder.pop();
    co_await nation_turn();
    st.nation.reset();
  }

  // Ending.
  if( st.need_eot ) co_await end_of_turn();

  st.new_turn();
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
waitable<> next_turn() {
  ScopedPlanePush pusher( e_plane_config::land_view );
  co_await next_turn_impl();
}

} // namespace rn
