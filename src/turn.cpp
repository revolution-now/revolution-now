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
#include "conductor.hpp"
#include "dispatch.hpp"
#include "fb.hpp"
#include "flat-queue.hpp"
#include "frame.hpp"
#include "fsm.hpp"
#include "logging.hpp"
#include "ranges.hpp"
#include "render.hpp"
#include "sound.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "viewport.hpp"
#include "window.hpp"

// Flatbuffers
#include "fb/sg-turn_generated.h"

// base-util
#include "base-util/algo.hpp"
#include "base-util/misc.hpp"
#include "base-util/variant.hpp"

// Range-v3
#include "range/v3/view/take.hpp"

// C++ standard library
#include <algorithm>
#include <deque>

using namespace std;

namespace rn {

namespace {

/****************************************************************
** FSMs
*****************************************************************/
adt_s_rn_( TurnCycleState,                          //
           ( starting_cycle ),                      //
           ( inside_cycle,                          //
             ( bool, need_eot ),                    //
             ( e_nation, nation ),                  //
             ( flat_queue<e_nation>, remainder ) ), //
           ( ending_cycle,                          //
             ( bool, need_eot ) )                   //
);

adt_rn_( TurnCycleEvent,         //
         ( next_turn,            //
           ( bool, need_eot ) ), //
         ( end_cycle,            //
           ( bool, need_eot ) )  //
);

// clang-format off
fsm_transitions( TurnCycle,
  ((starting_cycle, next_turn),  ->,  inside_cycle   ),
  ((inside_cycle,   next_turn),  ->,  inside_cycle   ),
  ((inside_cycle,   end_cycle),  ->,  ending_cycle   ),
  ((ending_cycle,   next_turn),  ->,  starting_cycle )
);
// clang-format on

fsm_class( TurnCycle ) { //
  fsm_init( TurnCycleState::starting_cycle{} );

  fsm_transition( TurnCycle, starting_cycle, next_turn, ->,
                  inside_cycle ) {
    (void)cur;
    TurnCycleState::inside_cycle res{
        /*need_eot=*/event.need_eot,  //
        /*nation=*/e_nation::english, //
        /*remainder=*/{}              //
    };
    res.remainder.push( e_nation::french );
    res.remainder.push( e_nation::dutch );
    res.remainder.push( e_nation::spanish );
    return res;
  }

  fsm_transition( TurnCycle, inside_cycle, next_turn, ->,
                  inside_cycle ) {
    (void)event;
    TurnCycleState::inside_cycle res = cur;

    auto next = res.remainder.front();
    CHECK( next, "nation remainder queue is empty." );
    res.remainder.pop();
    res.nation = *next;
    res.need_eot &= event.need_eot;
    return res;
  }

  fsm_transition( TurnCycle, inside_cycle, end_cycle, ->,
                  ending_cycle ) {
    bool need_eot = event.need_eot && cur.need_eot;
    if( need_eot ) {
      // FIXME: temporary.
      mark_end_of_turn();
      auto& vp_state = viewport_rendering_state();
      vp_state       = viewport_state::none{};
    }
    return {
        /*need_eot=*/need_eot, //
    };
  }
};

FSM_DEFINE_FORMAT_RN_( TurnCycle );

/****************************************************************
** Save-Game State
*****************************************************************/
struct SAVEGAME_STRUCT( Turn ) {
  // Fields that are actually serialized.

  // clang-format off
  SAVEGAME_MEMBERS( Turn,
  ( TurnCycleFsm, cycle_state ));
  // clang-format on

public:
  // Fields that are derived from the serialized fields.

private:
  SAVEGAME_FRIENDS( Turn );
  SAVEGAME_SYNC() {
    // Sync all fields that are derived from serialized fields
    // and then validate (check invariants).

    return xp_success_t{};
  }
};
SAVEGAME_IMPL( Turn );

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
void advance_turn_state() {
  if( SG().cycle_state.process_events() )
    lg.debug( "turn cycle state: {}", SG().cycle_state );
  switch_( SG().cycle_state.state() ) {
    case_( TurnCycleState::starting_cycle ) {
      SG().cycle_state.send_event(
          TurnCycleEvent::next_turn{ /*need_eot=*/true } );
    }
    case_( TurnCycleState::inside_cycle ) {
      // If no units need to take orders this turn then we need
      // to pause at the end of the turn to allow the user to
      // take control or make changes.
      bool need_eot = val.need_eot && true;
      // TODO: check turn state here.
      if( !val.remainder.empty() )
        SG().cycle_state.send_event(
            TurnCycleEvent::next_turn{ /*need_eot=*/need_eot } );
      else
        SG().cycle_state.send_event(
            TurnCycleEvent::end_cycle{ /*need_eot=*/need_eot } );
    }
    case_( TurnCycleState::ending_cycle ) {
      if( !val.need_eot ) {
        SG().cycle_state.send_event(
            TurnCycleEvent::next_turn{} );
      } else {
        if( was_next_turn_button_clicked() )
          SG().cycle_state.send_event(
              TurnCycleEvent::next_turn{} );
      }
    }
    switch_exhaustive;
  }
}

/****************************************************************
** Helpers
*****************************************************************/
// For each unit in the queue, remove all occurrences of it after
// the first. Duplicate ids in the queue is not actually a prob-
// lem, since a unit will just be passed up if it has already
// moved. Hence this is not strictly necessary, but leads to a
// smoother user experience in situations where units are sponta-
// neously prioritized in the queue.
void deduplicate_q( deque<UnitId>* q ) {
  deque<UnitId>               new_q;
  absl::flat_hash_set<UnitId> s;
  for( auto id : *q ) {
    if( s.contains( id ) ) continue;
    s.insert( id );
    new_q.push_back( id );
  }
  *q = new_q;
}

// Log the first few ids in the queue and print ... if there are
// more than 10.
void log_q( deque<UnitId> const& q ) {
  auto   q_str = rng_to_string( q | rv::take( 10 ) );
  string dots  = q.size() > 10 ? " ..." : "";
  lg.debug( "queue front: {}{}", q_str, dots );
}

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
  for( auto nation : { e_nation::english, e_nation::spanish } ) {
    auto res = turn( nation );
    if( res == e_turn_result::orders_taken ) need_eot = false;
  }

