/****************************************************************
**any.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-17.
*
* Description: RAII wrapper around registry any to any Lua
*              type.
*
*****************************************************************/
#include "any.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lauxlib.h"

using namespace std;

namespace lua {

/****************************************************************
** any
*****************************************************************/
any::any( cthread st, int ref ) noexcept : Base( ref ), L( st ) {
  DCHECK( ref != LUA_NOREF );
}

void any::free_resource() {
  c_api C( L );
  C.unref_registry( resource() );
}

int any::copy_resource() const {
  c_api C( L );
  C.registry_get( resource() );
  return C.ref_registry();
}

cthread any::this_cthread() const noexcept { return L; }

namespace {

template<typename T>
bool ref_op_eq( any const& r, T const& o ) {
  c_api C( r.this_cthread() );
  push( r.this_cthread(), r );
  C.push( o );
  bool res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

} // namespace

namespace internal {

bool compare_top_two_and_pop( cthread L ) {
  c_api C( L );
  bool  res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

} // namespace internal

bool operator==( any const& r, nil_t o ) {
  return ref_op_eq( r, o );
}

bool operator==( any const& r, boolean const& o ) {
  return ref_op_eq( r, o );
}

bool operator==( any const& r, lightuserdata const& o ) {
  return ref_op_eq( r, o );
}

bool operator==( any const& r, integer const& o ) {
  return ref_op_eq( r, o );
}

bool operator==( any const& r, floating const& o ) {
  return ref_op_eq( r, o );
}

void lua_push( cthread L, any const& r ) {
  c_api C( L );
  int   ref = r.has_value() ? r.resource() : LUA_REFNIL;
  C.registry_get( ref );
}

base::maybe<any> lua_get( cthread L, int idx, tag<any> ) {
  lua::c_api C( L );
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return any( L, C.ref_registry() );
}

void to_str( any const& r, string& out ) {
  c_api C( r.this_cthread() );
  push( C, r );
  out += C.pop_tostring();
}

} // namespace lua
