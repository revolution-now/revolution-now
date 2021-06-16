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

/****************************************************************
** thing
*****************************************************************/
type thing::type() const noexcept {
  switch( index() ) {
    case 0: return type::nil;
    case 1: return type::boolean;
    case 2: return type::lightuserdata;
    case 3: return type::number;
    case 4: return type::number;
    case 5: return type::string;
    case 6: return type::table;
    case 7: return type::function;
    case 8: return type::userdata;
    case 9: return type::thread;
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
  c_api     C( L );
  thing     res  = nil;
  lua::type type = C.type_of( -1 );
  switch( type ) {
    case type::nil: C.pop(); break;
    case type::boolean:
      res = C.get<boolean>( -1 );
      C.pop();
      break;
    case type::lightuserdata:
      res = *C.get<lightuserdata>( -1 );
      C.pop();
      break;
    case type::number:
      if( C.isinteger( -1 ) )
        res = *C.get<integer>( -1 );
      else
        res = *C.get<floating>( -1 );
      C.pop();
      break;
    case type::string:
      res = rstring( C.this_cthread(), C.ref_registry() );
      break;
    case type::table:
      res = table( C.this_cthread(), C.ref_registry() );
      break;
    case type::function:
      res = rfunction( C.this_cthread(), C.ref_registry() );
      break;
    case type::userdata:
      res = userdata( C.this_cthread(), C.ref_registry() );
      break;
    case type::thread:
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
      c_api C( scratch_state().thread.main().cthread() );
      L = C.this_cthread();
    }
  } );
  push( L, *this );
  return c_api( L ).pop_tostring();
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

void push_thing( cthread L, thing const& th ) {
  th.visit( [L]( auto const& o ) { push( L, o ); } );
}

/****************************************************************
** to_str
*****************************************************************/
void to_str( thing const& th, std::string& out ) {
  out += th.tostring();
}

} // namespace lua