  if( need_eot ) {
    /***************************************************/
    auto& vp_state = viewport_rendering_state();
    vp_state       = viewport_state::none{};

    mark_end_of_turn();

    frame_loop( true,
                [] { return was_next_turn_button_clicked(); } );
    return e_turn_result::no_orders_taken;
  }
  return e_turn_result::orders_taken;
}

e_turn_result turn( e_nation nation ) {
  // start of turn:

  lg.debug( "------ starting turn ({}) -------", nation );

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
      if( !unit_exists( id ) ) {
        // This will happen if the unit is disbanded or it is
        // cargo of something that was disbanded.
        lg.debug( "unit {} no longer exists.", id );
        q.pop_front();
        continue;
      }
      auto& unit = unit_from_id( id );
      CHECK( unit.nation() == nation );
      if( unit.finished_turn() ) {
        q.pop_front();
        continue;
      }
      lg.debug( "processing turn for {}", debug_string( id ) );

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
      while( CHECK_INL( !q.empty() ) && q.front() == id &&
             unit.orders_mean_input_required() &&
             !unit.moved_this_turn() ) {
        deduplicate_q( &q );
        log_q( q );
        lg.debug( "asking orders for: {}", debug_string( id ) );
        orders_taken = true;

        auto coords = coord_for_unit_indirect( id );
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
            return blink_unit.orders.has_value() ||
                   blink_unit.prioritize.size() > 0;
          } );
          for( auto add_id : blink_unit.add_to_back ) {
            auto& add_unit = unit_from_id( add_id );
            q.push_back( add_id );
            add_unit.unfinish_turn();
          }
          if( blink_unit.prioritize.size() > 0 ) {
            CHECK( !blink_unit.orders.has_value() );
            // The code that produces this prioritization list is
            // not supposed to allow any units in the list that
            // have already moved this turn. Doing this makes
            // this code here simpler since we don't have to deal
            // with cases where we go to the trouble of
            // restarting this while loop just for a unit that
            // won't move anyway and end up e.g. re-scrolling the
            // viewport for nothing in the process (non-ideal UI
            // effects).
            for( auto prio_id : blink_unit.prioritize ) {
              auto& prio_unit = unit_from_id( prio_id );
              CHECK( !prio_unit.moved_this_turn() );
              q.push_front( prio_id );
              prio_unit.unfinish_turn();
            }
            continue;
          }
          CHECK( blink_unit.orders.has_value() );
          maybe_orders = blink_unit.orders;
        } else {
          lg.debug( "found queued orders." );
        }
        /***************************************************/

        CHECK( maybe_orders.has_value() );
        auto& orders = *maybe_orders;
        lg.debug( "received orders: {}", orders );

        if( util::holds<orders::wait>( orders ) ) {
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
            lg.debug( "animating unit demotion to {}",
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
          conductor::play_request(
              combat_res->fight_stats->attacker_wins
                  ? conductor::e_request::won_battle_europeans
                  : conductor::e_request::lost_battle_europeans,
              conductor::e_request_probability::rarely );
          frame_loop( /*poll_input=*/true, [&depixelate_unit] {
            return depixelate_unit.finished;
          } );
          /***************************************************/
        }

        affect_orders( analysis );

        // Must immediately check if this unit has been killed in
        // combat or disbanded. If so then all the unit refer-
        // ences to it are invalid.
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

      // Must check if this unit has been killed in combat or
      // disbanded. If so then all the unit references to it are
      // invalid.
      if( !unit_exists( id ) ) continue;

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
}

} // namespace rn
