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

// base
#include "base/co-compat.hpp"

namespace rn {

// Any coroutine that is stored somewhere should be registered
// here so that, if it is still alive upon program termination,
// it can be freed to avoid ASan reporting memory leaks. This
// will not cause the coroutine to be resumed.
void register_coroutine_handle( coro::coroutine_handle<> h );

// Add the coroutine to the queue to be resumed.
void queue_coroutine_handle( coro::coroutine_handle<> h );

int num_coroutines_in_queue();

void run_next_coroutine_handle();

void run_all_coroutines();

void destroy_all_coroutines();

} // namespace rn
