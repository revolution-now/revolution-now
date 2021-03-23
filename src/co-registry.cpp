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

queue<unique_coro> g_coros_to_resume;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void queue_coroutine_handle( unique_coro h ) {
  CHECK( h );
  g_coros_to_resume.push( std::move( h ) );
}

void run_all_coroutines() {
  while( !g_coros_to_resume.empty() ) {
    // Should not need to pop before running; even if resuming
    // the coroutine causes additional items to be queued (which
    // it can) they will be queued at the back.
    g_coros_to_resume.front().release_and_resume();
    g_coros_to_resume.pop();
  }
}

void destroy_all_coroutines() {
  lg.debug( "destroying all queued coroutines: {}",
            g_coros_to_resume.size() );
  while( !g_coros_to_resume.empty() ) g_coros_to_resume.pop();
}

} // namespace rn
