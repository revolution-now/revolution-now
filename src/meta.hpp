/****************************************************************
**meta.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-10-06.
*
* Description: Metaprogramming utilities.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

namespace mp {

template<int N>
struct disambiguate;

/****************************************************************
** is_map_like
*****************************************************************/
template<typename, typename = void>
struct has_mapped_type_member : std::false_type {};

template<class T>
struct has_mapped_type_member<
    T, std::void_t<typename T::mapped_type>> : std::true_type {};

template<typename T>
constexpr bool has_mapped_type_member_v =
    has_mapped_type_member<T>::value;

template<typename, typename = void>
struct has_key_type_member : std::false_type {};

template<class T>
struct has_key_type_member<T, std::void_t<typename T::key_type>>
  : std::true_type {};

template<typename T>
constexpr bool has_key_type_member_v =
    has_key_type_member<T>::value;

template<typename T>
constexpr bool is_map_like = has_key_type_member_v<T>&& //
               has_mapped_type_member_v<T>;

} // namespace mp
