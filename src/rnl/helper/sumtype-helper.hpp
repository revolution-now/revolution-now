/****************************************************************
**sumtype-helper.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-13.
*
* Description: Some helper code for the sumtype code generator.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "errors.hpp"

// base
#include "base/variant.hpp"

// C++ standard library
#include <variant>

namespace rn {

template<typename Variant, typename Func, size_t... Indexes>
void try_deserialize_variant_types_impl(
    Func&& f, std::index_sequence<Indexes...> ) {
  ( f( static_cast<
        std::variant_alternative_t<Indexes, Variant>*>(
        nullptr ) ),
    ... );
}

template<typename Variant, typename Func>
void try_deserialize_variant_types( Func&& f ) {
  try_deserialize_variant_types_impl<Variant>(
      std::forward<Func>( f ),
      std::make_index_sequence<std::variant_size_v<Variant>>() );
}

template<typename V>
struct SumtypeToEnum;

template<typename V>
using SumtypeToEnum_v = typename SumtypeToEnum<V>::type;

template<typename... Args>
auto enum_for( base::variant<Args...> const& v ) {
  return static_cast<SumtypeToEnum_v<base::variant<Args...>>>(
      v.index() );
}

template<typename T, typename V>
auto& get_if_or_die( V& v ) {
  auto* res = std::get_if<T>( &v );
  DCHECK( res != nullptr );
  return *res;
}

} // namespace rn