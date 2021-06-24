/****************************************************************
**indexer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-10.
*
* Description: Proxy for composing chains of index operators.
*
*****************************************************************/
#pragma once

// luapp
#include "any.hpp"
#include "call.hpp"
#include "cthread.hpp"
#include "error.hpp"
#include "ext.hpp"
#include "types.hpp"

// base
#include "base/error.hpp"
#include "base/macros.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <utility>

namespace lua {

template<typename IndexT, typename Predecessor>
struct indexer {
  // Signal that objects of this type should not be treated as
  // any old user object.
  using luapp_internal = void;

  explicit indexer( IndexT index, Predecessor pred )
    : pred_( std::move( pred ) ), index_( index ) {}

  indexer( indexer&& ) noexcept = default;
  indexer& operator=( indexer&& ) noexcept = default;

  indexer( indexer const& ) = delete;

  template<Pushable U>
  auto operator[]( U&& idx ) && noexcept {
    return indexer<U, indexer>( std::forward<U>( idx ),
                                std::move( *this ) );
  }

  template<typename IndexT_, typename Predecessor_>
  friend void lua_push(
      cthread L, indexer<IndexT_, Predecessor_> const& idxr );

  cthread this_cthread() const noexcept {
    return pred_.this_cthread();
  }

  lua::type type() const noexcept {
    return type_of( this_cthread(), *this );
  }

  template<Pushable U>
  indexer& operator=( U&& rhs );

  template<Pushable U>
  bool operator==( U const& rhs ) const noexcept;

  operator any() const noexcept;

  template<Pushable... Args>
  any operator()( Args&&... args );

  template<GettableOrVoid R = void, Pushable... Args>
  R call( Args&&... args );

  template<GettableOrVoid R = void, Pushable... Args>
  error_type_for_return_type<R> pcall( Args&&... args );

private:
  Predecessor pred_;
  IndexT      index_;
};

namespace internal {

// Pushes (-2)[-1] onto the stack, and pops both table and key.
void indexer_gettable( cthread L );

// Pushes (-3)[-2] = (-1) onto the stack, and pops all three.
void indexer_settable( cthread L );

// Asks lua if (-2) == (-1), and pops both.
bool indexer_eq( cthread L );

// Pops n values from stack.
void indexer_pop( cthread L, int n );

} // namespace internal

/****************************************************************
** Template implementations.
*****************************************************************/
template<typename IndexT, typename Predecessor>
void lua_push( cthread                             L,
               indexer<IndexT, Predecessor> const& idxr ) {
  push( L, idxr.pred_ );
  push( L, idxr.index_ );
  internal::indexer_gettable( L );
}

template<typename IndexT, typename Predecessor>
template<Pushable U>
indexer<IndexT, Predecessor>&
indexer<IndexT, Predecessor>::operator=( U&& rhs ) {
  cthread L = this_cthread();
  push( L, pred_ );
  push( L, index_ );
  push( L, std::forward<U>( rhs ) );
  internal::indexer_settable( L );
  return *this;
}

template<typename IndexT, typename Predecessor>
template<Pushable U>
bool indexer<IndexT, Predecessor>::operator==(
    U const& rhs ) const noexcept {
  cthread L = this_cthread();
  push( L, *this );
  push( L, rhs );
  return internal::indexer_eq( L );
}

template<typename IndexT, typename Predecessor>
template<Pushable... Args>
any indexer<IndexT, Predecessor>::operator()( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<any>( L, FWD( args )... );
}

template<typename IndexT, typename Predecessor>
template<GettableOrVoid R, Pushable... Args>
R indexer<IndexT, Predecessor>::call( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<R>( L, FWD( args )... );
}

template<typename IndexT, typename Predecessor>
template<GettableOrVoid R, Pushable... Args>
error_type_for_return_type<R>
indexer<IndexT, Predecessor>::pcall( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_safe_and_get<R>( L, FWD( args )... );
}

template<typename IndexT, typename Predecessor>
indexer<IndexT, Predecessor>::operator any() const noexcept {
  cthread L = this_cthread();
  push( L, *this );
  // This should never fail regardless of C++ programmer error
  // or Lua programmer error.
  UNWRAP_CHECK( res, lua::get<any>( L, -1 ) );
  internal::indexer_pop( L, 1 );
  return res;
}

} // namespace lua
