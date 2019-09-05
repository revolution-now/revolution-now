/****************************************************************
**fsm.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-25.
*
* Description: Finite State Machine.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "errors.hpp"
#include "flat-queue.hpp"

#include "base-util/pp.hpp"
#include "base-util/type-map.hpp"

// C++ standard library
#include <variant>

namespace rn {

template<typename ChildT, typename StateT, typename EventT,
         typename TransitionMap>
class fsm {
protected:
  using Parent = fsm<ChildT, StateT, EventT, TransitionMap>;

public:
  fsm() : state_{}, events_{} {
    state_ = child().initial_state();
  }

  // Queue an event, but do not process it immediately.
  void send_event( EventT const& event ) {
    events_.push( event );
  }

  // Queue an event, but do not process it immediately.
  void send_event( EventT&& event ) {
    events_.push_emplace( std::move( event ) );
  }

  // Process all pending events.
  void process_events() {
    while( auto maybe_event_ref = events_.front() ) {
      process_event( maybe_event_ref->get() );
      events_.pop();
    }
  }

  StateT const& state() const { return state_; }

  template<typename T>
  Opt<CRef<T>> holds() const {
    Opt<CRef<T>> res;
    if( auto* s = std::get_if<T>( state_ ); s != nullptr )
      res = *s;
    return res;
  }

protected:
  // Default transition function.
  template<typename S1, typename E, typename S2>
  static StateT transition( S1 const&, E const&, S2 const& ) {
    // Unfortunately it seems that empty structs still report
    // having a size of one, so we'll just assume that if the
    // size is one that it is empty.
    static_assert(
        sizeof( S2 ) <= 1,
        "should not default-construct a state with data "
        "members.  Please provide a custom override of "
        "`transition` to handle this case." );
    static_assert(
        sizeof( E ) <= 1,
        "cannot default-construct a target state from an event "
        "containing data members as this could be an indication "
        "of data loss.  Please provide a custom override of "
        "`transition` to handle this case." );
    return StateT{S2{}};
  }

private:
  static StateT initial_state() { return StateT{}; }

  ChildT const& child() const {
    return *static_cast<ChildT const*>( this );
  }
  ChildT& child() { return *static_cast<ChildT*>( this ); }

  void process_event( EventT const& event ) {
    auto visitor = [this]( auto const& state,
                           auto const& event ) -> StateT {
      using state1_t = std::decay_t<decltype( state )>;
      using event_t  = std::decay_t<decltype( event )>;
      using key_t    = std::pair<state1_t, event_t>;

      struct NoTransition {};
      using state2_t = Get<TransitionMap, key_t, NoTransition>;

      if constexpr( std::is_same_v<state2_t, NoTransition> ) {
        FATAL( "state {} cannot receive the event {}", state,
               event );
      } else {
        return child().transition( state, event, state2_t{} );
      }
    };
    state_ = std::visit( visitor, state_, event );
  }

  StateT             state_;
  flat_queue<EventT> events_;
};

/****************************************************************
** Transition Map Macros
*****************************************************************/
// Example:
//
//    using TestMap = FSM_TRANSITIONS( BallState, BallEvent,
//        ((none, start_spinning), /*->*/ spinning),
//        ((none, start_rolling),  /*->*/ rolling)
//    );

#define FSM_TRANSITIONS( ... ) \
  EVAL( FSM_TRANSITION_MAP_IMPL( __VA_ARGS__ ) )

#define FSM_TRANSITION_MAP_IMPL( state_t_name, event_t_name, \
                                 ... )                       \
  TypeMap<PP_MAP_TUPLE_COMMAS(                               \
      FSM_TO_KV_PAIR,                                        \
      PP_MAP_PREPEND2_TUPLE( state_t_name, event_t_name,     \
                             __VA_ARGS__ ) )>

#define FSM_TO_KV_PAIR( state_t_name, event_t_name,          \
                        state1_event, state2 )               \
  KV<FSM_TO_PAIR PREPEND_TUPLE2( state_t_name, event_t_name, \
                                 state1_event ),             \
     state_t_name::state2>

#define FSM_TO_PAIR( state_t_name, event_t_name, a, b ) \
  std::pair<state_t_name::a, event_t_name::b>

/****************************************************************
** Testing
*****************************************************************/
void test_fsm();

} // namespace rn
