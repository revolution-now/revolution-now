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

// base
#include "source-loc.hpp"

namespace base {

// Each binary should implement this somewhere. It allows the
// base library to abort with stack trace without having to de-
// pend on any particular library that can do that. A binary must
// define this somewhere to avoid a linker error, but it need not
// necessarily print a stack trace if that is not available. But
// it should either call std::abort, or throw an exception, what-
// ever is most appropriate for the binary.
void abort_with_backtrace_here(
    SourceLoc loc = SourceLoc::current() );

} // namespace base
