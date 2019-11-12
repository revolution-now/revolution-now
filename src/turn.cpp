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
#include "flat-deque.hpp"
#include "flat-queue.hpp"
#include "frame.hpp"
#include "fsm.hpp"
#include "land-view.hpp"
#include "logging.hpp"
#include "panel.hpp" // FIXME
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

vector<e_nation> const g_turn_ordering{
    e_nation::english,
    e_nation::french,
    e_nation::dutch,
    e_nation::spanish,
};

// FIXME: Hack.
Opt<PlayerIntent> g_player_intent;

/****************************************************************
** Helpers
*****************************************************************/
// For each unit in the queue, remove all occurrences of it after
// the first. Duplicate ids in the queue is not actually a prob-
// lem, since a unit will just be passed up if it has already
// moved. Hence this is not strictly necessary, but leads to a
// smoother user experience in situations where units are sponta-
// neously prioritized in the queue.
template<typename T>
void deduplicate_deque( flat_deque<T>* q ) {
  flat_deque<T>          new_q;
  absl::flat_hash_set<T> s;
  s.reserve( q->size() );
  while( q->size() > 0 ) {
    auto item = q->front()->get();
    q->pop_front();
    if( s.contains( item ) ) continue;
    s.insert( item );
    new_q.push_back( item );
  }
  *q = std::move( new_q );
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

/****************************************************************
** Unit Turn FSM
*****************************************************************/
// This FSM represents the state across the processing of a
// single unit.
adt_rn_( UnitInputState,                      //
         ( none ),                            //
         ( processing ),                      //
         ( asking ),                          //
         ( have_response,                     //
           ( UnitInputResponse, response ) ), //
         ( executing_orders,                  //
           ( orders_t, orders ),              //
           ( bool, confirmed ) ),             //
         ( executed,                          //
           ( Vec<UnitId>, add_to_front ) )    //
);

adt_rn_( UnitInputEvent,                      //
         ( process ),                         //
         ( ask ),                             //
         ( put_response,                      //
           ( UnitInputResponse, response ) ), //
         ( execute ),                         //
         ( cancel ),                          //
         ( end,                               //
           ( Vec<UnitId>, add_to_front ) )    //
);

// clang-format off
fsm_transitions( UnitInput,
  ((none,             process     ),  ->,  processing       ),
  ((processing,       ask         ),  ->,  asking           ),
  ((processing,       put_response),  ->,  have_response    ),
  ((processing,       cancel      ),  ->,  none             ),
  ((asking,           put_response),  ->,  have_response    ),
  ((have_response,    execute     ),  ->,  executing_orders ),
  ((executing_orders, cancel      ),  ->,  processing       ),
  ((executing_orders, end         ),  ->,  executed         ),
  ((executed,         process     ),  ->,  processing       )
);
// clang-format on

fsm_class( UnitInput ) { //
  fsm_init( UnitInputState::none{} );

  fsm_transition( UnitInput, processing, put_response, ->,
                  have_response ) {
    (void)cur;
    return { /*response=*/std::move( event.response ) };
  }
  fsm_transition( UnitInput, asking, put_response, ->,
                  have_response ) {
    (void)cur;
    return { /*response=*/std::move( event.response ) };
  }
  fsm_transition( UnitInput, have_response, execute, ->,
                  executing_orders ) {
    (void)event;
    CHECK( cur.response.orders.has_value() );
    return { /*orders=*/*cur.response.orders,
             /*confirmed=*/false };
  }
  fsm_transition( UnitInput, executing_orders, end, ->,
                  executed ) {
    (void)cur;
    return { /*add_to_front=*/std::move( event.add_to_front ) };
  }
};

FSM_DEFINE_FORMAT_RN_( UnitInput );

// Will be called repeatedly until done() is called.
void advance_unit_input_state( UnitInputFsm&            fsm,
                               tl::function_ref<void()> done,
                               UnitId                   id ) {
  switch_( fsm.mutable_state() ) {
    case_( UnitInputState::none ) {
      done(); //
    }
    case_( UnitInputState::processing ) {
      // - if it is it in `goto` mode focus on it and advance
      //   it
      //
      // - if it is a ship on the high seas then advance it if
      //   it has arrived in the old world then jump to the old
      //   world screen (maybe ask user whether they want to
      //   ignore), which has its own game loop (see old-world
      //   loop).
      //
      // - if it is in the old world then ignore it, or pos-
      //   sibly remind the user it is there.
      //
      // - if it is performing an action, such as building a
      //   road, advance the state. If it finishes then mark it
      //   as active so that it will wait for orders in the
      //   next step.
      //
      // - if it is in an indian village then advance it, and
      //   mark it active if it is finished.
      //
      // - if unit is waiting for orders then focus on it, make
      //   it blink, and wait for orders.
      CHECK( unit_exists( id ) );

      auto& unit = unit_from_id( id );

      if( unit.finished_turn() ) {
        fsm.send_event( UnitInputEvent::cancel{} );
        break_;
      }

      if( !is_unit_on_map_indirect( id ) ) {
        // TODO.
        unit.finish_turn();
        fsm.send_event( UnitInputEvent::cancel{} );
        break_;
      }

      if( !unit.orders_mean_input_required() ||
          unit.mv_pts_exhausted() ) {
        unit.finish_turn();
        fsm.send_event( UnitInputEvent::cancel{} );
        break_;
      }

      lg.debug( "asking orders for: {}", debug_string( id ) );

      auto maybe_orders = pop_unit_orders( id );
      if( maybe_orders.has_value() ) {
        UnitInputResponse response;
        response.orders = *maybe_orders;
        fsm.send_event( UnitInputEvent::put_response{
            /*response=*/std::move( response ) } );
        break_;
      }

      landview_ask_orders( id );
      fsm.send_event( UnitInputEvent::ask{} );
      break_;
    }
    case_( UnitInputState::asking ) {
      auto maybe_response = unit_input_response();
      if( maybe_response.has_value() )
        fsm.send_event( UnitInputEvent::put_response{
            /*response=*/std::move( *maybe_response ) } );
      else
        done();
    }
    case_( UnitInputState::have_response ) {
      done(); //
    }
    case_( UnitInputState::executing_orders ) {
      if( !val.confirmed ) {
        CHECK( !g_player_intent.has_value() );
        auto maybe_intent = player_intent( id, val.orders );
        CHECK( maybe_intent.has_value(),
               "no handler for orders {}", val.orders );

        g_player_intent = std::move( *maybe_intent );

        if( !confirm_explain( *g_player_intent ) ) {
          fsm.send_event( UnitInputEvent::cancel{} );
          g_player_intent = nullopt;
          break_;
        }

        // Kick off animation if needed.
        switch_( *g_player_intent ) {
          case_( TravelAnalysis ) {
            ASSIGN_CHECK_OPT( d, val.move_src.direction_to(
                                     val.move_target ) );
            landview_animate_move( id, d );
            DCHECK( landview_is_animating() );
          }
          case_( CombatAnalysis ) {
            auto attacker = id;
            ASSIGN_CHECK_OPT( defender, val.target_unit );
            ASSIGN_CHECK_OPT( stats, val.fight_stats );
            auto const& defender_unit = unit_from_id( defender );
            auto const& attacker_unit = unit_from_id( attacker );
            e_depixelate_anim dp_anim =
                stats.attacker_wins
                    ? ( defender_unit.desc().demoted.has_value()
                            ? e_depixelate_anim::demote
                            : e_depixelate_anim::death )
                    : ( attacker_unit.desc().demoted.has_value()
                            ? e_depixelate_anim::demote
                            : e_depixelate_anim::death );
            landview_animate_attack( attacker, defender,
                                     stats.attacker_wins,
                                     dp_anim );
            CHECK( landview_is_animating() );
          }
          switch_non_exhaustive;
        }
        val.confirmed = true;
      }

      if( landview_is_animating() ) {
        done();
        break_;
      }

      // Animation (if any) is finished.
      CHECK( g_player_intent );
      affect_orders( *g_player_intent );
      fsm.send_event( UnitInputEvent::end{
          /*add_to_front=*/units_to_prioritize(
              *g_player_intent ) } );
      g_player_intent = nullopt;
      // !! Unit may no longer exist here.
    }
    case_( UnitInputState::executed ) {
      // !! Unit may no longer exist here.
      done(); //
    }
    switch_exhaustive;
  }
}

/****************************************************************
** Nation Turn FSM
*****************************************************************/
// This FSM represents the state across the processing of a
// single turn for a single nation.
adt_rn_( NationTurnState,                  //
         ( starting ),                     //
         ( doing_units,                    //
           ( bool, need_eot ),             //
           ( flat_deque<UnitId>, q ),      //
           ( Opt<UnitInputFsm>, uturn ) ), //
         ( ending,                         //
           ( bool, need_eot ) )            //
);

adt_rn_( NationTurnEvent, //
         ( next ),        //
         ( end )          //
);

// clang-format off
fsm_transitions( NationTurn,
  ((starting,    next),  ->,  doing_units),
  ((doing_units, end ),  ->,  ending     ),
);
// clang-format on

fsm_class( NationTurn ) {
  fsm_init( NationTurnState::starting{} );

  fsm_transition_( NationTurn, starting, next, ->,
                   doing_units ) {
    return { /*need_eot=*/true,
             /*q=*/{},
             /*uturn=*/{} };
  }

  fsm_transition( NationTurn, doing_units, end, ->, ending ) {
    (void)event;
    return { /*need_eot=*/cur.need_eot };
  }
};

FSM_DEFINE_FORMAT_RN_( NationTurn );

// Will be called repeatedly until done() is called.
void advance_nation_turn_state( NationTurnFsm&           fsm,
                                tl::function_ref<void()> done,
                                e_nation nation ) {
  switch_( fsm.mutable_state() ) {
    case_( NationTurnState::starting ) {
      lg.info( "-------[ Starting turn for {} ]-------",
               nation );
      fsm.send_event( NationTurnEvent::next{} );
    }
    case_( NationTurnState::doing_units ) {
      auto& doing_units = val;
      auto  log_q       = [&] {
        deduplicate_deque( &doing_units.q );
        lg.debug( "q: {}", doing_units.q.to_string( 3 ) );
      };
      if( doing_units.q.empty() ) {
        CHECK( !doing_units.uturn.has_value() );
        auto units = units_all( nation );
        util::sort_by_key( units,
                           []( auto id ) { return id._; } );
        units.erase(
            remove_if( units.begin(), units.end(),
                       L( unit_from_id( _ ).finished_turn() ) ),
            units.end() );
        for( auto id : units ) doing_units.q.push_back( id );
        log_q();
      }
      bool done_uturn = false;
      while( !done_uturn ) {
        if( doing_units.q.empty() ) break;
        auto id = doing_units.q.front()->get();
        // We need this check because units can be added into
        // the queue in this loop by user input.
        if( !unit_exists( id ) ||
            unit_from_id( id ).finished_turn() ) {
          doing_units.q.pop_front();
          log_q();
          doing_units.uturn = nullopt;
          continue;
        }
        // We have a unit that has not finished its turn.
        if( !doing_units.uturn.has_value() ) {
          doing_units.uturn = UnitInputFsm{};
          doing_units.uturn->send_event(
              UnitInputEvent::process{} );
        }
        DCHECK( doing_units.uturn.has_value() );
        fsm_auto_advance( *doing_units.uturn, "unit-turn",
                          { advance_unit_input_state }, id );
        switch_( doing_units.uturn->state() ) {
          case_( UnitInputState::none ) {
            // All unit turns should end here.
          }
          case_( UnitInputState::processing ) {
            FATAL(
                "should not be in this state after an advance "
                "call." );
          }
          case_( UnitInputState::asking ) {
            doing_units.need_eot = false;
            done_uturn           = true;
          }
          case_( UnitInputState::have_response ) {
            for( auto id : val.response.add_to_back ) {
              doing_units.q.push_back( id );
              unit_from_id( id ).unfinish_turn();
            }
            auto const& maybe_orders = val.response.orders;
            auto const& add_to_front = val.response.add_to_front;
            // Can only have one or the other.
            CHECK( maybe_orders.has_value() ==
                   add_to_front.empty() );
            if( !add_to_front.empty() ) {
              for( auto id : val.response.add_to_front ) {
                doing_units.q.push_front( id );
                unit_from_id( id ).unfinish_turn();
              }
              log_q();
              break_;
            }
            ASSIGN_CHECK_OPT( orders, maybe_orders );
            if( util::holds<orders::wait>( orders ) ) {
              doing_units.q.push_back( id );
              CHECK( doing_units.q.front() == id );
              doing_units.q.pop_front();
              // fallthrough.
            }
            log_q();
            doing_units.uturn->send_event(
                UnitInputEvent::execute{} );
          }
          case_( UnitInputState::executing_orders ) {
            done_uturn = true;
          }
          case_( UnitInputState::executed ) {
            for( auto id : val.add_to_front )
              doing_units.q.push_front( id );
            log_q();
            doing_units.uturn->send_event(
                UnitInputEvent::process{} );
          }
          switch_exhaustive;
        }
        // There may be no queue front at this point if there was
        // only one unit in the queue and it died.
        if( !doing_units.q.front().has_value() ||
            doing_units.q.front() != id )
          doing_units.uturn = nullopt;
      }
      if( doing_units.q.empty() )
        fsm.send_event( NationTurnEvent::end{} );
      else
        done();
    }
    case_( NationTurnState::ending ) {
      done(); //
    }
    switch_exhaustive;
  }
}

/****************************************************************
** Turn Cycle FSM
*****************************************************************/
// This FSM represents state over an entire turn, where all na-
// tions get processed (as well as anything else that needs to
// happen each turn).
adt_s_rn_( TurnCycleState,                          //
           ( starting ),                            //
           ( inside,                                //
             ( bool, need_eot ),                    //
             ( Opt<e_nation>, nation ),             //
             ( flat_queue<e_nation>, remainder ) ), //
           ( ending,                                //
             ( bool, need_eot ) )                   //
);

adt_rn_( TurnCycleEvent, //
         ( next ),       //
         ( end )         //
);

// clang-format off
fsm_transitions( TurnCycle,
  ((starting, next),  ->,  inside   ),
  ((inside,   end),   ->,  ending   ),
  ((ending,   next),  ->,  starting )
);
// clang-format on

fsm_class( TurnCycle ) { //
  fsm_init( TurnCycleState::starting{} );

  fsm_transition_( TurnCycle, starting, next, ->, inside ) {
    TurnCycleState::inside res{
        /*need_eot=*/true, //
        /*nation=*/{},     //
        /*remainder=*/{}   //
    };
    CHECK( g_turn_ordering.size() > 0 );
    for( auto nation : g_turn_ordering )
      res.remainder.push( nation );
    return res;
  }

  fsm_transition( TurnCycle, inside, end, ->, ending ) {
    (void)event;
    return { /*need_eot=*/cur.need_eot };
  }
};

FSM_DEFINE_FORMAT_RN_( TurnCycle );

// Will be called repeatedly until done() is called.
void advance_turn_cycle_state( TurnCycleFsm&            fsm,
                               tl::function_ref<void()> done,
                               NationTurnFsm& nation_turn_fsm ) {
  switch_( fsm.mutable_state() ) {
    case_( TurnCycleState::starting ) {
      map_units( []( Unit& unit ) { unit.new_turn(); } );
      fsm.send_event( TurnCycleEvent::next{} );
    }
    case_( TurnCycleState::inside ) {
      bool has_nation = val.nation.has_value();
      bool has_next   = val.remainder.front().has_value();
      if( !has_nation ) {
        if( !has_next ) {
          if( val.need_eot ) {
            mark_end_of_turn();
            landview_do_eot();
          }
          fsm.send_event( TurnCycleEvent::end{} );
          break_;
        }
        val.nation = *val.remainder.front();
        val.remainder.pop();
        nation_turn_fsm =
            NationTurnFsm( NationTurnState::starting{} );
      }
      fsm_auto_advance( nation_turn_fsm, "nation-turn",
                        { advance_nation_turn_state },
                        *val.nation );
      if_v( nation_turn_fsm.state(), NationTurnState::ending,
            ending ) {
        val.nation = nullopt;
        val.need_eot &= ending->need_eot;
      }
      else {
        done();
      }
    }
    case_( TurnCycleState::ending ) {
      if( !val.need_eot ) {
        fsm.send_event( TurnCycleEvent::next{} );
      } else {
        if( was_next_turn_button_clicked() )
          fsm.send_event( TurnCycleEvent::next{} );
        else
          done();
      }
    }
    switch_exhaustive;
  }
}

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
  NationTurnFsm nation_turn;

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
  fsm_auto_advance( SG().cycle_state, "turn-cycle",
                    { advance_turn_cycle_state },
                    SG().nation_turn );
}

