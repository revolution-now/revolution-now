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
  : L( st ), ref_( ref ) {}

reference::~reference() noexcept { release(); }

void reference::release() noexcept {
  CHECK( ref_ != LUA_NOREF );
  c_api C( L );
  C.unref_registry( ref_ );
}

reference::reference( reference const& rhs ) noexcept
  : L( rhs.L ) {
  push( L, rhs );
  c_api C( L );
  ref_ = C.ref_registry();
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
  push( L, rhs );
  c_api C( L );
  ref_ = C.ref_registry();
  return *this;
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

void push( cthread L, reference const& r ) {
  c_api C( L );
  C.registry_get( r.ref_ );
}

} // namespace lua
