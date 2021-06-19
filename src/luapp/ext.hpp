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
#include <cassert>
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
struct type_traits;

template<typename T>
using traits_type = std::remove_cvref_t<T>;

template<typename T>
using traits_for = type_traits<traits_type<T>>;

// This is a default implementation that can be used to aid in
// specializing a type_traits struct, but it will not be used au-
// tomatically.
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

/****************************************************************
** Concepts
*****************************************************************/
template<typename T>
concept HasTraitsNvalues = requires {
  typename traits_for<T>;
  { traits_for<T>::nvalues } -> std::same_as<int const&>;
  traits_for<T>::nvalues > 0;
};

template<typename T>
concept PushableViaAdl = requires( T const& o, cthread L ) {
  lua_push( L, o );
};

template<typename T>
concept PushableViaTraits = HasTraitsNvalues<T> &&
    requires( T const& o, cthread L ) {
  { traits_for<T>::push( L, o ) } -> std::same_as<void>;
};

template<typename T>
concept GettableViaAdl = requires( cthread L ) {
  { lua_get( L, -1, tag<T>{} ) } -> std::same_as<base::maybe<T>>;
};

template<typename T>
concept GettableViaTraits = HasTraitsNvalues<T> &&
    requires( cthread L ) {
  // clang-format off
  { traits_for<T>::get( L, -1, tag<T>{} ) } ->
    std::same_as<base::maybe<T>>;
  // clang-format on
};

// Must be one or the other to avoid ambiguity.  We could
// use the xor operator, but the following representation
// gives more revealing error messages.
template<typename T>
concept Pushable =
    (PushableViaAdl<T> ||
     PushableViaTraits<T>)&&!( PushableViaAdl<T> &&
                               PushableViaTraits<T> );

// Must be one or the other to avoid ambiguity.  We could
// use the xor operator, but the following representation
// gives more revealing error messages.
template<typename T>
concept Gettable =
    (GettableViaAdl<T> ||
     GettableViaTraits<T>)&&!( GettableViaAdl<T> &&
                               GettableViaTraits<T> );

// Can the type be sent to and from Lua.
template<typename T>
concept Stackable = Pushable<T> && Gettable<T>;

// This generally can be used on extension point overloads that
// are highly unconstrained and that can accept many unknown
// types, for which types internal to the luapp library need to
// be excluded.
template<typename T>
concept LuappInternal = requires {
  typename std::remove_reference_t<T>::luapp_internal;
};

/****************************************************************
** helpers
*****************************************************************/
namespace internal {

  int ext_stack_size( cthread L );

} // namespace internal

/****************************************************************
** nvalues_for
*****************************************************************/
template<typename T>
// We must have the Pushable and Gettable here in order to en-
// force that the extension point overrides for the type T are
// visible at this point, otherwise we might just silently de-
// fault to a value of 1 below which we do not want.
requires Pushable<T> || Gettable<T>
constexpr int nvalues_for() {
  if constexpr( HasTraitsNvalues<T> ) {
    static_assert( traits_for<T>::nvalues > 0 );
    return traits_for<T>::nvalues;
  } else {
    return 1;
  }
}

/****************************************************************
** push
*****************************************************************/
template<Pushable T>
int push( cthread L, T&& o ) {
  int start_stack_size = internal::ext_stack_size( L );
  if constexpr( PushableViaAdl<T> )
    lua_push( L, std::forward<T>( o ) );
  else if constexpr( PushableViaTraits<T> )
    traits_for<T>::push( L, std::forward<T>( o ) );
  else
    static_assert( "should not be here." );
  int n_pushed =
      internal::ext_stack_size( L ) - start_stack_size;
  assert( n_pushed == nvalues_for<T>() );
  return n_pushed;
}

/****************************************************************
** get
*****************************************************************/
template<Gettable T>
auto get( cthread L, int idx ) {
  if constexpr( GettableViaAdl<T> )
    return lua_get( L, idx, tag<T>{} );
  else if constexpr( GettableViaTraits<T> )
    return traits_for<T>::get( L, idx, tag<T>{} );
  else
    static_assert( "should not be here." );
}

} // namespace lua
