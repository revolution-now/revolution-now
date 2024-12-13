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

// C++ standard library
#include <utility>

namespace base {

/****************************************************************
** no_default
*****************************************************************/
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
// wrappers introduce other problems and inconveniences (that
// said, if you need one, then see below). This approach is clean
// and wrapper-free, at the expense that the errors come at link
// time.
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

/****************************************************************
** no_default_wrapper
*****************************************************************/
// This is used to allow forcing the initialization of an aggre-
// gate field during aggregate initialization when that field is
// initialized via default construction in the struct definition:
//
//   struct Foo {
//     int bar = {};
//     T baz   = {};
//   };
//
// Assuming that the type of baz is default constructible (which
// it has to be otherwise the above wouldn't compile) then com-
// piler won't force the initialization of baz. One solution is
// to use no_default above; however, that is a "right hand side"
// solution, which in some cases we don't have the ability to do.
// This is a "left hand side" solution where we wrap the type in
// a wrapper that has no default constructor.
//
//   struct Foo {
//     int bar = {};
//     base::no_default_wrapper<T> baz;
//   };
//
// But note that we had to remove the "= {}" from the rhs. Then,
// if someone only partially initializes the fields:
//
//   Foo foo{ .bar = 4 };
//
// Then we will get a compile-time error.
//
template<typename T>
requires std::equality_comparable<T>
struct no_default_wrapper {
  template<typename U>
  requires std::is_constructible_v<T, U&&>
  no_default_wrapper( U&& o ) : val_( std::forward<U>( o ) ) {}

  T& get() { return val_; }
  T const& get() const { return val_; }

  no_default_wrapper() = delete;

  bool operator==( no_default_wrapper const& ) const = default;

 private:
  T val_;
};

/****************************************************************
** no_default_linker_wrapper
*****************************************************************/
// This is used when we want a left hand side solution like the
// one above but where we are unable to remove the "= {}" from
// the right hand side. In that case, we need a wrapper that does
// have a default constructor, but will trigger a linker error if
// it is actually called:
//
//   struct Foo {
//     int bar = {};
//     T baz   = {};
//   };
//
// becomes:
//
//   struct Foo {
//     int bar                                = {};
//     base::no_default_linker_wrapper<T> baz = {};
//   };
//
// where, again, notice that we have kept the "= {}". Then, if
// someone only partially initializes the fields:
//
//   Foo foo{ .bar = 4 };
//
// Then we will get a linker error.
//
template<typename T>
requires std::equality_comparable<T>
struct no_default_linker_wrapper {
  template<typename U>
  requires std::is_constructible_v<T, U&&>
  no_default_linker_wrapper( U&& o )
    : val_( std::forward<U>( o ) ) {}

  T& get() { return val_; }
  T const& get() const { return val_; }

  struct Dummy {};
  static Dummy
  you_forgot_to_initialize_a_required_aggregate_field();
  no_default_linker_wrapper(
      Dummy =
          you_forgot_to_initialize_a_required_aggregate_field() ) {
  }

  bool operator==( no_default_linker_wrapper const& ) const =
      default;

 private:
  T val_;
};

} // namespace base
