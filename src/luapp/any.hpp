/****************************************************************
**any.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-17.
*
* Description: RAII wrapper around registry reference to any Lua
*              type.
*
*****************************************************************/
#pragma once

// luapp
#include "as.hpp"
#include "cthread.hpp"
#include "ext.hpp"

// base
#include "base/zero.hpp"

// C++ standard library
#include <string>

namespace lua {

/****************************************************************
** any
*****************************************************************/
struct any : base::zero<any, int> {
  // Signal that objects of this type should not be treated as
  // any old user object.
  using luapp_internal = void;

  any() = delete;
  any( cthread st, int ref ) noexcept;

  cthread this_cthread() const noexcept;

  friend bool operator==( any const& lhs, any const& rhs );

  // Pushes nil if there is no reference. Note that we don't push
  // onto the Lua state that is held inside the any object, since
  // that could correspond to a different thread.
  // clang-format off
  template<typename T>
  requires( std::is_base_of_v<any, T> )
  friend void lua_push( cthread L, T const& a ) {
    // clang-format on
    a.lua_push_impl( L );
  }

  friend base::maybe<any> lua_get( cthread L, int idx,
                                   tag<any> );

  // The body of this has to be defined in the indexer header in
  // order to avoid circular header dependencies.
  template<typename U>
  auto operator[]( U&& idx ) const noexcept;

  template<Gettable T>
  T as() const {
    return lua::as<T>( *this );
  }

  explicit operator bool() const;

 private:
  using Base = base::zero<any, int>;
  friend Base;

  // Implement base::zero.
  void free_resource();

  // Implement base::zero.
  int copy_resource() const;

 protected:
  void lua_push_impl( cthread L ) const;

  cthread L_; // not owned.
};

static_assert( Stackable<any> );
static_assert( LuappInternal<any> );

namespace internal {

// Asks Lua to compare the top to values on the stack and returns
// true if they are equal, and pops them.
bool compare_top_two_and_pop( cthread L );

} // namespace internal

// Need to exclude rhs types that derive from lua::any here oth-
// erwise there will be an ambiguity with the the operator==(
// any, any ) defined in the any class.
template<typename T>
requires( !std::is_convertible_v<T const&, any const&> )
bool operator==( any const& r, T const& rhs ) {
  cthread L = r.this_cthread();
  push( L, r );
  push( L, rhs );
  return internal::compare_top_two_and_pop( L );
}

void to_str( any const& r, std::string& out, base::ADL_t );

} // namespace lua
