/****************************************************************
**co-lua-scheduler.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-07-19.
*
* Description: Scheduler for resuming Lua coroutines.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// luapp
#include "luapp/rthread.hpp"

namespace rn {

// Add the coroutine to the queue to be resumed.
void queue_lua_coroutine( lua::rthread th );

// This will run all coroutines including the new coroutines that
// are queued in the process of running other coroutines.
void run_all_lua_coroutines();

int number_of_queued_lua_coroutines();

// This is to be called when a coroutine that has already been
// queued for running needs to be cancelled.
void remove_lua_coroutine_if_queued( lua::rthread th );

} // namespace rn
