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
#include "cast.hpp"
#include "cthread.hpp"
#include "error.hpp"
#include "ext.hpp"
#include "metatable-key.hpp"
#include "types.hpp"

// base
#include "base/error.hpp"
#include "base/fmt.hpp"
#include "base/macros.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <utility>

namespace lua {

template<Pushable Predecessor>
struct indexer_base {
  // Signal that objects of this type should not be treated as
  // any old user object.
  using luapp_internal = void;

  indexer_base( Predecessor pred )
    : pred_( std::move( pred ) ) {}

  cthread this_cthread() const noexcept {
    return pred_.this_cthread();
  }

 protected:
  Predecessor pred_;
};

template<typename IndexT, Pushable Predecessor>
struct indexer : indexer_base<Predecessor> {
  using Base = indexer_base<Predecessor>;

  using Base::this_cthread;

  explicit indexer( IndexT index, Predecessor pred )
    : Base( std::move( pred ) ), index_( index ) {}

  indexer( indexer&& ) noexcept = default;

  lua::type type() const noexcept {
    return type_of( this_cthread(), *this );
  }

  // When we assign to an indexer we don't want to copy/move the
  // indexer itself, we want it to trigger a copy action inside
  // Lua, so we use the more generic operator= further below. Ex-
  // ample:
  //
  //   st[1] = st[2];
  //
  // gcc seems to want to pick the default move assignment oper-
  // ator for this, which would not be what we want, and does not
  // work in some cases anyway some the pred_ and index_ members
  // can sometimes be references and so cannot be moved. So we
  // cannot use this default move assignment operator, but we
  // also cannot delete it, otherwise it would prevent us from
  // doing the above. So we just leave it commented:
  //
  //   indexer& operator=( indexer&& ) noexcept = delete;
  //
  // and let the compiler default to this one:
  //
  template<Pushable U>
  indexer& operator=( U&& rhs );

  indexer( indexer const& ) = delete;

  template<Pushable U>
  auto operator[]( U&& idx ) && noexcept {
    return indexer<U, indexer>( std::forward<U>( idx ),
                                std::move( *this ) );
  }

  auto operator[]( metatable_key_t ) && noexcept {
    return indexer<metatable_key_t const&, indexer>(
        metatable_key_t{}, std::move( *this ) );
  }

  template<Pushable IndexT_, Pushable Predecessor_>
  friend void lua_push(
      cthread L, indexer<IndexT_, Predecessor_> const& idxr );

  operator any() const noexcept;

  template<Pushable... Args>
  any operator()( Args&&... args );

  template<GettableOrVoid R = void, Pushable... Args>
  R call( Args&&... args );

  template<GettableOrVoid R = void, Pushable... Args>
  error_type_for_return_type<R> pcall( Args&&... args );

  template<typename T>
  T cast() const {
    return lua::cast<T>( *this );
  }

 private:
  IndexT index_;
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

// Gets the metatable for (-1) and pops (-1), leaving the metat-
// able, which might be nil.
void indexer_getmetatable( cthread L );

// (-2)[metatable] = (-1), and pops both.
void indexer_setmetatable( cthread L );

} // namespace internal

/****************************************************************
** Template implementations.
*****************************************************************/
template<Pushable IndexT, Pushable Predecessor>
void lua_push( cthread                             L,
               indexer<IndexT, Predecessor> const& idxr ) {
  push( L, idxr.pred_ );
  push( L, idxr.index_ );
  internal::indexer_gettable( L );
}

template<typename IndexT, Pushable Predecessor>
template<Pushable U>
indexer<IndexT, Predecessor>&
indexer<IndexT, Predecessor>::operator=( U&& rhs ) {
  cthread L = this_cthread();
  push( L, this->pred_ );
  push( L, index_ );
  push( L, std::forward<U>( rhs ) );
  internal::indexer_settable( L );
  return *this;
}

template<typename IndexT, Pushable Predecessor>
template<Pushable... Args>
any indexer<IndexT, Predecessor>::operator()( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<any>( L, FWD( args )... );
}

template<typename IndexT, Pushable Predecessor>
template<GettableOrVoid R, Pushable... Args>
R indexer<IndexT, Predecessor>::call( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_unsafe_and_get<R>( L, FWD( args )... );
}

template<typename IndexT, Pushable Predecessor>
template<GettableOrVoid R, Pushable... Args>
error_type_for_return_type<R>
indexer<IndexT, Predecessor>::pcall( Args&&... args ) {
  cthread L = this_cthread();
  push( L, *this );
  return call_lua_safe_and_get<R>( L, FWD( args )... );
}

template<typename IndexT, Pushable Predecessor>
indexer<IndexT, Predecessor>::operator any() const noexcept {
  cthread L = this_cthread();
  push( L, *this );
  // This should never fail regardless of C++ programmer error
  // or Lua programmer error.
  UNWRAP_CHECK( res, lua::get<any>( L, -1 ) );
  internal::indexer_pop( L, 1 );
  return res;
}

/****************************************************************
** metatable
*****************************************************************/
template<Pushable Predecessor>
struct indexer<metatable_key_t const&, Predecessor>
  : indexer_base<Predecessor> {
  using Base = indexer_base<Predecessor>;

  using Base::this_cthread;

  explicit indexer( metatable_key_t, Predecessor pred )
    : Base( std::move( pred ) ) {}

  lua::type type() const noexcept {
    return type_of( this_cthread(), *this );
  }

  friend void lua_push( cthread                     L,
                        indexer<metatable_key_t const&,
                                Predecessor> const& idxr ) {
    push( L, idxr.pred_ );
    internal::indexer_getmetatable( L );
  }

  template<Pushable U>
  auto operator[]( U&& idx ) && noexcept {
    return indexer<U, indexer>( std::forward<U>( idx ),
                                std::move( *this ) );
  }

  operator any() const noexcept {
    cthread L = this_cthread();
    push( L, *this );
    // This should never fail regardless of C++ programmer error
    // or Lua programmer error.
    UNWRAP_CHECK( res, lua::get<any>( L, -1 ) );
    internal::indexer_pop( L, 1 );
    return res;
  }

  template<Pushable U>
  indexer& operator=( U&& rhs ) {
    cthread L = this_cthread();
    push( L, this->pred_ );
    push( L, std::forward<U>( rhs ) );
    internal::indexer_setmetatable( L );
    return *this;
  }
};

/****************************************************************
** Equality
*****************************************************************/
// clang-format off
template<typename IndexT, Pushable Predecessor, Pushable U>
requires( Pushable<indexer<IndexT, Predecessor>> )
bool operator==( indexer<IndexT, Predecessor> const& idxr,
                 U const&                            rhs ) {
  // clang-format on
  cthread L = idxr.this_cthread();
  push( L, idxr );
  push( L, rhs );
  return internal::indexer_eq( L );
}

/****************************************************************
** any
*****************************************************************/
// This member function of the `any` class must be defined here
// in order to avoid circular header dependencies.
template<typename U>
auto any::operator[]( U&& idx ) const noexcept {
  return indexer<U, any>( std::forward<U>( idx ), *this );
}

} // namespace lua

EVAL( DEFINE_FORMAT_T( ( I, P ), (::lua::indexer<I, P>), "{}",
                       ::lua::any( o ) ) );
