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
#include "logging.hpp"
#include "loops.hpp"
#include "movement.hpp"
#include "orders.hpp"
#include "ownership.hpp"
#include "render.hpp"
#include "unit.hpp"
#include "viewport.hpp"

// base-util
#include "base-util/variant.hpp"

// C++ standard library
#include <algorithm>
#include <deque>

using namespace std;

namespace rn {

namespace {

bool animate_move( ProposedMoveAnalysisResult const& analysis ) {
  CHECK( util::holds<e_unit_mv_good>( analysis.desc ) );
  auto type = get<e_unit_mv_good>( analysis.desc );
  switch( type ) {
    case e_unit_mv_good::map_to_map: return true;
    case e_unit_mv_good::board_ship: return true;
    case e_unit_mv_good::offboard_ship: return true;
    case e_unit_mv_good::land_fall: return false;
  };
  SHOULD_NOT_BE_HERE;
  return false;
}

} // namespace

e_turn_result turn() {
  for( auto nation : all_nations() ) {
    auto res = turn( nation );
    if( res == e_turn_result::quit ) return e_turn_result::quit;
  }
  return e_turn_result::cont;
}

e_turn_result turn( e_nation nation ) {
  // start of turn:

  // Mark all units as not having moved.
  reset_moves();

  //  clang-format off
  //  Iterate through the colonies, for each:
  //  TODO

  //    * advance state of the colony

  //    * display messages to user any/or show animations where
  //    necessary

  //    * allow them to enter colony when events happens; in that
  //    case
  //      go to the colony screen game loop.  When the user exits
  //      the colony screen then this colony iteration
  //      immediately proceeds; i.e., user cannot enter any other
  //      colonies.  This prevents the user from making
  //      last-minute changes to colonies that have not yet been
  //      advanced in this turn (otherwise that might allow
  //      cheating in some way).

  //    * during this time, the user is not free to scroll
  //      map (menus?) or make any changes to units.  They are
  //      also not allowed to enter colonies apart from the one
  //      that has just been processed.

  //  Advance the state of the old world, possibly displaying
  //  messages to the user where necessary.
  //  clang-format on

  // If no units need to take orders this turn then we need to
  // pause at the end of the turn to allow the user to take
  // control or make changes.  In that case, the flag will remain
  // true.  On the other hand, if at least one unit takes orders
  // then that means that the user will at least have that
  // opportunity to have control and so then we don't need to
  // pause at the end of the turn.  This flag controls that.
  // TODO: consider using a flag type from type_safe
  auto need_eot_loop{true};

  auto& vp_state = viewport_rendering_state();

  // We keep looping until all units that need moving have moved.
  // We don't know this list a priori because some units may de-
  // cide to require orders during the course of this process,
  // and this could happen for various reasons. Perhaps even
  // units could be created during this process (?).
  while( true ) {
    auto units    = units_all( nation );
    auto finished = []( UnitId id ) {
      return unit_from_id( id ).finished_turn();
    };
    if( all_of( units.begin(), units.end(), finished ) ) break;

    deque<UnitId> q;
    for( auto id : units ) q.push_back( id );

    //  Iterate through all units, for each:
    while( !q.empty() ) {
      auto& unit = unit_from_id( q.front() );
      if( unit.finished_turn() ) {
        q.pop_front();
        continue;
      }
      auto id = unit.id();
      logger->debug( "processing turn for {}",
                     debug_string( id ) );
      // This will trigger until we start distinguishing nations.
      CHECK( unit.nation() == nation );

      //    clang-format off
      //
      //    * if it is it in `goto` mode focus on it and advance
      //      it
      //
      //    * if it is a ship on the high seas then advance it
      //        if it has arrived in the old world then jump to
      //        the old world screen (maybe ask user whether they
      //        want to ignore), which has its own game loop (see
      //        old-world loop).
      //
      //    * if it is in the old world then ignore it, or
      //      possibly remind the user it is there.
      //
      //    * if it is performing an action, such as building a
      //      road, advance the state.  If it finishes then mark
      //      it as active so that it will wait for orders in the
      //      next step.
      //
      //    * if it is in an indian village then advance it, and
      //      mark it active if it is finished.

      //    * if unit is waiting for orders then focus on it, and
      //      enter a realtime game loop where the user can
      //      interact with the map and GUI in general.  See
      //      `unit orders` game loop.
      //
      //    clang-format on
      while( q.front() == id &&
             unit.orders_mean_input_required() &&
             !unit.moved_this_turn() ) {
        logger->debug( "asking orders for: {}",
                       debug_string( id ) );
        need_eot_loop = false;

        auto coords = coords_for_unit( id );
        viewport().ensure_tile_visible( coords,
                                        /*smooth=*/true );

        auto maybe_orders = pop_unit_orders( id );

        /***************************************************/
        if( !maybe_orders.has_value() ) {
          vp_state = viewport_state::blink_unit{};
          auto& blink_unit =
              get<viewport_state::blink_unit>( vp_state );
          blink_unit.id = id;
          frame_throttler( true, [&blink_unit] {
            return blink_unit.orders.has_value();
          } );
          CHECK( blink_unit.orders.has_value() );
          maybe_orders = blink_unit.orders;
        } else {
          logger->debug( "found queued orders." );
        }
        /***************************************************/

        CHECK( maybe_orders.has_value() );
        auto& orders = *maybe_orders;
        logger->debug( "received orders: {}", orders.index() );

        if( util::holds<orders::quit_t>( orders ) )
          return e_turn_result::quit;

        if( util::holds<orders::wait_t>( orders ) ) {
          q.push_back( q.front() );
          q.pop_front();
          break;
        }
        auto analysis = analyze_proposed_orders( id, orders );
        if( confirm_explain_orders( analysis ) ) {
          // Check if the unit is physically moving; usually at
          // this point it will be unless it is e.g. a ship
          // offloading units.
          if_v( analysis.result, ProposedMoveAnalysisResult,
                mv_res ) {
            /***************************************************/
            if( animate_move( *mv_res ) ) {
              viewport().ensure_tile_visible(
                  mv_res->move_target,
                  /*smooth=*/true );
              vp_state = viewport_state::slide_unit(
                  id, mv_res->move_target );
              auto& slide_unit =
                  get<viewport_state::slide_unit>( vp_state );
              // In this call we specify that it should not
              // collect any user input (keyboard, mouse) to
              // avoid movement commands (issued during
              // animation) from getting swallowed.
              frame_throttler( false, [&slide_unit] {
                return slide_unit.percent >= 1.0;
              } );
            }
            /***************************************************/
          }
          apply_orders( id, analysis );
          // Note that it shouldn't hurt if the unit is already
          // in the queue, since this turn code will never move a
          // unit after it has already completed its turn, no
          // matter how many times it appears in the queue.
          for( auto prioritize : analysis.units_to_prioritize() )
            q.push_front( prioritize );
        }
      }

      // Now we must decide if the unit has finished its turn.
      // TODO: try to put this in the unit class.
      if( unit.moved_this_turn() ||
          !unit.orders_mean_input_required() )
        unit.finish_turn();
    }
  }

  //    clang-format off
  //    * Make AI moves
  //        Make European moves
  //        Make Native moves
  //        Make expeditionary force moves
  //      TODO

  //    * if no player units needed orders then show a message
  //    somewhere
  //      that says "end of turn" and let the user interact with
  //      the map and GUI.
  //    clang-format on
  if( need_eot_loop ) {
    /***************************************************/
    // disable EOT for now
    // vp_state = viewport_state::none{};
    // frame_throttler( true, [] { return false; } );
    /***************************************************/
    // switch( res ) {
    //  case e_eot_loop_result::quit_game:
    //    return e_turn_result::quit;
    //  case e_eot_loop_result::none: break;
    //};
  }
  return e_turn_result::cont;
} // namespace rn

} // namespace rn
