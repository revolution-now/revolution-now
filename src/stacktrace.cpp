/****************************************************************
**stacktrace.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-22.
*
* Description: Handles printing of stack traces upon error.
*
*****************************************************************/
#include "stacktrace.hpp"

// Revolution Now
//#include "dummy.hpp"

// C++ standard library
#include <iostream>

// backward
#ifdef STACK_TRACE_ON
#  include "backward.hpp"
#endif

using namespace std;

namespace rn {

#ifdef STACK_TRACE_ON
StackTrace::StackTrace() {}

StackTrace::~StackTrace() {}

StackTrace::StackTrace( unique_ptr<backward::StackTrace>&& st_ )
  : st( std::move( st_ ) ) {}

StackTrace::StackTrace( StackTrace&& st_ )
  : st( std::move( st_.st ) ) {}
#endif

ND StackTrace stack_trace_here() {
#ifdef STACK_TRACE_ON
  auto st = make_unique<backward::StackTrace>();
  st->load_here( 32 );
  return StackTrace( std::move( st ) );
#else
  return StackTrace{};
#endif
}

void print_stack_trace( StackTrace const& st_, int skip ) {
#ifdef STACK_TRACE_ON
  backward::StackTrace st = *( st_.st );
  // Skip uninteresting stack frames
  st.skip_n_firsts( skip );
  backward::Printer p;
  p.print( st, stderr );
#else
  (void)st_;
  (void)skip;
  cerr << "(stack trace unavailable: binary built without "
          "support for it)\n";
#endif
}

} // namespace rn
