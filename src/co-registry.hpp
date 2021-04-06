/****************************************************************
**co-registry.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-10.
*
* Description: Coroutine handle queue for storing and running
*              coroutine continuations.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "co-handle.hpp"

namespace rn {

// Add the coroutine to the queue to be resumed.
void queue_coroutine_handle( unique_coro h );

// This will run all coroutines including the new coroutines that
// are queued in the process of running other coroutines.
void run_all_coroutines();

int number_of_queued_coroutines();

// This is to be called when a coroutine that has already been
// queued for running needs to be cancelled.
void destroy_queued_coroutine_handler(
    coro::coroutine_handle<> h );

// Since the coroutines are held in RAII wrappers, this would
// happen automatically on program termination, but this function
// is used to control more precisely when it happens during shut-
// down, since destroying a coroutine could potentially call the
// destructors of many other things and so it should probably
// happen before all the other systems are shut down.
//
// Actually, in practice, this will probably only result in a
// handful of coroutines getting freed, or possibly none at all,
// since all queued coroutines are run (and removed from the
// queue) each frame. So the only coroutines that would be de-
// stroyed here would be any new ones that were added in the
// frame just before program exit.
void destroy_all_coroutines();

} // namespace rn
