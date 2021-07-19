/****************************************************************
**co-lua-scheduler.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-19.
*
* Description: Scheduler for resuming Lua coroutines.
*
*****************************************************************/
#include "co-lua-scheduler.hpp"

// Revolution Now
#include "error.hpp"

// C++ standard library
#include <queue>

using namespace std;

namespace rn {

namespace {

queue<lua::rthread> g_lua_coros_to_resume;

} // namespace

/****************************************************************
** Public API
*****************************************************************/
void queue_lua_coroutine( lua::rthread th ) {
  CHECK( th.status() != lua::thread_status::err );
  g_lua_coros_to_resume.push( th );
}

void run_all_lua_coroutines() {
  while( !g_lua_coros_to_resume.empty() ) {
    lua::rthread th = g_lua_coros_to_resume.front();
    g_lua_coros_to_resume.pop();
    th.resume();
    // May have added some more coroutines into the queue or re-
    // moved some (due to coroutine cancellation).
  }
}

int number_of_queued_lua_coroutines() {
  return int( g_lua_coros_to_resume.size() );
}

void remove_lua_coroutine_if_queued( lua::rthread th ) {
  queue<lua::rthread> coros_to_resume;
  while( !g_lua_coros_to_resume.empty() ) {
    lua::rthread front = g_lua_coros_to_resume.front();
    if( front != th ) coros_to_resume.push( front );
    g_lua_coros_to_resume.pop();
  }
  g_lua_coros_to_resume = std::move( coros_to_resume );
}

} // namespace rn
