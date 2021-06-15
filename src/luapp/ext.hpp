/****************************************************************
**ext.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-15.
*
* Description: Declarations needed by types that want to use the
*              luapp extension points.
*
*****************************************************************/
#pragma once

// luapp
#include "cthread.hpp"

// base
#include "base/maybe.hpp"

// C++ standard library
#include <type_traits>

namespace lua {

// NOTE: The stuff in this header file should be limited to the
// minimum necessary that another (non-luapp) header needs to in-
// clude in order to declare extension points for a custom type.
// It should not be necessary to include any other headers from
// the luapp library in the header file that declares the type in
// question, although the implemention of those extension point
// methods in its cpp file might rely on other facilities from
// luapp that it can pull in.

/****************************************************************
** Macros
*****************************************************************/
// This macro is used to define a push function that can only be
// called for a specific type, preventing issues with ambiguious
// overloads due to implicit conversions.
#define LUA_PUSH_FUNC( type )                                   \
  template<typename T>                                          \
  requires( std::is_same_v<std::remove_cvref_t<T>, type> ) void \
  lua_push( cthread L, T const& o )

/****************************************************************
** Tag used throughout luapp.
*****************************************************************/
// This is used to help select function overloads that are in
// different namespaces (and hence where specializing a function
// template won't work).
template<typename T>
struct tag {};

/****************************************************************
** type traits
*****************************************************************/
template<typename T>
struct default_traits {
  // This should only be a base type.
  static_assert( !std::is_reference_v<T> );
  static_assert( !std::is_const_v<T> );

  static constexpr int nvalues = 1;

  template<typename U>
  static base::maybe<T> get( cthread L, int idx,
                             tag<U> ) noexcept {
    return lua_get( L, idx, tag<T>{} );
  }

  static void push( cthread L, T const& o ) { lua_push( L, o ); }

  static void push( cthread L, T&& o ) {
    lua_push( L, std::move( o ) );
  }
};

// Default for types that don't override it.
template<typename T>
struct type_traits : public default_traits<T> {};

template<typename T>
using traits_for = type_traits<std::remove_cvref_t<T>>;

/****************************************************************
** push
*****************************************************************/
template<typename T>
void push( cthread L, T&& o ) {
  traits_for<T>::push( L, std::forward<T>( o ) );
}

/****************************************************************
** get
*****************************************************************/
template<typename T>
auto get( cthread L, int idx ) {
  return traits_for<T>::get( L, idx, tag<T>{} );
}

} // namespace lua
