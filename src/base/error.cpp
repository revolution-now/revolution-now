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
#include "stack-trace.hpp"

// C++ standard library
#include <string>

using namespace std;

namespace base {

namespace detail {

string check_msg( char const* expr, string const& msg ) {
  string suffix = msg.empty() ? "." : ( ": " + msg );
  return string( "CHECK( " ) + expr + " ) failed" + suffix;
}

} // namespace detail

void abort_with_msg( string_view msg, SourceLoc loc ) {
  fprintf( stderr, "%s:%d: error: %s\n", loc.file_name(),
           loc.line(), string( msg ).c_str() );
  abort_with_backtrace_here();
}

void to_str( generic_err const& ge, std::string& out, ADL_t ) {
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

string rethrow_and_get_msg( exception_ptr p ) {
  try {
    rethrow_exception( p );
  } catch( exception const& e ) {
    return e.what();
  } catch( ... ) { return "unknown exception type"; }
}

} // namespace base
