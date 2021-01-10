/****************************************************************
**co-registry.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-10.
*
* Description: Coroutine handle queue for storing and running
*              coroutine continuations.
*
*****************************************************************/
#include "co-registry.hpp"

// Revolution Now
#include "error.hpp"
#include "logging.hpp"

// C++ standard library
#include <queue>

using namespace std;

namespace rn {

namespace {

queue<coro::coroutine_handle<>> g_coros;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void queue_coroutine_handle( coro::coroutine_handle<> h ) {
  CHECK( h );
  CHECK( !h.done() );
  g_coros.push( h );
}

int coroutines_in_queue() { return g_coros.size(); }

void run_next_coroutine_handle() {
  auto h = g_coros.front();
  g_coros.pop();
  CHECK( h );
  CHECK( !h.done() );
  lg.trace( "running coroutine continuation." );
  h();
  lg.trace( "finished running coroutine continuation." );
}

void run_all_coroutines() {
  while( coroutines_in_queue() > 0 ) run_next_coroutine_handle();
}

void destroy_all_coroutines() {
  while( !g_coros.empty() ) {
    auto& h = g_coros.front();
    h.destroy();
    CHECK( h );
    CHECK( !h.done() );
    g_coros.pop();
  }
}

} // namespace rn
