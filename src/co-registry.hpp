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

void queue_coroutine_handle( coro::coroutine_handle<> h );

int coroutines_in_queue();

void run_next_coroutine_handle();

void run_all_coroutines();

void destroy_all_coroutines();

} // namespace rn
