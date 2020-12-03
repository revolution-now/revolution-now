/****************************************************************
**util.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Common utilities for all modules.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "errors.hpp"

// base
#include "base/variant.hpp"

// C++ standard library
#include <algorithm>
#include <string_view>
#include <variant>

namespace rn {

// Here "up" means "toward +inf" and "down" means "toward -inf".
ND int round_up_to_nearest_int_multiple( double d, int m );
ND int round_down_to_nearest_int_multiple( double d, int m );

// FIXME: move to base-util.
template<typename ContainerT, typename ElemT>
ND int count( ContainerT&& c, ElemT&& e ) {
  return std::count( std::forward<ContainerT>( c ).begin(),
                     std::forward<ContainerT>( c ).end(),
                     std::forward<ElemT>( e ) );
}

template<typename Base, typename From, typename To>
void copy_common_base_object( From const& from, To& to ) {
  *( static_cast<Base*>( &to ) ) =
      *( static_cast<Base const*>( &from ) );
}

// All the parameters must be either the result type or convert-
// ible to the result type.
template<typename Res, typename... T>
auto params_to_vector( T&&... ts ) {
  std::vector<Res> res;
  res.reserve( sizeof...( T ) );
  ( res.push_back( std::forward<T>( ts ) ), ... );
  return res;
}

// If the HOME environment variable is set then return it.
Opt<fs::path> user_home_folder();

// Get the value of an environment variable, if available.
Opt<std::string_view> env_var( char const* name );

// Set an environment variable, always overwriting.
void set_env_var( char const* var_name, char const* value );
// Set an environment variable, never overwriting.
void set_env_var_if_not_set( char const* var_name,
                             char const* value );
void unset_env_var( char const* name );

// Get the number of columns in the terminal from which the pro-
// gram was launched, if available.
Opt<int> os_terminal_columns();

} // namespace rn
