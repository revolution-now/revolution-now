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

// base
#include "base/assoc-queue.hpp"
#include "base/error.hpp"

using namespace std;

namespace rn {

namespace {

// This associative queue, which is basically a list+map data
// structure that allows O(1) push, pop, and removal by value ap-
// pears to be noticeably faster than than using the alternatives
// (std::queue, std::set, set::unordered_set). This because we
// need to not only push/pop efficiently, but we need to be able
// to delete from the middle efficiently for when a coroutine
// gets destroyed.
//
// Because it is a kind of queue, it will ensure that coroutines
// get run in a FIFO manner. This does not actually seem neces-
// sary, since coroutines that are scheduled to run in the same
// frame are conceptually running asynchronously, and so should
// be tolerant of any (even non-deterministic) ordering. That
// said, the fact that we order them in a FIFO manner will guar-
// antee no starvation, just in case that ever becomes a concern
// (I don't believe that the code base currently has the kind of
// complex interaction among coroutines that would be necessary
// to make it a concern, but I don't know for sure).
base::AssociativeQueue<coroutine_handle<>> g_coros_to_resume;

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
  g_coros_to_resume.erase( h );
}

int number_of_queued_cpp_coroutines() {
  return g_coros_to_resume.size();
}

} // namespace rn
