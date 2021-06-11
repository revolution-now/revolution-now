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
string call_tostring( lua_State* L ) noexcept {
  c_api       C   = c_api::view( L );
  size_t      len = 0;
  char const* p   = C.tostring( -1, &len );
  string_view sv( p, len );
  string      res = string( sv );
  C.pop();
  return res;
}

/****************************************************************
** scratch lua state
*****************************************************************/
// The Lua state returned here should ONLY be used to do simple
// things such as compare value types. It should not be used to
// hold any objects and nothing in its state should be changed.
//
// This state is never reset or released.
c_api& scratch_state() {
  static c_api& C = []() -> c_api& {
    static c_api C = c_api::view( luaL_newstate() );
    // Kill the global table.
    C.push( nil );
    C.rawseti( LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS );
    CHECK( C.stack_size() == 0 );
    C.pushglobaltable();
    CHECK( C.type_of( -1 ) == e_lua_type::nil );
    C.pop();
    return C;
  }();
  return C;
}

} // namespace

/****************************************************************
** value types
*****************************************************************/
namespace {

template<typename Left, typename Right>
bool eq_value_and_value( Left const& l, Right const& r ) {
  c_api& C = scratch_state();
  C.push( l );
  C.push( r );
  bool res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

} // namespace

#define EQ_VAL_VAL_IMPL( left_t, right_t )               \
  bool operator==( left_t const& l, right_t const& r ) { \
    return eq_value_and_value( l, r );                   \
  }

EQ_VAL_VAL_IMPL( nil_t, boolean );
EQ_VAL_VAL_IMPL( nil_t, lightuserdata );
EQ_VAL_VAL_IMPL( nil_t, integer );
EQ_VAL_VAL_IMPL( nil_t, floating );
EQ_VAL_VAL_IMPL( boolean, lightuserdata );
EQ_VAL_VAL_IMPL( boolean, integer );
EQ_VAL_VAL_IMPL( boolean, floating );
EQ_VAL_VAL_IMPL( lightuserdata, integer );
EQ_VAL_VAL_IMPL( lightuserdata, floating );
EQ_VAL_VAL_IMPL( integer, floating );

/****************************************************************
** reference
*****************************************************************/
reference::reference( lua_State* st, int ref,
                      e_lua_type type ) noexcept
  : L( st ), ref_( ref ) {
#ifndef NDEBUG
  // In debug mode, check that we have the right type.
  c_api C = c_api::view( L );
  C.registry_get( ref_ );
  CHECK( C.type_of( -1 ) == type );
  C.pop();
#else
  (void)type;
#endif
}

reference::~reference() noexcept { release(); }

void reference::release() noexcept {
  CHECK( ref_ != LUA_NOREF );
  c_api C = c_api::view( L );
  C.unref_registry( ref_ );
}

reference::reference( reference const& rhs ) noexcept
  : L( rhs.L ) {
  rhs.push();
  c_api C = c_api::view( L );
  ref_    = C.ref_registry();
}

reference& reference::operator=(
    reference const& rhs ) noexcept {
  if( this == &rhs ) return *this;
  // If we're different objects then we should never be holding
  // the same reference, even if they refer to the same under-
  // lying object.
  CHECK( ref_ != rhs.ref_ );
  release();
  L = rhs.L;
  rhs.push();
  c_api C = c_api::view( L );
  ref_    = C.ref_registry();
  return *this;
}

int reference::noref() noexcept { return c_api::noref(); }

e_lua_type reference::push() const noexcept {
  c_api C = c_api::view( L );
  return C.registry_get( ref_ );
}

lua_State* reference::lua_state() const noexcept { return L; }

bool operator==( reference const& lhs, reference const& rhs ) {
  CHECK( lhs.lua_state() == rhs.lua_state(),
         "not sure if it is OK to push values with different "
         "states and then try to compare them.  Need to "
         "investigate this more." );
  lhs.push();
  rhs.push();
  c_api C   = c_api::view( lhs.lua_state() );
  bool  res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

namespace {

template<typename T>
bool ref_op_eq( reference const& r, T const& o ) {
  c_api C = c_api::view( r.lua_state() );
  r.push();
  C.push( o );
  bool res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

} // namespace

bool operator==( reference const& r, nil_t o ) {
  return ref_op_eq( r, o );
}

bool operator==( reference const& r, boolean const& o ) {
  return ref_op_eq( r, o );
}

bool operator==( reference const& r, lightuserdata const& o ) {
  return ref_op_eq( r, o );
}

bool operator==( reference const& r, integer const& o ) {
  return ref_op_eq( r, o );
}

bool operator==( reference const& r, floating const& o ) {
  return ref_op_eq( r, o );
}

/****************************************************************
** table
*****************************************************************/
table::table( lua_State* st, int ref ) noexcept
  : reference( st, ref, e_lua_type::table ) {}

/****************************************************************
** lstring
*****************************************************************/
lstring::lstring( lua_State* st, int ref ) noexcept
  : reference( st, ref, e_lua_type::string ) {}

string lstring::as_cpp() const {
  c_api C = c_api::view( L );
  push();
  CHECK( C.type_of( -1 ) == e_lua_type::string );
  UNWRAP_CHECK( res, C.get<string>( -1 ) );
  C.pop();
  return res;
}

bool lstring::operator==( char const* s ) const {
  return as_cpp() == s;
}

bool lstring::operator==( string_view s ) const {
  return as_cpp() == s;
}

bool lstring::operator==( string const& s ) const {
  return as_cpp() == s;
}

/****************************************************************
** lfunction
*****************************************************************/
lfunction::lfunction( lua_State* st, int ref ) noexcept
  : reference( st, ref, e_lua_type::function ) {}

/****************************************************************
** userdata
*****************************************************************/
userdata::userdata( lua_State* st, int ref ) noexcept
  : reference( st, ref, e_lua_type::userdata ) {}

/****************************************************************
** lthread
*****************************************************************/
lthread::lthread( lua_State* st, int ref ) noexcept
  : reference( st, ref, e_lua_type::thread ) {}

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

void thing::push( lua_State* L ) const noexcept {
  this->visit( [&]<typename T>( T&& o ) {
    if constexpr( is_base_of_v<reference, remove_cvref_t<T>> ) {
      CHECK( o.lua_state() == L );
      o.push();
    } else {
      c_api C = c_api::view( L );
      C.push( o );
    }
  } );
}

thing thing::pop( lua_State* L ) noexcept {
  c_api      C    = c_api::view( L );
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
      res = lstring( C.state(), C.ref_registry() );
      break;
    case e_lua_type::table:
      res = table( C.state(), C.ref_registry() );
      break;
    case e_lua_type::function:
      res = lfunction( C.state(), C.ref_registry() );
      break;
    case e_lua_type::userdata:
      res = userdata( C.state(), C.ref_registry() );
      break;
    case e_lua_type::thread:
      res = lthread( C.state(), C.ref_registry() );
      break;
  }
  DCHECK( res.type() == type, "{} not equal to {}", res.type(),
          type );
  return res;
}

string thing::tostring() const noexcept {
  lua_State* L = nullptr;
  this->visit( [&]<typename T>( T&& o ) {
    if constexpr( is_base_of_v<reference, remove_cvref_t<T>> ) {
      L = o.lua_state();
    } else {
      c_api& C = scratch_state();
      L        = C.state();
    }
  } );
  push( L );
  return call_tostring( L );
}

bool thing::operator==( thing const& rhs ) const noexcept {
  if( this == &rhs ) return true;
  return std::visit( L2( _1 == _2 ), this->as_std(),
                     rhs.as_std() );
}

bool thing::operator==( std::string_view rhs ) const noexcept {
  auto maybe_s = get_if<lstring>();
  if( !maybe_s.has_value() ) return false;
  return *maybe_s == rhs;
}

/****************************************************************
** to_str
*****************************************************************/
void to_str( reference const& r, string& out ) {
  r.push();
  out += call_tostring( r.lua_state() );
}

void to_str( thing const& th, std::string& out ) {
  out += th.tostring();
}

} // namespace lua
