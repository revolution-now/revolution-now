/****************************************************************
**waitable.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-05-12.
*
* Description: Synchronous promise & future.
*
*****************************************************************/
#include "waitable.hpp"

// Revolution Now
#include "error.hpp"

using namespace std;

namespace rn {

namespace detail {

void AbortingChainLink::link_to_previous(
    AbortingChainLink& prev ) {
  previous = &prev;
  DCHECK( previous->ref_count >= 0 );
  ++previous->ref_count;
  if( was_aborted ) abort();
}

// The idea here is to run all the way to the end of the chain
// (to the root, most downstream) and call its callback function,
// which is expected to then cancel/reset the entire chain. Note
// that we don't call the callbacks on the intermediate links.
void AbortingChainLink::abort() {
  was_aborted = true;
  // Need to still run the function and call previous even if our
  // own `was_aborted` was true initially, just in case we are
  // being chained to a promise after having been aborted. In
  // particular, a coroutine could run and return a waitable that
  // is in the aborted state if it aborted eagerly, then someone
  // might attach a callback to its `abort_func`, then link it,
  // in which case we need to both call the function again and
  // call the previous links' functions.
  if( previous ) {
    DCHECK( previous->ref_count >= 0 );
    if( previous->ref_count > 0 ) {
      if( --previous->ref_count > 0 ) //
        // This means that we've come to a point where we have a
        // many-to-one relationship going to the previous node,
        // but not all of the upstream nodes have yet been
        // aborted, so we don't want to go further downstream.
        // However, we do want to eagerly abort (destroy) each
        // branch of that tree that itself aborts, in order to
        // not have to wait until the last branch has aborted. If
        // we don't do this, then a branch that aborts won't run
        // its destructors until all of the branches abort and/or
        // finish and the entire coroutine stack gets destroyed.
        // That could potentially work, but the decision here is
        // to just destroy each branch eagerly. So therefore, we
        // basically treat this node as a root node in the sense
        // that we're not going to traverse further downstream
        // (for now) and so we want to run the abort function so
        // that the branch (e.g. coroutine) associated with it
        // can be destroyed.
        goto found_root;
    }
    previous->abort();
    // !! At this point, this object may no longer exist, so we
    // must do nothing but return at this point.
    return;
  }
found_root:
  // Run only the last abort_func, which is normally expected to
  // cancel the entire coroutine chain and, in doing so, will
  // reset all of the AbortingChainLink objects in all of the
  // shared states, hence we should not do anything after this,
  // apart from setting was_aborted to true just in case it was
  // reset.
  abort_func();
  // At this point, this object will still exist, because this is
  // the last object in the chain, so it itself will not be de-
  // stroyed. And in that last object, we need to mark the fact
  // that we've aborted so that the user can call the .aborted()
  // method on the waitable. The above function call will have
  // reset it.
  was_aborted = true;
}

} // namespace detail

} // namespace rn
