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

// C++ standard library
#include <variant>

namespace rn {

template<typename ChildT, typename StateT, typename EventT>
class fsm {
  fsm() : state_{}, events_{} {}

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
    while( auto maybe_event_ref = events_.front() )
      process_event( maybe_event_ref->get() );
  }

  StateT const& state() const { return state_; }

  template<typename T>
  Opt<CRef<T>> holds() const {
    Opt<CRef<T>> res;
    if( auto* s = std::get_if<T>( state_ ); s != nullptr )
      res = *s;
    return res;
  }

private:
  ChildT const& child() const {
    return *static_cast<ChildT const*>( this );
  }
  ChildT& child() { return *static_cast<ChildT*>( this ); }

  struct NoTransition {};

  StateT transition_auto( StateT const& state,
                          EventT const& event,
                          NoTransition const& ) {
    FATAL( "state {} cannot receive the event {}", state,
           event );
  }

  template<typename T>
  StateT transition_auto( StateT const& state,
                          EventT const& event,
                          T const&      new_state ) {
    return StateT{new_state};
  }

  StateT transition( StateT const& state, EventT const& event ) {

  }

  void process_event( EventT const& event ) {
    // auto transition = [this]( auto const& state,
    //                          auto const& event ) {
    //  return child().transition( state, event,
    //  Get<Transitionmap, ...> );
    //};
    // state_ = std::visit( transition, state_, event );

    // Then it will look for a function with this signature:
    //
    //   void action( State1_t const&, Event1_t const&, State2_t
    //   const& );
    //
    // and if one exists it will call it to perform an action.
  }

  StateT             state_;
  flat_queue<EventT> events_;
};

/****************************************************************
** Testing
*****************************************************************/
void test_fsm();

} // namespace rn
