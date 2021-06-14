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
#include "error.hpp"

using namespace std;

namespace lua {

/****************************************************************
** rstring
*****************************************************************/
string rstring::as_cpp() const {
  c_api C( L );
  push( L, *this );
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

} // namespace lua
