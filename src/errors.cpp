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
