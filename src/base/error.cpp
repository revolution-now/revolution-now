/****************************************************************
**error.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-01-03.
*
* Description: Utilities for handling errors.
*
*****************************************************************/
#include "error.hpp"

// base
#include "cc-specific.hpp"
#include "stack-trace.hpp"

// C++ standard library
#include <source_location>
#include <string>

using namespace std;

namespace base {

namespace detail {

string check_msg( char const* expr, string const& msg ) {
  string suffix = msg.empty() ? "." : ( ": " + msg );
  return string( "CHECK( " ) + expr + " ) failed" + suffix;
}

} // namespace detail

void abort_with_msg( string_view msg, source_location loc ) {
  fprintf( stderr, "%s:%u: error: %s\n", loc.file_name(),
           loc.line(), string( msg ).c_str() );
  abort_with_backtrace_here( loc );
}

void to_str( generic_err const& ge, std::string& out,
             tag<generic_err> ) {
  if( ge == nullptr ) {
    out += "nullptr";
    return;
  }
  out += ge->loc.file_name();
  out += ':';
  out += to_string( ge->loc.line() );
  out += ':';
  out += " error: ";
  out += ge->what;
}

ExceptionInfo rethrow_and_get_info( exception_ptr const p ) {
  try {
    rethrow_exception( p );
  } catch( exception const& e ) {
    return ExceptionInfo{
      .demangled_type_name = demangle( typeid( e ).name() ),
      .msg                 = e.what() };
  } catch( ... ) {
    return ExceptionInfo{ .demangled_type_name = "unknown",
                          .msg = "unknown exception type" };
  }
}

} // namespace base
