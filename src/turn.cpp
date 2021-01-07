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
#include "colony-mgr.hpp"
#include "conductor.hpp"
#include "cstate.hpp"
#include "dispatch.hpp"
#include "fb.hpp"
#include "flat-deque.hpp"
#include "flat-queue.hpp"
#include "frame.hpp"
#include "fsm.hpp"
#include "land-view.hpp"
#include "logging.hpp"
#include "no-serial.hpp"
#include "panel.hpp" // FIXME
#include "render.hpp"
#include "sg-macros.hpp"
#include "sound.hpp"
#include "unit.hpp"
#include "ustate.hpp"
#include "variant.hpp"
#include "viewport.hpp"
#include "window.hpp"

// base
#include "base/lambda.hpp"

// Flatbuffers
#include "fb/sg-turn_generated.h"

// base-util
#include "base-util/algo.hpp"

// C++ standard library
#include <algorithm>
#include <deque>

using namespace std;

namespace rn {

DECLARE_SAVEGAME_SERIALIZERS( Turn );

namespace {

vector<e_nation> const g_turn_ordering{
    e_nation::english,
    e_nation::french,
    e_nation::dutch,
    e_nation::spanish,
};

template<typename T = monostate>
using SyncFutureSerial =
    no_serial<sync_future<T>, /*bFailOnSerialize=*/false>;

template<typename T = monostate>
using SyncFutureNoSerial =
    no_serial<sync_future<T>, /*bFailOnSerialize=*/true>;

} // namespace
} // namespace rn

// Rnl
#include "rnl/turn-unit.hpp"

namespace rn {

namespace {

// FIXME: Hack.
maybe<PlayerIntent> g_player_intent;

/****************************************************************
** Helpers
*****************************************************************/
bool animate_move( TravelAnalysis const& analysis ) {
  CHECK( holds<e_unit_travel_good>( analysis.desc ) );
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

sync_future<> kick_off_unit_animation(
    UnitId id, PlayerIntent const& intent ) {
  // Default future object that is born ready.
  auto def = make_sync_future<>();
  // Kick off animation if needed.
  return overload_visit(
      intent,
      [&]( TravelAnalysis const& val ) {
        if( !animate_move( val ) ) return def;
        UNWRAP_CHECK(
            d, val.move_src.direction_to( val.move_target ) );
        return landview_animate_move( id, d );
      },
      [&]( CombatAnalysis const& val ) {
        auto attacker = id;
        UNWRAP_CHECK( defender, val.target_unit );
        UNWRAP_CHECK( stats, val.fight_stats );
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
        return landview_animate_attack(
            attacker, defender, stats.attacker_wins, dp_anim );
      },
      [&]( auto const& ) { return def; } );
}

/****************************************************************
** Unit Turn FSM
*****************************************************************/
// This FSM represents the state across the processing of a
// single unit.
// clang-format off
fsm_transitions( UnitInput,
  ((none,             process     ),  ->,  processing       ),
  ((processing,       ask         ),  ->,  asking           ),
  ((processing,       put_response),  ->,  have_response    ),
  ((processing,       cancel      ),  ->,  none             ),
  ((asking,           put_response),  ->,  have_response    ),
  ((have_response,    execute     ),  ->,  executing_orders ),
  ((have_response,    process     ),  ->,  processing       ),
  ((executing_orders, cancel      ),  ->,  processing       ),
  ((executing_orders, end         ),  ->,  executed         ),
  ((executed,         process     ),  ->,  processing       )
);
// clang-format on

fsm_class( UnitInput ) { //
  fsm_init( UnitInputState::none{} );

  fsm_transition_( UnitInput, processing, ask, ->, asking ) {
    return { /*response=*/{} };
  }
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
    CHECK( cur.response->orders.has_value() );
    return { /*conf_anim=*/sync_future<bool>{},
             /*orders=*/*cur.response->orders };
  }
  fsm_transition( UnitInput, executing_orders, end, ->,
                  executed ) {
    (void)cur;
    return { /*add_to_front=*/std::move( event.add_to_front ) };
  }
};

FSM_DEFINE_FORMAT_RN_( UnitInput );

// Will be called repeatedly until no more events added to fsm.
void advance_unit_input_state( UnitInputFsm& fsm, UnitId id ) {
  switch( auto& v = fsm.mutable_state(); v.to_enum() ) {
    case UnitInputState::e::none: //
      break;
    case UnitInputState::e::processing: {
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
        break;
      }

      if( !is_unit_on_map_indirect( id ) ) {
        // TODO.
        unit.finish_turn();
        fsm.send_event( UnitInputEvent::cancel{} );
        break;
      }

      if( !unit.orders_mean_input_required() ||
          unit.mv_pts_exhausted() ) {
        unit.finish_turn();
        fsm.send_event( UnitInputEvent::cancel{} );
        break;
      }

      lg.debug( "asking orders for: {}", debug_string( id ) );

      auto maybe_orders = pop_unit_orders( id );
      if( maybe_orders.has_value() ) {
        UnitInputResponse response;
        response.orders = *maybe_orders;
        fsm.send_event( UnitInputEvent::put_response{
            /*response=*/std::move( response ) } );
        break;
      }

      fsm.send_event( UnitInputEvent::ask{} );
      break;
    }
    case UnitInputState::e::asking: {
      auto& val = v.get<UnitInputState::asking>();
      // sync_future could be empty in two situations: the first
      // time we pass through this code and just after deserial-
      // ization.
      if( val.response->empty() )
        val.response = landview_ask_orders( id );
      if( val.response->ready() )
        fsm.send_event( UnitInputEvent::put_response{
            /*response=*/val.response->get_and_reset() } );
      break;
    }
    case UnitInputState::e::have_response: //
      break;
    case UnitInputState::e::executing_orders: {
      auto& val     = v.get<UnitInputState::executing_orders>();
      bool  proceed = step_with_future<bool>(
          &val.conf_anim.o,
          /*init=*/
          [&] {
            CHECK( !g_player_intent.has_value() );
            auto maybe_intent = player_intent( id, val.orders );
            CHECK( maybe_intent.has_value(),
                   "no handler for orders {}", val.orders );
            g_player_intent = std::move( *maybe_intent );
            return confirm_explain( &*g_player_intent ) >>
                   [id, &fsm]( bool confirmed ) {
                     if( confirmed ) {
                       return kick_off_unit_animation(
                                  id, *g_player_intent ) >>
                              [&fsm]( auto ) {
                                // Animation (if any) is
                                // finished.
                                CHECK( g_player_intent );
                                affect_orders(
                                    *g_player_intent );
                                fsm.send_event( UnitInputEvent::end{
                                    .add_to_front =
                                        units_to_prioritize(
                                            *g_player_intent ) } );
                                g_player_intent = nothing;
                                // !! Unit may no longer exist
                                // here.
                                return make_sync_future<bool>(
                                    true );
                              };
                     } else {
                       fsm.send_event(
                           UnitInputEvent::cancel{} );
                       g_player_intent = nothing;
                       return make_sync_future<bool>( false );
                     }
                   };
          },
          /*when_ready=*/L( _ ) );
      if( !proceed ) break;
      ;
      break;
    }
    case UnitInputState::e::executed:
      // !! Unit may no longer exist here.
      break;
  }
}

/****************************************************************
** Nation Turn FSM
*****************************************************************/
} // namespace
} // namespace rn

