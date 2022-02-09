/****************************************************************
**ext.hpp
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
#include "base/valid.hpp"

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
struct StructField {
  using accessor_traits_t = mp::callable_traits<Accessor>;

  // Field type.
  using type = typename accessor_traits_t::ret_type;

  consteval StructField( std::string_view nm, Accessor acc )
    : name{ nm }, accessor{ acc } {}

  std::string_view name;
  Accessor         accessor;
};

template<typename T>
concept ReflectedStruct = Reflected<T> && requires {
  requires std::is_class_v<T>;
  requires traits<T>::kind == type_kind::struct_kind;
  requires HasTupleSize<typename traits<T>::template_types>;
  requires HasTupleSize<
      std::remove_cvref_t<decltype( traits<T>::fields )>>;
};

// This is specifically for structs that are reflected so that
// they can be validated after construction, since often they
// will be constructed through deserialization. This allows them
// to enforce any invariants among their fields. It is a member
// function as opposed to a free function so that the validation
// function (which will likely be implemented as an Rds feature
// on structs) can be removed, and that will cause a compile
// error on the definition to remind the user to remove it.
template<typename T>
concept ValidatableStruct = requires( T const& o ) {
  { o.validate() } -> std::same_as<base::valid_or<std::string>>;
};

/****************************************************************
** Wrappers
*****************************************************************/
template<typename T>
using wrapped_refltype_t =
    std::remove_cvref_t<decltype( std::declval<T>().refl() )>;

// This is for classes that are not themselves reflected but wrap
// all of their reflected fields in one subfield that is of a re-
// flected type. Essentially, operations that operate on such a
// wrapper will just forward their operations into the wrapped
// type.
template<typename T>
concept WrapsReflected = requires( T o ) {
  requires !Reflected<T>;
  requires std::is_class_v<T>;
  // This is a const& because the object should not expose a mu-
  // table reflected type because that would allow direct manipu-
  // lation of members. If the object wants that, then it should
  // expose those in a different way. For constructing the ob-
  // ject, one should first construct a wrapped type, then pass
  // it to the object's constructor, which is guaranteed to
  // exxist by the below.
  { o.refl() } -> std::same_as<wrapped_refltype_t<T> const&>;
  requires Reflected<wrapped_refltype_t<T>>;
  requires std::is_constructible_v<T, wrapped_refltype_t<T> &&>;
  { T::refl_name } -> std::same_as<std::string_view const&>;
};

} // namespace refl
