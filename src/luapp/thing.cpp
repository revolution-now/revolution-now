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

// luapp
#include "c-api.hpp"
#include "scratch.hpp"
#include "state.hpp"

// base
#include "base/error.hpp"
#include "base/lambda.hpp"

// Lua
#include "lauxlib.h"
#include "lua.h"

using namespace std;

#undef L

namespace lua {

namespace {

// Expects value to be pushed onto stack of L.
string call_tostring( cthread L ) noexcept {
  c_api       C( L );
  size_t      len = 0;
  char const* p   = C.tostring( -1, &len );
  string_view sv( p, len );
  string      res = string( sv );
  C.pop();
  return res;
}

} // namespace

/****************************************************************
** thing
*****************************************************************/
e_lua_type thing::type() const noexcept {
  switch( index() ) {
    case 0: return e_lua_type::nil;
    case 1: return e_lua_type::boolean;
    case 2: return e_lua_type::lightuserdata;
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

thing thing::pop( cthread L ) noexcept {
  c_api      C( L );
  thing      res  = nil;
  e_lua_type type = C.type_of( -1 );
  switch( type ) {
    case e_lua_type::nil: C.pop(); break;
    case e_lua_type::boolean:
      res = C.get<boolean>( -1 );
      C.pop();
      break;
    case e_lua_type::lightuserdata:
      res = *C.get<lightuserdata>( -1 );
      C.pop();
      break;
    case e_lua_type::number:
      if( C.isinteger( -1 ) )
        res = *C.get<integer>( -1 );
      else
        res = *C.get<floating>( -1 );
      C.pop();
      break;
    case e_lua_type::string:
      res = rstring( C.this_cthread(), C.ref_registry() );
      break;
    case e_lua_type::table:
      res = table( C.this_cthread(), C.ref_registry() );
      break;
    case e_lua_type::function:
      res = rfunction( C.this_cthread(), C.ref_registry() );
      break;
    case e_lua_type::userdata:
      res = userdata( C.this_cthread(), C.ref_registry() );
      break;
    case e_lua_type::thread:
      res = rthread( C.this_cthread(), C.ref_registry() );
      break;
  }
  DCHECK( res.type() == type, "{} not equal to {}", res.type(),
          type );
  return res;
}

string thing::tostring() const noexcept {
  cthread L = nullptr;
  this->visit( [&]<typename T>( T&& o ) {
    if constexpr( is_base_of_v<reference, remove_cvref_t<T>> ) {
      L = o.this_cthread();
    } else {
      c_api C( scratch_state().thread.main.cthread() );
      L = C.this_cthread();
    }
  } );
  push( L, *this );
  return call_tostring( L );
}

bool thing::operator==( thing const& rhs ) const noexcept {
  if( this == &rhs ) return true;
  return std::visit( L2( _1 == _2 ), this->as_std(),
                     rhs.as_std() );
}

bool thing::operator==( std::string_view rhs ) const noexcept {
  auto maybe_s = get_if<rstring>();
  if( !maybe_s.has_value() ) return false;
  return *maybe_s == rhs;
}

void push( cthread L, thing const& th ) {
  th.visit( [L]( auto const& o ) { push( L, o ); } );
}

/****************************************************************
** to_str
*****************************************************************/
void to_str( reference const& r, string& out ) {
  push( r.this_cthread(), r );
  out += call_tostring( r.this_cthread() );
}

void to_str( thing const& th, std::string& out ) {
  out += th.tostring();
}

} // namespace lua