#include "rnl/turn-nation.hpp"

namespace rn {
namespace {

// clang-format off
fsm_transitions( NationTurn,
  ((starting,    next),  ->,  colonies),
  ((colonies,    next),  ->,  doing_units),
  ((doing_units, end ),  ->,  ending     ),
);
// clang-format on

fsm_class( NationTurn ) {
  fsm_init( NationTurnState::ending{} );

  fsm_transition( NationTurn, starting, next, ->, colonies ) {
    (void)event;
    flat_queue<ColonyId> q;
    for( ColonyId colony_id : colonies_all( cur.nation ) )
      q.push( colony_id );
    return { /*q=*/std::move( q ) };
  }

  fsm_transition_( NationTurn, colonies, next, ->,
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

// Will be called repeatedly until no more events added to fsm.
void advance_nation_turn_state( NationTurnFsm& fsm,
                                e_nation       nation ) {
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
  switch( auto& nation_state = fsm.mutable_state();
          nation_state.to_enum() ) {
    case NationTurnState::e::starting: {
      print_bar( '-', fmt::format( "[ {} ]", nation ) );
      fsm.send_event( NationTurnEvent::next{} );
      break;
    }
    case NationTurnState::e::colonies: {
      auto& val = nation_state.get<NationTurnState::colonies>();
      lg.info( "processing colonies for the {}.", nation );
      while( !val.q.empty() ) {
        ColonyId colony_id = *val.q.front();
        val.q.pop();
        evolve_colony_one_turn( colony_id );
      }
      fsm.send_event( NationTurnEvent::next{} );
      break;
    }
    case NationTurnState::e::doing_units: {
      auto& val =
          nation_state.get<NationTurnState::doing_units>();
      auto& doing_units = val;
      auto  log_q       = [&] {
        lg.debug( "q: {}", doing_units.q.to_string( 3 ) );
      };
      if( doing_units.q.empty() ) {
        CHECK( !doing_units.uturn.has_value() );
        auto units = units_all( nation );
        util::sort_by_key( units,
                           []( auto id ) { return id._; } );
        erase_if( units,
                  L( unit_from_id( _ ).finished_turn() ) );
        for( auto id : units ) doing_units.q.push_back( id );
        log_q();
      }
      bool done_uturn = false;
      while( !done_uturn ) {
        if( doing_units.q.empty() ) break;
        auto id = *doing_units.q.front();
        // We need this check because units can be added into
        // the queue in this loop by user input.
        if( !unit_exists( id ) ||
            unit_from_id( id ).finished_turn() ) {
          doing_units.q.pop_front();
          log_q();
          doing_units.uturn = nothing;
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
        switch( auto& uturn_state = doing_units.uturn->state();
                uturn_state.to_enum() ) {
          case UnitInputState::e::none: {
            // All unit turns should end here.
            break;
          }
          case UnitInputState::e::processing: {
            FATAL(
                "should not be in this state after an advance "
                "call." );
            break;
          }
          case UnitInputState::e::asking: {
            doing_units.need_eot = false;
            done_uturn           = true;
            break;
          }
          case UnitInputState::e::have_response: {
            auto& val =
                uturn_state.get<UnitInputState::have_response>();
            for( auto id : val.response->add_to_back ) {
              doing_units.q.push_back( id );
              unit_from_id( id ).unfinish_turn();
            }
            auto const& maybe_orders = val.response->orders;
            auto const& add_to_front =
                val.response->add_to_front;
            // Can only have one or the other.
            CHECK( maybe_orders.has_value() ==
                   add_to_front.empty() );
            if( !add_to_front.empty() ) {
              for( auto id : add_to_front ) {
                doing_units.q.push_front( id );
                unit_from_id( id ).unfinish_turn();
              }
              log_q();
              // Send a process signal in case add_to_front ==
              // [id] (in which case we just keep asking the unit
              // for orders).
              doing_units.uturn->send_event(
                  UnitInputEvent::process{} );
              break;
            }
            UNWRAP_CHECK( orders, maybe_orders );
            if( holds<orders::wait>( orders ) ) {
              doing_units.q.push_back( id );
              CHECK( doing_units.q.front() == id );
              doing_units.q.pop_front();
              // fallthrough.
            }
            log_q();
            doing_units.uturn->send_event(
                UnitInputEvent::execute{} );
            break;
          }
          case UnitInputState::e::executing_orders: {
            done_uturn = true;
            break;
          }
          case UnitInputState::e::executed: {
            auto& val =
                uturn_state.get<UnitInputState::executed>();
            for( auto id : val.add_to_front ) {
              doing_units.q.push_front( id );
              unit_from_id( id ).unfinish_turn();
            }
            log_q();
            doing_units.uturn->send_event(
                UnitInputEvent::process{} );
            break;
          }
        }
        // There may be no queue front at this point if there was
        // only one unit in the queue and it died.
        if( !doing_units.q.front().has_value() ||
            doing_units.q.front() != id )
          doing_units.uturn = nothing;
      }
      if( doing_units.q.empty() )
        fsm.send_event( NationTurnEvent::end{} );
      break;
    }
    case NationTurnState::e::ending: //
      break;
  }
}

/****************************************************************
** Turn Cycle FSM
*****************************************************************/
} // namespace
} // namespace rn

#include "rnl/turn.hpp"

namespace rn {
namespace {

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
        /*need_eot=*/true,  //
        /*nation=*/{},      //
        /*nation_turn=*/{}, //
        /*remainder=*/{}    //
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

// Will be called repeatedly until no more events added to fsm.
void advance_turn_cycle_state( TurnCycleFsm& fsm ) {
  switch( auto& v = fsm.mutable_state(); v.to_enum() ) {
    case TurnCycleState::e::starting: {
      print_bar( '=', "[ Starting Turn ]" );
      map_units( []( Unit& unit ) { unit.new_turn(); } );
      fsm.send_event( TurnCycleEvent::next{} );
      break;
    }
    case TurnCycleState::e::inside: {
      auto& val        = v.get<TurnCycleState::inside>();
      bool  has_nation = val.nation.has_value();
      bool  has_next   = val.remainder.front().has_value();
      if( !has_nation ) {
        if( !has_next ) {
          fsm.send_event( TurnCycleEvent::end{} );
          break;
          ;
        }
        auto nation = val.remainder.front();
        val.nation  = nation;
        val.remainder.pop();
        val.nation_turn = NationTurnFsm(
            NationTurnState::starting{ .nation = *val.nation } );
      }
      fsm_auto_advance( val.nation_turn, "nation-turn",
                        { advance_nation_turn_state },
                        *val.nation );
      if_get( val.nation_turn.state(), NationTurnState::ending,
              ending ) {
        val.nation = nothing;
        val.need_eot &= ending.need_eot;
      }
      break;
    }
    case TurnCycleState::e::ending: {
      auto& val = v.get<TurnCycleState::ending>();
      if( !val.need_eot ) {
        fsm.send_event( TurnCycleEvent::next{} );
      } else {
        if( was_next_turn_button_clicked() )
          fsm.send_event( TurnCycleEvent::next{} );
        else
          mark_end_of_turn();
      }
      break;
    }
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

} // namespace

/****************************************************************
** Turn State Advancement
*****************************************************************/
void advance_turn_state() {
  fsm_auto_advance( SG().cycle_state, "turn-cycle",
                    { advance_turn_cycle_state } );
}

} // namespace rn
