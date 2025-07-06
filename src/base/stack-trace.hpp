/****************************************************************
**stack-trace.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-16.
*
* Description: Utilities for handling stack traces.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"

// C++ standard library
#include <functional>
#include <string_view>

// This decides when to enable stack traces in the build.
#ifdef ENABLE_CPP23_STACKTRACE
#  define STACK_TRACE_ON
#endif

#ifdef STACK_TRACE_ON
#  include <stacktrace>
#endif

namespace base {

#ifdef STACK_TRACE_ON
using StackTrace = std::stacktrace;
#else
struct StackTrace {};
#endif

// Get a stack at the location where this function is called;
// will include the stack from inside this function.
[[nodiscard]] StackTrace stack_trace_here();

// Controls which stack frames are printing based on where the
// associated source code file lives. This is used for e.g. sup-
// pressing stack frames from standard library code that is not
// relevant.
enum class e_stack_trace_frames {
  all,
  rn_only,
  rn_and_extern_only
};

// Options for customizing how stack traces are printing.
struct StackTraceOptions {
  // Skip the frames in the stack-trace module.
  bool skip_stacktrace_module = true;
  // Additional frames to skip after the skip_stacktrace_module
  // flag is applied (if true).
  int skip_frames             = 0;
  // Which folders do we care about.
  e_stack_trace_frames frames = e_stack_trace_frames::rn_only;
};

bool should_include_filepath_in_stacktrace(
    std::string_view path, e_stack_trace_frames frames );

// Print stack trace to stderr with sensible options and skip
// the latest `skip` number of frames.  That is, if stack
// traces have been enabled in the build.
void print_stack_trace( StackTrace const& st,
                        StackTraceOptions const& options = {} );

// This should be be used to register a callback to be called to
// cleanup the engine just before aborting the process, since
// otherwise the usual cleanup routines won't get run. This can
// lead to bad things such as midi synths not being stopped.
void register_cleanup_callback_on_abort(
    maybe<std::function<void()>> fn );

[[noreturn]] void abort_with_backtrace_here( int skip_frames );

} // namespace base
