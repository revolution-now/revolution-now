/****************************************************************
**co-scheduler.hpp
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

// C++ standard library
#include <coroutine>

namespace rn {

// Add the coroutine to the queue to be resumed.
void queue_cpp_coroutine_handle( std::coroutine_handle<> h );

// This will run all coroutines including the new coroutines that
// are queued in the process of running other coroutines.
void run_all_cpp_coroutines();

int number_of_queued_cpp_coroutines();

// This is to be called when a coroutine that has already been
// queued for running needs to be cancelled.
void remove_cpp_coroutine_if_queued( std::coroutine_handle<> h );

} // namespace rn
