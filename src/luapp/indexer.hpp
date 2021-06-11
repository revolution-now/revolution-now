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
#include "types.hpp"

namespace lua {

template<typename IndexT, typename Predecessor>
struct indexer {
  explicit indexer( IndexT index, Predecessor pred )
    : pred_( std::move( pred ) ), index_( index ) {}

  indexer( indexer&& ) noexcept = default;
  indexer& operator=( indexer&& ) noexcept = default;

  indexer( indexer const& ) = delete;

  template<typename U>
  auto operator[]( U&& idx ) && noexcept {
    return indexer<U, indexer>( std::forward<U>( idx ),
                                std::move( *this ) );
  }

  template<typename IndexT_, typename Precedessor_>
  friend void push( lua_State*                            L,
                    indexer<IndexT_, Precedessor_> const& idxr );

  lua_State* lua_state() const noexcept {
    return pred_.lua_state();
  }

  template<typename U>
  indexer& operator=( U&& rhs );

private:
  Predecessor pred_;
  IndexT      index_;
};

// Pushes (-2)[-1] onto the stack, and pops both table and key.
void indexer_gettable( lua_State* L );

// Pushes (-3)[-2] = (-1) onto the stack, and pops all three.
void indexer_settable( lua_State* L );

template<typename IndexT, typename Precedessor>
void push( lua_State*                          L,
           indexer<IndexT, Precedessor> const& idxr ) {
  push( L, idxr.pred_ );
  push( L, idxr.index_ );
  indexer_gettable( L );
}

template<typename IndexT, typename Predecessor>
template<typename U>
indexer<IndexT, Predecessor>&
indexer<IndexT, Predecessor>::operator=( U&& rhs ) {
  lua_State* L = lua_state();
  push( L, pred_ );
  push( L, index_ );
  push( L, std::forward<U>( rhs ) );
  indexer_settable( L );
  return *this;
}

} // namespace lua
