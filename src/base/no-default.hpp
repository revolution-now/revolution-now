/****************************************************************
**no-default.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-05-24.
*
* Description: Type for enforcing initilization of aggregate
*              fields in aggregate initialization.
*
*****************************************************************/
#pragma once

namespace base {

// This is used to allow forcing the initialization of aggregate
// fields during aggregate initialization. The compiler does not
// enforce that they be initialized, and sometimes we don't want
// to rely on default values, so we can do the following using
// this helper:
//
//   struct Foo {
//     int        bar = base::no_default<>;
//     int        baz = base::no_default<>;
//     int const& boz = base::no_default<>;
//     int&       boq = base::no_default<int&>;
//   };
//
// For most fields we just use the default template argument, but
// for non-const references we need to help it out otherwise it
// will try to bind a non-const reference to a temporary.
//
// Then, if someone only partially initializes the fields:
//
//   int n, m;
//   Foo foo{
//     .bar = 4,
//     .boz = n,
//     .boq = m,
//   };
//
// Then we will get a linker error because we purposely omit the
// definition of the implicit conversion operators in the classes
// below. It is possible to get a compile error instead of a
// linker error using this approach, but it requires making Foo a
// template, which is not always desired.
//
// Another way of achieving the same thing would be to wrap the
// field types in a wrapper that has no default constructor, but
// wrappers introduce other problems and inconveniences. This ap-
// proach is clean and wrapper-free, at the expense that the er-
// rors come at link time.
//
namespace detail {

template<typename Hint>
struct no_default_t {
  // This is purposely not implemented in order to trigger a
  // linker error when someone tries to invoke it.
  operator Hint() const;
};

template<>
struct no_default_t<void> {
  // This is purposely not implemented in order to trigger a
  // linker error when someone tries to invoke it.
  template<typename T>
  operator T() const;
};

} // namespace detail

// For most types, let the template parameter assume its default
// value. For non-const references, specify it explicitly.
template<typename T = void>
inline constexpr detail::no_default_t<T> no_default;

} // namespace base
