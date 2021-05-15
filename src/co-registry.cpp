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
#include <vector>

using namespace std;

namespace rn {

namespace {

queue<coro::coroutine_handle<>> g_coros_to_resume;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void queue_coroutine_handle( coro::coroutine_handle<> h ) {
  CHECK( h );
  g_coros_to_resume.push( h );
}

void run_all_coroutines() {
  while( !g_coros_to_resume.empty() ) {
    coro::coroutine_handle<> h = g_coros_to_resume.front();
    g_coros_to_resume.pop();
    h.resume();
    // May have added some more coroutines into the queue.
  }
}

void remove_coroutine_if_queued( coro::coroutine_handle<> h ) {
  queue<coro::coroutine_handle<>> coros_to_resume;
  while( !g_coros_to_resume.empty() ) {
    coro::coroutine_handle<> front = g_coros_to_resume.front();
    if( front != h ) coros_to_resume.push( front );
    g_coros_to_resume.pop();
  }
  g_coros_to_resume = std::move( coros_to_resume );
}

int number_of_queued_coroutines() {
  return int( g_coros_to_resume.size() );
}

} // namespace rn
