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
any::any( cthread st, int ref ) noexcept
  : Base( ref ), L_( st ) {
  DCHECK( ref != LUA_NOREF );
}

void any::free_resource() {
  c_api C( L_ );
  C.unref_registry( resource() );
}

int any::copy_resource() const {
  c_api C( L_ );
  C.registry_get( resource() );
  return C.ref_registry();
}

cthread any::this_cthread() const noexcept { return L_; }

void any::lua_push_impl( cthread L ) const {
  c_api C( L );
  int   ref = this->has_value() ? this->resource() : LUA_REFNIL;
  C.registry_get( ref );
}

any::operator bool() const { return as<bool>(); }

namespace internal {

bool compare_top_two_and_pop( cthread L ) {
  c_api C( L );
  bool  res = C.compare_eq( -2, -1 );
  C.pop( 2 );
  return res;
}

} // namespace internal

base::maybe<any> lua_get( cthread L, int idx, tag<any> ) {
  lua::c_api C( L );
  // Copy the requested value to the top of the stack.
  C.pushvalue( idx );
  // Then pop it into a registry reference.
  return any( L, C.ref_registry() );
}

void to_str( any const& r, string& out, base::ADL_t ) {
  c_api C( r.this_cthread() );
  push( C, r );
  out += C.pop_tostring();
}

bool operator==( any const& lhs, any const& rhs ) {
  cthread L = lhs.this_cthread();
  push( L, lhs );
  push( L, rhs );
  return internal::compare_top_two_and_pop( L );
}

} // namespace lua
