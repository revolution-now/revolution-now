/****************************************************************
**ref.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-11.
*
* Description: RAII wrapper around a Lua registry reference.
*
*****************************************************************/
#include "ref.hpp"

// luapp
#include "c-api.hpp"

// base
#include "base/error.hpp"

// Lua
#include "lauxlib.h"

using namespace std;

namespace lua {

/****************************************************************
** reference
*****************************************************************/
reference::reference( cthread st, int ref ) noexcept
  : Base( ref ), L( st ) {
  DCHECK( ref != LUA_NOREF );
}

void reference::free_resource() {
  c_api C( L );
  C.unref_registry( resource() );
}

int reference::copy_resource() const {
  c_api C( L );
  C.registry_get( resource() );
  return C.ref_registry();
}

cthread reference::this_cthread() const noexcept { return L; }

bool operator==( reference const& lhs, reference const& rhs ) {
  // Need to use the same Lua state for both pushes in the
  // event that lhs and rhs have L's that correspond to different
  // threads.
  cthread L = lhs.this_cthread();
  push( L, lhs );
  push( L, rhs );
  c_api C( L );
  bool  res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

namespace {

template<typename T>
bool ref_op_eq( reference const& r, T const& o ) {
  c_api C( r.this_cthread() );
  push( r.this_cthread(), r );
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

void lua_push( cthread L, reference const& r ) {
  c_api C( L );
  int   ref = r.has_value() ? r.resource() : LUA_REFNIL;
  C.registry_get( ref );
}

void to_str( reference const& r, string& out ) {
  c_api C( r.this_cthread() );
  push( C, r );
  out += C.pop_tostring();
}

} // namespace lua
