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
#include "flat-queue.hpp"

namespace rn {

template<typename ChildT, typename StateT, typename EventT>
class fsm {
  fsm() : state_{}, event_queue_{} {}

  ChildT const& child() const {
    return *static_cast<ChildT const*>( this );
  }
  ChildT& child() { return *static_cast<ChildT*>( this ); }

  void queue_event( EventT const& ) {}

  Opt<EventT> next_event() {}

public:
  void reset() { state_ = StateT{}; }

private:
  StateT             state_;
  flat_queue<EventT> event_queue_;
};

/****************************************************************
** Testing
*****************************************************************/
void test_fsm();

} // namespace rn
