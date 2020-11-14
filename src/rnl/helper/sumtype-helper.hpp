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

} // namespace rn