/****************************************************************
**co-scheduler.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-10.
*
* Description: Coroutine handle queue for storing and running
*              coroutine continuations.
*
*****************************************************************/
#include "co-scheduler.hpp"

// Revolution Now
#include "error.hpp"

// C++ standard library
#include <queue>

using namespace std;

namespace rn {

namespace {

queue<coroutine_handle<>> g_coros_to_resume;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void queue_cpp_coroutine_handle( coroutine_handle<> h ) {
  CHECK( h );
  g_coros_to_resume.push( h );
}

void run_all_cpp_coroutines() {
  while( !g_coros_to_resume.empty() ) {
    coroutine_handle<> h = g_coros_to_resume.front();
    g_coros_to_resume.pop();
    h.resume();
    // May have added some more coroutines into the queue or re-
    // moved some (due to coroutine cancellation).
  }
}

void remove_cpp_coroutine_if_queued( coroutine_handle<> h ) {
  queue<coroutine_handle<>> coros_to_resume;
  while( !g_coros_to_resume.empty() ) {
    coroutine_handle<> front = g_coros_to_resume.front();
    if( front != h ) coros_to_resume.push( front );
    g_coros_to_resume.pop();
  }
  g_coros_to_resume = std::move( coros_to_resume );
}

int number_of_queued_cpp_coroutines() {
  return int( g_coros_to_resume.size() );
}

} // namespace rn
