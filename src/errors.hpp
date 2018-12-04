/****************************************************************
**errors.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-04.
*
* Description: Error handling code.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

#include "base-util/non-copyable.hpp"

#include <stdexcept>

// This decides when to enable stack traces in the build.
#ifndef NDEBUG
#define STACK_TRACE_ON
#endif

// Forward declare this so that we can expose a pointer to it but
// hide implementation so that we can backward.hpp.
#ifdef STACK_TRACE_ON
namespace backward {
class StackTrace;
}
#endif

namespace rn {

#ifdef STACK_TRACE_ON
struct StackTrace {
  // Pointer so that we can avoid including backward.hpp here.
  // We don't use unique_ptr here because then it requires us to
  // have a complete type. So this memory will be leaked, but
  // that's ok because we only use this on an error anyway.
  backward::StackTrace* st;
};
#else
struct StackTrace {};
#endif

struct exception_with_bt : public std::runtime_error {
  exception_with_bt( std::string msg, StackTrace st_ )
    : std::runtime_error( msg ), st{st_} {}
  StackTrace st; // will be empty in non-debug builds.
};

// All code in RN should use these functions to interact with
// stack traces.

// Get a stack at the location where this function is called;
// will include the stack from inside this function.
ND StackTrace stack_trace_here();

// Print stack trace to stderr with sensible options and skip
// the latest `skip` number of frames.  That is, if stack
// traces have been enabled in the build.
void print_stack_trace( StackTrace const& st, int skip = 0 );

} // namespace rn
