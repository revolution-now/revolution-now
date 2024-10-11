/****************************************************************
**rstring.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII holder for registry references to Lua
*              strings.
*
*****************************************************************/
#include "rstring.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace lua {

/****************************************************************
** rstring
*****************************************************************/
string rstring::as_cpp() const {
  c_api C( L_ );
  push( L_, *this );
  CHECK( C.type_of( -1 ) == type::string );
  UNWRAP_CHECK( res, C.get<string>( -1 ) );
  C.pop();
  return res;
}

bool rstring::operator==( char const* s ) const {
  return as_cpp() == s;
}

bool rstring::operator==( string_view s ) const {
  return as_cpp() == s;
}

bool rstring::operator==( string const& s ) const {
  return as_cpp() == s;
}

base::maybe<rstring> lua_get( cthread L, int idx,
                              tag<rstring> ) {
  lua::c_api C( L );
  if( C.type_of( idx ) != type::string ) {
    // We might be able to convert it to a string.
    base::maybe<string> ms = C.get<string>( idx );
    if( !ms ) return base::nothing;
    C.push( *ms );
  } else {
    // Copy the requested value to the top of the stack.
    C.pushvalue( idx );
  }
  // Then pop it into a registry reference.
  return rstring( L, C.ref_registry() );
}

void to_str( rstring const& o, std::string& out,
             base::tag<rstring> ) {
  to_str( o, out, base::tag<any>{} );
}

} // namespace lua
