/****************************************************************
**variant.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-01-27.
*
* Description: Utilities for handling variants.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "meta.hpp"

// base-util
#include "base-util/variant.hpp"

// Scelta
#include <scelta.hpp>

// C++ standard library
#include <variant>

namespace rn {

// Use this when all alternatives inherit from a common base
// class and you need a base-class pointer to the active member.
//
// This will give a compiler error if at least one alternative
// type does not inherit from the Base or does not do so
// publicly.
//
// This should only be called on variants that have a unique type
// list, although that is not enforced here (results will just
// not be meaninful).
//
// The result should always be non-null assuming that the variant
// is not in a valueless-by-exception state.
template<typename Base, typename... Args>
Base* variant_base_ptr( std::variant<Args...>& v ) {
  return util::visit( v, []( auto& e ) {
    using from_t = std::decay_t<decltype( e )>*;
    using to_t   = Base*;
    static_assert(
        std::is_convertible_v<from_t, to_t>,
        "all types in the variant must inherit from Base" );
    return static_cast<to_t>( &e ); //
  } );
}

// And one for const.
template<typename Base, typename... Args>
Base const* variant_base_ptr( std::variant<Args...> const& v ) {
  return util::visit( v, []( auto const& e ) {
    using from_t = std::decay_t<decltype( e )> const*;
    using to_t   = Base const*;
    static_assert(
        std::is_convertible_v<from_t, to_t>,
        "all types in the variant must inherit from Base" );
    return static_cast<to_t>( &e ); //
  } );
}

// A wrapper around scelta::visit which allows taking the variant
// as the first argument.
template<typename Ret, typename Visited, typename... Args>
auto match( Visited&& visited, Args&&... args ) {
  return scelta::match<Ret>( std::forward<Args>( args )... )(
      std::forward<Visited>( visited ) );
}

// Non-mutating, just observes and returns something. For mutat-
// ing. This will apply the given function (which must take one
// argument and return void) and will apply it to the value of
// the variant if the current value has a type with a base class
// of the function argument type.
template<typename Func, typename DefT, typename... Args>
auto apply_to_alternatives_with_base(
    std::variant<Args...> const& v, DefT&& def, Func&& f ) {
  using Ret      = mp::callable_ret_type_t<Func>;
  using ArgsList = mp::callable_arg_types_t<Func>; // type_list
  static_assert( mp::type_list_size_v<ArgsList> == 1 );
  using Base = std::decay_t<mp::head_t<ArgsList>>;
  constexpr bool at_least_one = mp::any_v<
      std::is_base_of_v<Base, std::remove_cvref_t<Args>>...>;
  static_assert(
      at_least_one,
      "There are no variants with the given base class!" );
  return std::visit(
      [&]<typename T>( T const& alternative ) -> Ret {
        if constexpr( std::is_base_of_v<Base,
                                        std::remove_cvref_t<T>> )
          return f( alternative );
        else
          return def;
      },
      v );
}

// For mutating. This will apply the given function (which must
// take one argument and return void) and will apply it to the
// value of the variant if the current value has a type with a
// base class of the function argument type.
template<typename Func, typename... Args>
void apply_to_alternatives_with_base( std::variant<Args...>& v,
                                      Func&& f ) {
  using Ret = mp::callable_ret_type_t<Func>;
  static_assert( std::is_same_v<Ret, void> );
  using ArgsList = mp::callable_arg_types_t<Func>; // type_list
  static_assert( mp::type_list_size_v<ArgsList> == 1 );
  using Base = std::decay_t<mp::head_t<ArgsList>>;
  constexpr bool at_least_one = mp::any_v<
      std::is_base_of_v<Base, std::remove_cvref_t<Args>>...>;
  static_assert(
      at_least_one,
      "There are no variants with the given base class!" );
  std::visit(
      [&]<typename T>( T&& alternative ) {
        if constexpr( std::is_base_of_v<Base,
                                        std::remove_cvref_t<T>> )
          f( std::forward<T>( alternative ) );
      },
      v );
}

} // namespace rn
