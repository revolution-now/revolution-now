/****************************************************************
**thing.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-09.
*
* Description: C++ containers for Lua values/objects.
*
*****************************************************************/
#include "thing.hpp"

// base
#include "base/error.hpp"

using namespace std;

namespace luapp {

e_lua_type thing::type() const noexcept {
  switch( index() ) {
    case 0: return e_lua_type::nil;
    case 1: return e_lua_type::boolean;
    case 2: return e_lua_type::light_userdata;
    case 3: return e_lua_type::number;
    case 4: return e_lua_type::number;
    case 5: return e_lua_type::string;
    case 6: return e_lua_type::table;
    case 7: return e_lua_type::function;
    case 8: return e_lua_type::userdata;
    case 9: return e_lua_type::thread;
  }
  SHOULD_NOT_BE_HERE;
}

// Follows Lua's rules, where every value is true except for
// boolean:false and nil.
thing::operator bool() const noexcept {
  switch( index() ) {
    case 0:
      // nil
      return false;
    case 1:
      // bool
      return this->get<boolean>().get();
    default: //
      return true;
  }
}

void to_str( table const& o, std::string& out ) {
  (void)o;
  out += "<table>";
}

void to_str( lstring const& o, std::string& out ) {
  (void)o;
  out += "<string>";
}

void to_str( lfunction const& o, std::string& out ) {
  (void)o;
  out += "<function>";
}

void to_str( userdata const& o, std::string& out ) {
  (void)o;
  out += "<userdata>";
}

void to_str( lthread const& o, std::string& out ) {
  (void)o;
  out += "<thread>";
}

void to_str( lightuserdata const& o, std::string& out ) {
  out += fmt::format( "<lightuserdata:{}>", o.get() );
}

} // namespace luapp
