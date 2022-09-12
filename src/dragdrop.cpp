/****************************************************************
**dragdrop.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-20.
*
* Description: A framework for drag & drop of entities.
*
*****************************************************************/
#include "dragdrop.hpp"

// Revolution Now
#include "co-combinator.hpp"
#include "input.hpp"

namespace rn {

namespace {

using namespace std;

} // namespace

namespace detail {

wait<> eat_remaining_drag_events(
    co::stream<input::event_t>& input ) {
  while( true ) {
    input::event_t event = co_await input.next();
    auto drag = event.get_if<input::mouse_drag_event_t>();
    if( !drag ) continue;
    CHECK( drag->state.phase != input::e_drag_phase::begin )
    if( drag->state.phase != input::e_drag_phase::end ) continue;
    break;
  }
}

} // namespace detail

} // namespace rn
