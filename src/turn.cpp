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
#include "dispatch.hpp"
#include "logging.hpp"
#include "loops.hpp"
#include "ownership.hpp"
#include "render.hpp"
#include "sound.hpp"
#include "unit.hpp"
#include "viewport.hpp"
#include "window.hpp"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

// C++ standard library
#include <algorithm>
#include <deque>

using namespace std;

namespace rn {

namespace {

bool animate_move( TravelAnalysis const& analysis ) {
  CHECK( util::holds<e_unit_travel_good>( analysis.desc ) );
  auto type = get<e_unit_travel_good>( analysis.desc );
  // TODO: in the case of board_ship we need to make sure that
  // the ship being borded gets rendered on top because there may
  // be a stack of ships in the square, otherwise it will be de-
  // ceiving to the player. This is because when a land unit en-
  // ters a water square it will just automatically pick a ship
  // and board it.
  switch( type ) {
    case e_unit_travel_good::map_to_map: return true;
    case e_unit_travel_good::board_ship: return true;
    case e_unit_travel_good::offboard_ship: return true;
    case e_unit_travel_good::land_fall: return false;
  };
  SHOULD_NOT_BE_HERE;
}

} // namespace

e_turn_result turn() {
  // If no units need to take orders this turn then we need to
  // pause at the end of the turn to allow the user to take
  // control or make changes. In that case, the flag will remain
  // true. On the other hand, if at least one unit takes orders
  // then that means that the user will at least have that
  // opportunity to have control and so then we don't need to
  // pause at the end of the turn. This flag controls that.
  bool need_eot = true;
  // for( auto nation : all_nations() ) {
  // for( auto nation : {e_nation::dutch} ) {
  for( auto nation : {e_nation::dutch, e_nation::french} ) {
    auto res = turn( nation );
    if( res == e_turn_result::quit ) return e_turn_result::quit;
    if( res == e_turn_result::orders_taken ) need_eot = false;
  }

  if( need_eot ) {
    /***************************************************/
    auto& vp_state = viewport_rendering_state();
    vp_state       = viewport_state::none{};
    // frame_loop( true, [] { return false; } );
    // TODO: use enums here
    auto res =
        ui::select_box( "End of turn.", {"Continue", "Quit"} );
    if( res == "Quit" ) return e_turn_result::quit;
    return e_turn_result::no_orders_taken;
  }
  return e_turn_result::orders_taken;
}

e_turn_result turn( e_nation nation ) {
  // start of turn:

  logger->debug( "------ starting turn ({}) -------", nation );

  // Mark all units as not having moved.
  map_units( []( Unit& unit ) { unit.new_turn(); } );

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

  bool orders_taken = false;

  auto& vp_state = viewport_rendering_state();

  // We keep looping until all units that need moving have moved.
  // We don't know this list a priori because some units may de-
  // cide to require orders during the course of this process,
  // and this could happen for various reasons. Perhaps even
  // units could be created during this process (?).
  while( true ) {
    auto units = units_all( nation );
    // Optional, but makes for good consistency for units to ask
    // for orders in the order that they were created.
    // TODO: in the future, try to sort by geographic location.
    util::sort_by_key( units, []( auto id ) { return id._; } );
    auto finished = []( UnitId id ) {
      return unit_from_id( id ).finished_turn();
    };
    if( all_of( units.begin(), units.end(), finished ) ) break;

    deque<UnitId> q;
    for( auto id : units ) q.push_back( id );

    //  Iterate through all units, for each:
    while( !q.empty() ) {
      auto id = q.front();
      // I think this will trigger; if it does then we need to to
      // remove it from the queue if it no longer exists.
      CHECK( unit_exists( id ) );
      auto& unit = unit_from_id( id );
      CHECK( unit.nation() == nation );
      if( unit.finished_turn() ) {
        q.pop_front();
        continue;
      }
      logger->debug( "processing turn for {}",
                     debug_string( id ) );

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
        orders_taken = true;

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
          frame_loop( true, [&blink_unit] {
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
        logger->debug( "received orders: {}", orders );

        if( util::holds<orders::quit_t>( orders ) )
          return e_turn_result::quit;

        if( util::holds<orders::wait_t>( orders ) ) {
          q.push_back( q.front() );
          q.pop_front();
          break;
        }
        auto maybe_intent = player_intent( id, orders );
        CHECK( maybe_intent.has_value(),
               "no handler for orders {}", orders );
        auto const& analysis = maybe_intent.value();

        if( !confirm_explain( analysis ) ) continue;

        // Check if the unit is physically moving; usually at
        // this point it will be unless it is e.g. a ship of-
        // floading units.
        if_v( analysis, TravelAnalysis, mv_res ) {
          /***************************************************/
          if( animate_move( *mv_res ) ) {
            play_sound_effect( e_sfx::move );
            viewport().ensure_tile_visible( mv_res->move_target,
                                            /*smooth=*/true );
            vp_state = viewport_state::slide_unit(
                id, mv_res->move_target );
            auto& slide_unit =
                get<viewport_state::slide_unit>( vp_state );
            // In this call we specify that it should not collect
            // any user input (keyboard, mouse) to avoid movement
            // commands (issued during animation) from getting
            // swallowed.
            frame_loop( false, [&slide_unit] {
              return slide_unit.percent >= 1.0;
            } );
          }
          /***************************************************/
        }
        if_v( analysis, CombatAnalysis, combat_res ) {
          /***************************************************/
          play_sound_effect( e_sfx::move );
          viewport().ensure_tile_visible(
              combat_res->attack_target,
              /*smooth=*/true );
          vp_state = viewport_state::slide_unit(
              id, combat_res->attack_target );
          auto& slide_unit =
              get<viewport_state::slide_unit>( vp_state );
          frame_loop( /*poll_input=*/true, [&slide_unit] {
            return slide_unit.percent >= 1.0;
          } );
          CHECK( combat_res->target_unit );
          CHECK( combat_res->fight_stats );
          auto dying_unit =
              combat_res->fight_stats->attacker_wins
                  ? *combat_res->target_unit
                  : id;
          auto const& dying_unit_desc =
              unit_from_id( dying_unit ).desc();
          Opt<e_unit_type> demote_to;
          if( dying_unit_desc.demoted.has_value() ) {
            logger->debug( "animating unit demotion to {}",
                           dying_unit_desc.demoted.value() );
            demote_to = dying_unit_desc.demoted.value();
          }
          vp_state = viewport_state::depixelate_unit(
              dying_unit, demote_to );
          auto& depixelate_unit =
              get<viewport_state::depixelate_unit>( vp_state );
          play_sound_effect(
              combat_res->fight_stats->attacker_wins
                  ? e_sfx::attacker_won
                  : e_sfx::attacker_lost );
          frame_loop( /*poll_input=*/true, [&depixelate_unit] {
            return depixelate_unit.finished;
          } );
          /***************************************************/
        }

        affect_orders( analysis );

        // Must immediately check if this unit has been killed in
        // combat. If so then all the unit references to it are
        // invalid.
        if( !unit_exists( id ) ) break;

        // Note that it shouldn't hurt if the unit is already in
        // the queue, since this turn code will never move a unit
        // after it has already completed its turn, no matter how
        // many times it appears in the queue.
        for( auto unit_id : units_to_prioritize( analysis ) ) {
          CHECK( unit_id != id );
          q.push_front( unit_id );
        }
      }

      if( !unit_exists( id ) ) break;

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
  //    clang-format on

  return orders_taken ? e_turn_result::orders_taken
                      : e_turn_result::no_orders_taken;
} // namespace rn

} // namespace rn
