/****************************************************************
**stack-trace.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-16.
*
* Description: Utilities for handling stack-traces.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// C++ standard library
#include <source_location>

// FIXME: move this to the error module after that is moved into
// the base library.
#ifndef NDEBUG
#  define assert_bt( ... ) \
    if( !( __VA_ARGS__ ) ) ::base::abort_with_backtrace_here();
#else
// This is in case there are any variables in there that would
// otherwise be unused in a release build and trigger warnings.
#  define assert_bt( ... ) \
    (void)( ( decltype( __VA_ARGS__ )* ){} );
#endif

namespace base {

// Each binary should implement this somewhere. It allows the
// base library to abort with stack trace without having to de-
// pend on any particular library that can do that. A binary must
// define this somewhere to avoid a linker error, but it need not
// necessarily print a stack trace if that is not available. But
// it should either call std::abort, or throw an exception, what-
// ever is most appropriate for the binary.
[[noreturn]] void abort_with_backtrace_here(
    std::source_location loc = std::source_location::current() );

} // namespace base
