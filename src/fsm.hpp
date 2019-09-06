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
#include "adt.hpp"
#include "errors.hpp"
#include "flat-queue.hpp"

#include "base-util/pp.hpp"
#include "base-util/type-map.hpp"

// C++ standard library
#include <variant>

namespace rn {

/****************************************************************
** Finite State Machine
*****************************************************************/
// Example:
//
//   adt_rn_( ColorState,
//            ( red ),
//            ( light_red ),
//            ( dark_red )
//   );
//
//   adt_rn_( ColorEvent,
//            ( light ),
//            ( dark )
//   );
//
//   fsm_transitions(
//       Color
//      ,((red,          light),  /*->*/  light_red   )
//      ,((red,          dark ),  /*->*/  dark_red    )
//      ,((light_red,    dark ),  /*->*/  red         )
//      ,((dark_red,     light),  /*->*/  red         )
//   );
//
//   fsm_class( Color ) {
//     fsm_init( ColorState::red{} );
//
//     // Optionally can override these to perform actions
//     // during a transition.  Overriding these are mandatory
//     // if either the event or target state has data members.
//     fsm_transition( Color, blue, rotate, ->, yellow ) {
//       cout << "Going from blue to yellow via rotate.\n";
//       return {};
//     }
//   };
//
//   ColorFsm color;
//   lg.info( "color state: {}", color.state() );
//
//   color.send_event( ColorEvent::light{} );
//   color.process_events();
//   lg.info( "color state: {}", color.state() );
//
//   color.send_event( ColorEvent::dark{} );
//   color.process_events();
//   lg.info( "color state: {}", color.state() );
//
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

  bool has_pending_events() const { return !events_.empty(); }

  CRef<StateT> state() const { return state_; }

  // !! No pointer stability here; state could change after
  //    calling another non-const method.
  template<typename T>
  Opt<CRef<T>> holds() const {
    Opt<CRef<T>> res;
    if( auto* s = std::get_if<T>( &state_ ); s != nullptr )
      res = *s;
    return res;
  }

protected:
  template<typename T>
  struct FsmTag {
    using type = T;
  };

  // Default transition function.
  template<typename S1, typename E, typename S2>
  static S2 transition( S1 const&, E const&,
                        FsmTag<S2> const& ) {
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
    return S2{};
  }

private:
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
        // Maybe in the future, at least for release builds, we
        // will want to avoid throwing here and just emit an
        // error and return the current state (i.e., leave state
        // unchanged).
        FATAL( "state {} cannot receive the event {}", state,
               event );
      } else {
        if constexpr(
            !std::is_same_v<
                std::decay_t<decltype( child().transition(
                    state, event, FsmTag<state2_t>{} ) )>,
                state2_t> ) {
          struct
              one_of_your_transition_functions_returns_the_wrong_type {
          };
          // If you get an error on the line below, then one of
          // your transition functions does not return the right
          // type. A transition function must return the type of
          // the new state (not the state variant).
          return one_of_your_transition_functions_returns_the_wrong_type{};
        } else {
          return child().transition( state, event,
                                     FsmTag<state2_t>{} );
        }
      }
    };
    state_ = std::visit( visitor, state_, event );
  }

  StateT             state_;
  flat_queue<EventT> events_;
};

/****************************************************************
** Macros
*****************************************************************/
#define fsm_transitions( ... ) \
  EVAL( FSM_TRANSITION_MAP_IMPL( __VA_ARGS__ ) )

#define FSM_TRANSITION_MAP_IMPL( name, ... )             \
  using name##FsmTransitions = FSM_TRANSITION_MAP_IMPL2( \
      name##State, name##Event, __VA_ARGS__ )

#define FSM_TRANSITION_MAP_IMPL2( state_t_name, event_t_name, \
                                  ... )                       \
  TypeMap<PP_MAP_TUPLE_COMMAS(                                \
      FSM_TO_KV_PAIR,                                         \
      PP_MAP_PREPEND2_TUPLE( state_t_name, event_t_name,      \
                             __VA_ARGS__ ) )>

#define FSM_TO_KV_PAIR( state_t_name, event_t_name,          \
                        state1_event, dummy, state2 )        \
  KV<FSM_TO_PAIR PREPEND_TUPLE2( state_t_name, event_t_name, \
                                 state1_event ),             \
     state_t_name::state2>

#define FSM_TO_PAIR( state_t_name, event_t_name, a, b ) \
  std::pair<state_t_name::a, event_t_name::b>

#define fsm_class( name )                                 \
  struct name##Fsm                                        \
    : public fsm<name##Fsm, name##State_t, name##Event_t, \
                 name##FsmTransitions>

#define fsm_init( a )       \
  using Parent::transition; \
  auto initial_state() const { return a; }

#define fsm_transition( fsm_name, start, e, dummy, end )   \
  static_assert(                                           \
      std::is_same_v<fsm_name##State::end,                 \
                     Get<fsm_name##FsmTransitions,         \
                         std::pair<fsm_name##State::start, \
                                   fsm_name##Event::e>>>,  \
      "this transition is not in the transitions map" );   \
  fsm_name##State::end transition(                         \
      fsm_name##State::start const&,                       \
      fsm_name##Event::e const& event,                     \
      FsmTag<fsm_name##State::end> )

/****************************************************************
** Testing
*****************************************************************/
void test_fsm();

} // namespace rn
