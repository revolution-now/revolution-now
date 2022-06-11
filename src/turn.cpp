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
#include "game-state.hpp"
#include "gs-colonies.hpp"
#include "gs-players.hpp"
#include "gs-units.hpp"
#include "gui.hpp"
#include "harbor-units.hpp"
#include "harbor-view.hpp"
#include "interrupts.hpp"
#include "land-view.hpp"
#include "logger.hpp"
#include "map-edit.hpp"
#include "map-updater.hpp"
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
#include "turn-plane.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "window.hpp"

// Rds
#include "gs-turn.rds.hpp"
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

wait<> advance_time( IGui& gui, TurnTimePoint& time_point ) {
  if( time_point.year == 1600 &&
      time_point.season == e_season::spring )
    co_await gui.message_box(
        "Starting in the year @[H]1600@[] the time scale "
        "changes.  Henceforth there will be both a "
        "@[H]Spring@[] and a @[H]Fall@[] turn each year." );
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
        time_point.season = e_season::fall;
      } else {
        // Stay in Spring and just go to the next year.
        ++time_point.year;
      }
      break;
    case e_season::summer:
      // We're not currently supporting four seasons per year, so
      // just revert it to the spring/fall cycle.
      time_point.season = e_season::fall;
      break;
    case e_season::fall:
      // Two seasons per year.
      time_point.season = e_season::spring;
      ++time_point.year;
      break;
  }
}

void reset_turn_obj( TurnState& st ) {
  queue<e_nation> remainder;
  remainder.push( e_nation::english );
  remainder.push( e_nation::french );
  remainder.push( e_nation::dutch );
  remainder.push( e_nation::spanish );
  st.started   = false;
  st.nation    = nothing;
  st.remainder = std::move( remainder );
}

/****************************************************************
** Menu Handlers
*****************************************************************/
wait<bool> proceed_to_leave_game() { co_return true; }

