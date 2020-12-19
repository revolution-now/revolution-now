/****************************************************************
**errors.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-04.
*
* Description: Error handling code.
*
*****************************************************************/
#include "errors.hpp"

// Revolution Now
#include "fmt-helper.hpp"
#include "stacktrace.hpp"

// base
#include "base/stack-trace.hpp"

using namespace std;

namespace rn {

void die( char const* file, int line, std::string_view msg ) {
  auto here = stack_trace_here();
  // Uncomment this to print a stack trace when a particular
  // error happens. This is useful during unit testing where
  // stack traces are not printed on exceptions.
  //
  // if( util::contains( msg, "<insert-string-here>" ) ) {
  //  backward::StackTrace const& st = *( here.st );
  //  backward::Printer           p;
  //  p.print( st, stderr );
  //}
  throw exception_with_bt(
      fmt::format( "{} ({}:{})", msg, file, line ),
      std::move( here ) );
}

namespace detail {

std::string check_msg( char const*        expr,
                       std::string const& msg ) {
  std::string suffix = msg.empty() ? "." : ( ": " + msg );
  return fmt::format( "CHECK( {} ) failed{}", expr, suffix );
}

bool check_inline( bool b, char const* msg ) {
  if( !b ) { FATAL_( detail::check_msg( msg, "" ) ); }
  return true;
}

} // namespace detail

} // namespace rn

namespace base {

// See description in base/stack-trace.hpp for what this is.
void abort_with_backtrace_here( SourceLoc /*loc*/ ) {
  auto here = ::rn::stack_trace_here();
  print_stack_trace(
      here, ::rn::StackTraceOptions{ .skip_frames = 4 } );
  std::abort();
}

} // namespace base
