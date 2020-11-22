/****************************************************************
**stacktrace.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-22.
*
* Description: Handles printing of stack traces upon error.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
//#include "dummy.hpp"

// C++ standard library
#include <exception>
#include <memory>
#include <string>

// This decides when to enable stack traces in the build.
#ifndef NDEBUG
#  define STACK_TRACE_ON
#endif

// Forward declare this so that we can expose a pointer to it but
// hide implementation so that we can avoid including back-
// ward.hpp.
#ifdef STACK_TRACE_ON
namespace backward {
class StackTrace;
}
#endif

namespace rn {

#ifdef STACK_TRACE_ON
struct StackTrace {
  // We must define all of these StackTrace standard functions in
  // the cpp file since defining them here in the header would
  // require knowledge (in the header) of the unique_ptr's de-
  // structor (which is actually needed in the StackTrace con-
  // structor in case it throws an exception) which cannot be
  // generated in the header because we leave it as an incomplete
  // type here.
  StackTrace();
  ~StackTrace();
  StackTrace( std::unique_ptr<backward::StackTrace>&& st_ );
  // This is not defaulted because backward::StackTrace is only
  // forward declared in this header.
  StackTrace( StackTrace&& st );

  // Pointer so that we can avoid including backward.hpp here.
  std::unique_ptr<backward::StackTrace> st;
};
#else
struct StackTrace {};
#endif

struct exception_with_bt : public std::runtime_error {
  exception_with_bt( std::string msg, StackTrace st_ )
    : std::runtime_error( msg ), st( std::move( st_ ) ) {}
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