wait<> menu_handler( Planes& planes, Player& player,
                     IMapUpdater&        map_updater,
                     LandViewState&      land_view_state,
                     UnitsState&         units_state,
                     TerrainState const& terrain_state,
                     IGui& gui, e_menu_item item ) {
  switch( item ) {
    case e_menu_item::exit: {
      if( co_await proceed_to_leave_game() )
        throw game_quit_interrupt{};
      break;
    }
    case e_menu_item::save: {
      if( auto res = save_game( 0 ); !res ) {
        co_await gui.message_box(
            "There was a problem saving the game." );
        lg.error( "failed to save game: {}", res.error() );
      } else {
        co_await gui.message_box( fmt::format(
            "Successfully saved game to {}.", res ) );
        lg.info( "saved game to {}.", res );
      }
      break;
    }
    case e_menu_item::load: {
      if( co_await proceed_to_leave_game() )
        throw game_load_interrupt{};
      break;
    }
    case e_menu_item::revolution: {
      string answer = co_await gui.choice(
          { .msg     = "Declare Revolution?",
            .options = {
                { .key = "no", .display_name = "Not Yet..." },
                { .key          = "yes",
                  .display_name = "Give me liberty or give me "
                                  "death!" } } } );
      co_await gui.message_box( "You selected: {}", answer );
      break;
    }
    case e_menu_item::harbor_view: {
      co_await show_harbor_view( planes, player, units_state,
                                 terrain_state,
                                 /*selected_unit=*/nothing );
      break;
    }
    case e_menu_item::map_editor: {
      // Need to co_await so that the map_updater stays
      // alive.
      co_await run_map_editor( planes, map_updater,
                               land_view_state, terrain_state,
                               /*standalone_mode=*/false );
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

// Get all units in the eight squares that surround coord.
vector<UnitId> surrounding_units( Coord const& coord ) {
  vector<UnitId> res;
  for( e_direction d : refl::enum_values<e_direction> )
    for( auto id : units_from_coord( coord.moved( d ) ) )
      res.push_back( id );
  return res;
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

vector<UnitId> units_all( UnitsState const& units_state,
                          e_nation          n ) {
  vector<UnitId> res;
  res.reserve( units_state.all().size() );
  for( auto const& p : units_state.all() )
    if( n == p.second.unit.nation() ) res.push_back( p.first );
  return res;
}

// Apply a function to all units. The function may mutate the
// units.
void map_units( UnitsState&                       units_state,
                base::function_ref<void( Unit& )> func ) {
  for( auto& p : units_state.all() )
    func( units_state.unit_for( p.first ) );
}

void map_units( UnitsState& units_state, e_nation nation,
                base::function_ref<void( Unit& )> func ) {
  for( auto& p : units_state.all() ) {
    Unit& unit = units_state.unit_for( p.first );
    if( unit.nation() == nation ) func( unit );
  }
}

/****************************************************************
** Processing Player Input (End of Turn).
*****************************************************************/
namespace eot {

wait<> process_player_input( e_menu_item item, ColoniesState&,
                             IGui& gui, IMapUpdater& map_updater,
                             LandViewState& land_view_state,
                             Planes& planes, Player& player,
                             TerrainState const& terrain_state,
                             UnitsState&         units_state ) {
  // In the future we might need to put logic here that is spe-
  // cific to the end-of-turn, but for now this is sufficient.
  return menu_handler( planes, player, map_updater,
                       land_view_state, units_state,
                       terrain_state, gui, item );
}

wait<> process_player_input( LandViewPlayerInput_t const& input,
                             ColoniesState& colonies_state,
                             IGui& gui, IMapUpdater& map_updater,
                             LandViewState& land_view_state,
                             Planes& planes, Player& player,
                             TerrainState const& terrain_state,
                             UnitsState&         units_state ) {
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::colony: {
      co_await show_colony_view(
          planes,
          colonies_state.colony_for( input.get<colony>().id ) );
      break;
    }
    default: break;
  }
}

wait<> process_player_input( next_turn_t,
                             ColoniesState& colonies_state,
                             IGui& gui, IMapUpdater& map_updater,
                             LandViewState& land_view_state,
                             Planes& planes, Player& player,
                             TerrainState const& terrain_state,
                             UnitsState&         units_state ) {
  lg.debug( "end of turn button clicked." );
  co_return;
}

wait<> process_inputs(
    PanelPlane& panel_plane, MenuPlane& menu_plane,
    LandViewPlane& land_view_plane, Planes& planes, IGui& gui,
    ColoniesState& colonies_state, UnitsState& units_state,
    IMapUpdater& map_updater, LandViewState& land_view_state,
    TerrainState const& terrain_state, Player& player ) {
  land_view_plane.landview_reset_input_buffers();
  while( true ) {
    auto wait_for_button =
        co::fmap( [] λ( next_turn_t{} ),
                  panel_plane.wait_for_eot_button_click() );
    // The reason that we want to use co::first here instead of
    // interleaving the three streams is because as soon as one
    // becomes ready (and we start processing it) we want all the
    // others to be automatically be cancelled, which will have
    // the effect of disabling further input on them (e.g., dis-
    // abling menu items), which is what we want for a good user
    // experience.
    UserInput command = co_await co::first(            //
        wait_for_menu_selection( menu_plane ),         //
        land_view_plane.landview_eot_get_next_input(), //
        std::move( wait_for_button )                   //
    );
    co_await rn::visit(
        command,
        LC( process_player_input(
            _, colonies_state, gui, map_updater, land_view_state,
            planes, player, terrain_state, units_state ) ) );
    if( command.holds<next_turn_t>() ||
        command.get_if<LandViewPlayerInput_t>()
            .fmap( L( _.template holds<
                      LandViewPlayerInput::next_turn>() ) )
            .is_value_truish() )
      co_return;
  }
}

} // namespace eot

wait<> end_of_turn(
    PanelPlane& panel_plane, MenuPlane& menu_plane,
    LandViewPlane& land_view_plane, Planes& planes, IGui& gui,
    ColoniesState& colonies_state, UnitsState& units_state,
    IMapUpdater& map_updater, LandViewState& land_view_state,
    TerrainState const& terrain_state, Player& player ) {
  SCOPED_SET_AND_CHANGE( g_doing_eot, true, false );
  return eot::process_inputs(
      panel_plane, menu_plane, land_view_plane, planes, gui,
      colonies_state, units_state, map_updater, land_view_state,
      terrain_state, player );
}

/****************************************************************
** Processing Player Input (During Turn).
*****************************************************************/
wait<> process_player_input(
    UnitId, e_menu_item item, IMapUpdater& map_updater,
    IGui& gui, Player& player, NationTurnState&,
    TerrainState const& terrain_state, UnitsState& units_state,
    ColoniesState&, SettingsState const&, LandViewPlane&,
    Planes& planes, LandViewState& land_view_state ) {
  // In the future we might need to put logic here that is spe-
  // cific to the mid-turn scenario, but for now this is suffi-
  // cient.
  return menu_handler( planes, player, map_updater,
                       land_view_state, units_state,
                       terrain_state, gui, item );
}

wait<> process_player_input(
    UnitId id, LandViewPlayerInput_t const& input,
    IMapUpdater& map_updater, IGui& gui, Player& player,
    NationTurnState&    nat_turn_st,
    TerrainState const& terrain_state, UnitsState& units_state,
    ColoniesState& colonies_state, SettingsState const& settings,
    LandViewPlane& land_view_plane, Planes& planes,
    LandViewState& land_view_state ) {
  auto& st = nat_turn_st;
  auto& q  = st.units;
  switch( input.to_enum() ) {
    using namespace LandViewPlayerInput;
    case e::next_turn: {
      // The land view should never send us a 'next turn' command
      // when we are not at the end of a turn.
      SHOULD_NOT_BE_HERE;
    }
    case e::colony: {
      co_await show_colony_view(
          planes,
          colonies_state.colony_for( input.get<colony>().id ) );
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

      unique_ptr<OrdersHandler> handler = orders_handler(
          id, orders, &map_updater, gui, player, terrain_state,
          units_state, colonies_state, settings, land_view_plane,
          planes );
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
        land_view_plane.landview_reset_input_buffers();
      }
      if( !run_result.order_was_run ) break;
      // !! The unit may no longer exist at this point, e.g. if
      // they were disbanded or if they lost a battle to the na-
      // tives.

      // If the unit still exists, check if it has moved squares
      // in any fashion, and if so then wake up any foreign sen-
      // try'd neighbors.
      if( units_state.exists( id ) ) {
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
        co_await gui.message_box(
            "The selected unit(s) have already moved this "
            "turn." );
      else if( curr_size < orig_size )
        co_await gui.message_box(
            "Some of the selected units have already moved this "
            "turn." );
      for( UnitId id_to_add : prioritize )
        prioritize_unit( q, id_to_add );
      break;
    }
  }
}

wait<LandViewPlayerInput_t> landview_player_input(
    LandViewPlane& land_view_plane, NationTurnState& nat_turn_st,
    UnitsState const& units_state, UnitId id ) {
  LandViewPlayerInput_t response;
  if( auto maybe_orders = pop_unit_orders( id ) ) {
    response = LandViewPlayerInput::give_orders{
        .orders = *maybe_orders };
  } else {
    lg.debug( "asking orders for: {}",
              debug_string( units_state, id ) );
    nat_turn_st.need_eot = false;
    response =
        co_await land_view_plane.landview_get_next_input( id );
  }
  co_return response;
}

wait<> query_unit_input(
    UnitId id, IMapUpdater& map_updater, IGui& gui,
    Player& player, NationTurnState& nat_turn_st,
    TerrainState const& terrain_state, UnitsState& units_state,
    ColoniesState& colonies_state, SettingsState const& settings,
    LandViewPlane& land_view_plane, Planes& planes,
    MenuPlane& menu_plane, LandViewState& land_view_state ) {
  auto command = co_await co::first(
      wait_for_menu_selection( menu_plane ),
      landview_player_input( land_view_plane, nat_turn_st,
                             units_state, id ) );
  co_await overload_visit( command, [&]( auto const& action ) {
    return process_player_input(
        id, action, map_updater, gui, player, nat_turn_st,
        terrain_state, units_state, colonies_state, settings,
        land_view_plane, planes, land_view_state );
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
wait<bool> advance_unit( Planes&              planes,
                         LandViewPlane&       land_view_plane,
                         UnitsState&          units_state,
                         ColoniesState const& colonies_state,
                         TerrainState const&  terrain_state,
                         SettingsState const& settings,
                         Player& player, IGui& gui,
                         IMapUpdater& map_updater, UnitId id ) {
  CHECK( !should_remove_unit_from_queue( id ) );
  Unit& unit = units_state.unit_for( id );

  if( unit.orders() == e_unit_orders::road ) {
    perform_road_work( units_state, terrain_state, map_updater,
                       unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders() == e_unit_orders::none );
      co_await land_view_plane.landview_ensure_visible( id );
      co_await gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return( unit.orders() != e_unit_orders::road );
  }

  if( unit.orders() == e_unit_orders::plow ) {
    perform_plow_work( units_state, terrain_state, map_updater,
                       unit );
    if( unit.composition()[e_unit_inventory::tools] == 0 ) {
      CHECK( unit.orders() == e_unit_orders::none );
      co_await land_view_plane.landview_ensure_visible( id );
      // TODO: if we were clearing a forest then we should pick a
      // colony in the vicinity and add a certain amount of
      // lumber to it (see strategy guide for formula) and give a
      // message to the user.
      co_await gui.message_box(
          "Our pioneer has exhausted all of its tools." );
    }
    co_return( unit.orders() != e_unit_orders::plow );
  }

  if( is_unit_in_port( units_state, id ) ) {
    finish_turn( id );
    co_return false; // do not ask for orders.
  }

  // If it is a ship on the high seas then advance it. If it has
  // arrived in the old world then jump to the old world screen.
  if( is_unit_inbound( units_state, id ) ||
      is_unit_outbound( units_state, id ) ) {
    e_high_seas_result res = advance_unit_on_high_seas(
        terrain_state, units_state, player, id );
    switch( res ) {
      case e_high_seas_result::still_traveling:
        finish_turn( id );
        co_return false; // do not ask for orders.
      case e_high_seas_result::arrived_in_new_world: {
        lg.debug( "unit has arrived in new world." );
        maybe<Coord> dst_coord = find_new_world_arrival_square(
            units_state, colonies_state, terrain_state, player,
            units_state.harbor_view_state_of( id ) );
        if( !dst_coord.has_value() ) {
          co_await gui.message_box(
              "Unfortunately, while our @[H]{}@[] has arrived "
              "in the new world, there are no appropriate water "
              "squares on which to place it.  We will try again "
              "next turn.",
              units_state.unit_for( id ).desc().name );
          finish_turn( id );
          break;
        }
        units_state.unit_for( id ).clear_orders();
        co_await unit_to_map_square(
            units_state, terrain_state, player, settings, gui,
            map_updater, id, *dst_coord );
        unsentry_surroundings( id );
        co_return true; // needs to ask for orders.
      }
      case e_high_seas_result::arrived_in_harbor: {
        lg.debug( "unit has arrived in old world." );
        finish_turn( id );
        co_await show_harbor_view( planes, player, units_state,
                                   terrain_state, id );
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

wait<> units_turn_one_pass(
    Planes& planes, MenuPlane& menu_plane,
    LandViewPlane& land_view_plane, IMapUpdater& map_updater,
    IGui& gui, Player& player, NationTurnState& nat_turn_st,
    TerrainState const& terrain_state, UnitsState& units_state,
    ColoniesState& colonies_state, SettingsState const& settings,
    LandViewState& land_view_state, deque<UnitId>& q ) {
  while( !q.empty() ) {
    // lg.trace( "q: {}", q );
    UnitId id = q.front();
    // We need this check because units can be added into the
    // queue in this loop by user input. Also, the very first
    // check that we must do needs to be to check if the unit
    // still exists, which it might not if e.g. it was disbanded.
    if( !units_state.exists( id ) ||
        should_remove_unit_from_queue( id ) ) {
      q.pop_front();
      continue;
    }

    bool should_ask = co_await advance_unit(
        planes, land_view_plane, units_state, colonies_state,
        terrain_state, settings, player, gui, map_updater, id );
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
    co_await query_unit_input(
        id, map_updater, gui, player, nat_turn_st, terrain_state,
        units_state, colonies_state, settings, land_view_plane,
        planes, menu_plane, land_view_state );
    // !! The unit may no longer exist at this point, e.g. if
    // they were disbanded or if they lost a battle to the na-
    // tives.
  }
}

wait<> units_turn( Planes& planes, MenuPlane& menu_plane,
                   LandViewPlane& land_view_plane,
                   IMapUpdater& map_updater, IGui& gui,
                   Player& player, NationTurnState& nat_turn_st,
                   TerrainState const&  terrain_state,
                   UnitsState&          units_state,
                   ColoniesState&       colonies_state,
                   SettingsState const& settings,
                   LandViewState&       land_view_state ) {
  auto& st = nat_turn_st;
  auto& q  = st.units;

  // Unsentry any units that are sentried but have foreign units
  // in an adjacent square.
  map_units( units_state, st.nation, try_unsentry_unit );

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
    co_await units_turn_one_pass(
        planes, menu_plane, land_view_plane, map_updater, gui,
        player, nat_turn_st, terrain_state, units_state,
        colonies_state, settings, land_view_state, q );
    CHECK( q.empty() );
    // Refill the queue.
    auto units = units_all( units_state, st.nation );
    util::sort_by_key( units, []( auto id ) { return id._; } );
    erase_if( units, should_remove_unit_from_queue );
    if( units.empty() ) co_return;
    for( UnitId id : units ) q.push_back( id );
  }
}

/****************************************************************
** Per-Colony Turn Processor
*****************************************************************/
wait<> colonies_turn( LandViewPlane&       land_view_plane,
                      SettingsState const& settings,
                      UnitsState&          units_state,
                      TerrainState const&  terrain_state,
                      Player&              player,
                      ColoniesState&       colonies_state,
                      IMapUpdater& map_updater, IGui& gui,
                      Planes& planes ) {
  co_await evolve_colonies_for_player(
      land_view_plane, colonies_state, settings, units_state,
      terrain_state, player, map_updater, gui, planes );
}

/****************************************************************
** Per-Nation Turn Processor
*****************************************************************/
wait<> nation_turn(
    PanelPlane& panel_plane, MenuPlane& menu_plane,
    LandViewPlane& land_view_plane, Player& player,
    NationTurnState&    nat_turn_st,
    TerrainState const& terrain_state, UnitsState& units_state,
    SettingsState const& settings, PlayersState& players_state,
    ColoniesState& colonies_state, IMapUpdater& map_updater,
    IGui& gui, Planes& planes, LandViewState& land_view_state ) {
  auto& st = nat_turn_st;

  if( !player.human ) co_return; // TODO: Until we have AI.

  // Starting.
  if( !st.started ) {
    print_bar( '-', fmt::format( "[ {} ]", st.nation ) );
    st.started = true;
  }

  // Colonies.
  if( !st.did_colonies ) {
    co_await colonies_turn(
        land_view_plane, settings, units_state, terrain_state,
        player, colonies_state, map_updater, gui, planes );
    st.did_colonies = true;
  }

  if( !st.did_units ) {
    co_await units_turn(
        planes, menu_plane, land_view_plane, map_updater, gui,
        player, st, terrain_state, units_state, colonies_state,
        settings, land_view_state );
    st.did_units = true;
  }
  CHECK( st.units.empty() );

  // Ending.
  if( st.need_eot )
    co_await end_of_turn(
        panel_plane, menu_plane, land_view_plane, planes, gui,
        colonies_state, units_state, map_updater,
        land_view_state, terrain_state, player );
}

/****************************************************************
** Turn Processor
*****************************************************************/
wait<> next_turn(
    PanelPlane& panel_plane, MenuPlane& menu_plane,
    LandViewPlane& land_view_plane, PlayersState& players_state,
    TerrainState const& terrain_state, UnitsState& units_state,
    SettingsState const& settings, TurnState& turn_state,
    ColoniesState& colonies_state, IMapUpdater& map_updater,
    IGui& gui, Planes& planes, LandViewState& land_view_state ) {
  land_view_plane.landview_start_new_turn();
  auto& st = turn_state;

  // Starting.
  if( !st.started ) {
    print_bar( '=', "[ Starting Turn ]" );
    map_units( units_state,
               []( Unit& unit ) { unit.new_turn(); } );
    reset_turn_obj( st );
    st.started = true;
  }

  // Body.
  if( st.nation.has_value() ) {
    Player& player = players_state.players[st.nation->nation];
    co_await nation_turn(
        panel_plane, menu_plane, land_view_plane, player,
        *st.nation, terrain_state, units_state, settings,
        players_state, colonies_state, map_updater, gui, planes,
        land_view_state );
    st.nation.reset();
  }

  while( !st.remainder.empty() ) {
    st.nation = new_nation_turn_obj( st.remainder.front() );
    st.remainder.pop();
    Player& player = players_state.players[st.nation->nation];
    co_await nation_turn(
        panel_plane, menu_plane, land_view_plane, player,
        *st.nation, terrain_state, units_state, settings,
        players_state, colonies_state, map_updater, gui, planes,
        land_view_state );
    st.nation.reset();
  }

  reset_turn_obj( st );
  co_await advance_time( gui, st.time_point );
}

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
wait<> turn_loop( Planes& planes, PlayersState& players_state,
                  TerrainState const&  terrain_state,
                  LandViewState&       land_view_state,
                  UnitsState&          units_state,
                  SettingsState const& settings,
                  TurnState&           turn_state,
                  ColoniesState&       colonies_state,
                  IMapUpdater&         map_updater ) {
  WindowPlane   window_plane;
  RealGui       gui( window_plane );
  MenuPlane     menu_plane;
  LandViewPlane land_view_plane(
      menu_plane, window_plane, land_view_state, terrain_state );
  PanelPlane panel_plane( menu_plane );

  auto        popper = planes.new_group();
  PlaneGroup& group  = planes.back();
  group.push( land_view_plane );
  group.push( panel_plane );
  group.push( menu_plane );
  group.push( window_plane );

  // FIXME: Temporary
  land_view_plane.zoom_out_full();

  while( true )
    co_await next_turn( panel_plane, menu_plane, land_view_plane,
                        players_state, terrain_state,
                        units_state, settings, turn_state,
                        colonies_state, map_updater, gui, planes,
                        land_view_state );
}

} // namespace rn
