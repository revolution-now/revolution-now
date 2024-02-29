/****************************************************************
**auto-field.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-02-29.
*
* Description: Utilities related to extracting fields from
*              structs using structured bindings and without
*              relying on reflection.
*
*****************************************************************/
#pragma once

// base
#include "attributes.hpp"

// C++ standard library
#include <type_traits>

namespace base {

/****************************************************************
** Inner field.
*****************************************************************/
// FIXME: would be nice to have a concept to test whether a
// struct has precisely one field, but the destructuring approach
// used here to extract the fields is not SFINAE-friendly it
// seems :(

template<typename T>
auto&& single_inner_field( T&& o ATTR_LIFETIMEBOUND ) {
  auto&& [field] = o;
  return field;
}

namespace detail {

template<typename T>
struct inner_field_type {
 private:
  static auto& inner_ref( T& o ) {
    return single_inner_field( o );
  }

 public:
  using type = std::remove_reference_t<
      decltype( inner_field_type::inner_ref(
          std::declval<T&>() ) )>;
};

} // namespace detail

template<typename T>
using inner_field_type_t =
    typename detail::inner_field_type<T>::type;

} // namespace base
