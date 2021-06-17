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
#include "cthread.hpp"
#include "error.hpp"
#include "ext.hpp"
#include "types.hpp"

// base
#include "base/cc-specific.hpp"
#include "base/error.hpp"
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

  template<Pushable U>
  indexer& operator=( U&& rhs );

  template<Gettable U>
  U as() const;

  template<Pushable U>
  bool operator==( U const& rhs ) const noexcept;

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
template<Gettable U>
U indexer<IndexT, Predecessor>::as() const {
  cthread L = this_cthread();
  push( L, *this );
  // The indexer only ever occupies one stack slot, since pushing
  // an indexer is always pushing a single Lua value.
  static_assert( nvalues_for<U>() == 1 );
  base::maybe<U> res = lua::get<U>( L, -1 );
  if( !res.has_value() )
    throw_lua_error(
        L, "expected type `{}' but received type `{}' from Lua.",
        base::demangled_typename<U>(), type_of( L, -1 ) );
  internal::indexer_pop( L, 1 );
  return *res;
}

} // namespace lua
