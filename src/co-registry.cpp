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

vector<coro::coroutine_handle<>> g_coros_registered;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void register_coroutine_handle( coro::coroutine_handle<> h ) {
  g_coros_registered.push_back( h );
}

void queue_coroutine_handle( coro::coroutine_handle<> h ) {
  CHECK( h );
  CHECK( !h.done() );
  g_coros_to_resume.push( h );
  // We can unregister this one now because we put it in the
  // queue to run. If it doesn't end up getting run then we will
  // destroy it from the resume queue. If it does get run, one of
  // two things will happen: 1) either it will finish and destroy
  // itself (in which case it doesn't need to be registered any-
  // more) or 2) it will suspend again and in doing so register
  // itself again.
  erase_if( g_coros_registered,
            [&]( coro::coroutine_handle<> h_ ) {
              return h.address() == h_.address();
            } );
}

int num_coroutines_in_queue() {
  return g_coros_to_resume.size();
}

void run_next_coroutine_handle() {
  auto h = g_coros_to_resume.front();
  g_coros_to_resume.pop();
  CHECK( h );
  CHECK( !h.done() );
  lg.trace( "running coroutine continuation." );
  h();
  lg.trace( "finished running coroutine continuation." );
}

void run_all_coroutines() {
  while( num_coroutines_in_queue() > 0 )
    run_next_coroutine_handle();
}

void destroy_all_coroutines() {
  while( !g_coros_to_resume.empty() ) {
    auto& h = g_coros_to_resume.front();
    CHECK( h );
    CHECK( !h.done() );
    h.destroy();
    g_coros_to_resume.pop();
  }
  if( g_coros_registered.size() > 0 ) {
    lg.warn(
        "destroying {} registered coroutines that have not "
        "finished running.",
        g_coros_registered.size() );
  }
  for( auto h : g_coros_registered ) {
    CHECK( h );
    CHECK( !h.done() );
    h.destroy();
  }
  g_coros_registered.clear();
}

} // namespace rn