#if 0
e_turn_result turn( e_nation nation ) {
  // start of turn:

  lg.debug( "------ starting turn ({}) -------", nation );

  // Mark all units as not having moved.
  map_units( []( Unit& unit ) { unit.new_turn(); } );

  //  Iterate through the colonies, for each:
  //
  //    - advance state of the colony
  //
  //    - display messages to user any/or show animations where
  //      necessary
  //
  //    - allow them to enter colony when events happens; in that
  //      case go to the colony screen game loop. When the user
  //      exits the colony screen then this colony iteration im-
  //      mediately proceeds; i.e., user cannot enter any other
  //      colonies. This prevents the user from making
  //      last-minute changes to colonies that have not yet been
  //      advanced in this turn (otherwise that might allow
  //      cheating in some way).
  //
  //    - during this time, the user is not free to scroll map
  //      (menus?) or make any changes to units. They are also
  //      not allowed to enter colonies apart from the one that
  //      has just been processed.
  //
  //  Advance the state of the old world, possibly displaying
  //  messages to the user where necessary. clang-format on

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
             !unit.mv_pts_exhausted() ) {
        deduplicate_deque( &q );
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
              CHECK( !prio_unit.mv_pts_exhausted() );
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
      if( unit.mv_pts_exhausted() ||
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
#endif

} // namespace rn
