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

} // namespace rn
