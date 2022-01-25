/****************************************************************
**refl.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-03.
*
* Description: Reflection framework.
*
*****************************************************************/
#pragma once

// base
#include "base/meta.hpp"

// C++ standard library
#include <array>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace refl {

/****************************************************************
** Helpers
*****************************************************************/
template<typename T>
concept HasTupleSize = requires {
  { std::tuple_size_v<T> } -> std::same_as<std::size_t const&>;
};

template<typename T, typename U>
// clang-format off
concept StdArray =
  std::is_same_v<
      T,
      std::array<U,
                 std::tuple_size_v<std::remove_cvref_t<T>>> const&
  >;
// clang-format on

/****************************************************************
** Common
*****************************************************************/
enum class type_kind {
  enum_kind,
  struct_kind,
};

template<typename T>
struct traits;

template<typename T>
concept Reflected = requires {
  typename traits<T>::type;
  // clang-format off
  requires std::is_same_v<typename traits<T>::type, T>;
  { traits<T>::kind } -> std::same_as<type_kind const&>;
  { traits<T>::ns   } -> std::same_as<std::string_view const&>;
  { traits<T>::name } -> std::same_as<std::string_view const&>;
  // clang-format on
  requires !traits<T>::name.empty();
};

/****************************************************************
** Enums
*****************************************************************/
// Note this only applies to enums that don't have any explicit
// integer values set for the alternatives; they should just as-
// sume their default values. But there doesn't seem to be a way
// to enforce that with concepts.
template<typename T>
concept ReflectedEnum = Reflected<T> && requires {
  requires std::is_enum_v<T>;
  requires traits<T>::kind == type_kind::enum_kind;
  { traits<T>::value_names } -> StdArray<std::string_view>;
};

/****************************************************************
** Structs
*****************************************************************/
// Note that this will not allow fields that are references,
// since we can't form an accessor pointer to such a field.
template<typename Accessor>
struct ReflectedStructField {
  using accessor_traits_t = mp::callable_traits<Accessor>;

  // Field type.
  using type = typename accessor_traits_t::ret_type;

  consteval ReflectedStructField( std::string_view nm,
                                  Accessor         acc )
    : name{ nm }, accessor{ acc } {}

  std::string_view name;
  Accessor         accessor;
};

template<typename T>
concept ReflectedStruct = Reflected<T> && requires {
  requires std::is_class_v<T>;
  requires traits<T>::kind == type_kind::struct_kind;
  requires HasTupleSize<std::remove_cvref_t<
      decltype( traits<T>::template_types )>>;
  requires HasTupleSize<
      std::remove_cvref_t<decltype( traits<T>::fields )>>;
};

};

} // namespace refl
