/****************************************************************
**variant-util.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-12-27.
*
* Description: Utilities for working with variants.
*
*****************************************************************/
#pragma once

#include "config.hpp"

// base
#include "maybe.hpp"
#include "variant.hpp"

// base
#include "meta.hpp"

// C++ standard library
#include <variant>

// If the variant holds a value of type X then get a reference to
// its value and then do something with it.
//
//   variant<int, string> v = ...;
//
//   if_get( v, string, number ) {
//     /* v holds type string and number
//      * is a reference to it. */
//     cout << number.size();
//   }
//
// In order to produce a reference we need to deference the value
// of std::get_if. However, we can't do this in the top-level if
// statement otherwise we would sometimes be deferencing a
// nullptr which would be caught by sanitizers (-fsanitize=null
// to be specific). So therefore we add another statement under
// the top-level if that only gets executed when the pointer is
// non-null and which declares the reference. This secondary
// statement could either be another if statement or a switch
// statement (both allow declaring variables before performing
// their respective tests). However, if we use an if-statement
// then the compiler will issue a warning when we have an `else`
// statement below it, since it considers that an error-prone
// situation (i.e., when an else statement follows two nested
// if-statements without braces), since it could be considered
// ambiguous to a reader what if statement is supposed ot match
// with the else. So therefore we use a switch statement. This
// looks a bit hacky, but it seems to work pretty well and should
// be fine.
//
// WARNING: You cannot use `break` from within the body of this
//          if_get. E.g., the following will not do what you ex-
//          pect:
//
//          if_get( xxx, yyy, zzz ) {
//            ...
//            break;
//          }
//
//          because the break will simply break out of the
//          `switch` statement that we're using to implement
//          this.
#define if_get( v, type, var )                               \
  if( auto* _##var##_ = std::get_if<type>( &v ); _##var##_ ) \
    switch( auto& var = *_##var##_; 0 )                      \
    default:

namespace base {

// Does the variant hold the specified value. It will first be
// tested to see if it holds the type of the value, which is ob-
// viously a prerequisite for holding the value.
template<typename T, typename... Vs>
bool holds( variant<Vs...> const& v, T const& val ) {
  return std::holds_alternative<T>( v ) &&
         *std::get_if<T>( &v ) == val;
}

// This is just to have consistent notation with the above in the
// case when we just want to test if it holds a type and get a
// reference to the value.
template<typename T, typename... Vs>
maybe<T&> holds( variant<Vs...>& v ) {
  T* p = std::get_if<T>( &v );
  return p ? maybe<T&>( *p ) : nothing;
}

// For const access to the variant.
template<typename T, typename... Vs>
maybe<T const&> holds( variant<Vs...> const& v ) {
  T const* p = std::get_if<T>( &v );
  return p ? maybe<T const&>( *p ) : nothing;
}

// Non-mutating, just observes and returns something. For mutat-
// ing. This will apply the given function (which must take one
// argument and return void) and will apply it to the value of
// the variant if the current value has a type with a base class
// of the function argument type.
template<typename Func, typename DefT, typename... Args>
auto apply_to_alternatives_with_base( variant<Args...> const& v,
                                      DefT&& def, Func&& f ) {
  using Ret      = mp::callable_ret_type_t<Func>;
  using ArgsList = mp::callable_arg_types_t<Func>; // type_list
  static_assert( mp::list_size_v<ArgsList> == 1 );
  using Base = std::decay_t<mp::head_t<ArgsList>>;
  constexpr bool at_least_one = mp::any_v<
      std::is_base_of_v<Base, std::remove_cvref_t<Args>>...>;
  static_assert(
      at_least_one,
      "There are no variants with the given base class!" );
  return std::visit(
      [&]<typename T>( T const& alternative ) -> Ret {
        if constexpr( std::is_base_of_v<Base,
                                        std::remove_cvref_t<T>> )
          return f( alternative );
        else
          return def;
      },
      v );
}

// For mutating. This will apply the given function (which must
// take one argument and return void) and will apply it to the
// value of the variant if the current value has a type with a
// base class of the function argument type.
template<typename Func, typename... Args>
void apply_to_alternatives_with_base( variant<Args...>& v,
                                      Func&& f ) {
  using Ret = mp::callable_ret_type_t<Func>;
  static_assert( std::is_same_v<Ret, void> );
  using ArgsList = mp::callable_arg_types_t<Func>; // type_list
  static_assert( mp::list_size_v<ArgsList> == 1 );
  using Base = std::decay_t<mp::head_t<ArgsList>>;
  constexpr bool at_least_one = mp::any_v<
      std::is_base_of_v<Base, std::remove_cvref_t<Args>>...>;
  static_assert(
      at_least_one,
      "There are no variants with the given base class!" );
  std::visit(
      [&]<typename T>( T&& alternative ) {
        if constexpr( std::is_base_of_v<Base,
                                        std::remove_cvref_t<T>> )
          f( std::forward<T>( alternative ) );
      },
      v );
}

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
Base& variant_base( variant<Args...>& v ) {
  return std::visit(
      []<typename T>( T&& e ) -> Base& {
        static_assert(
            std::is_base_of_v<Base, std::decay_t<T>>,
            "all types in the variant must inherit from Base" );
        return e; //
      },
      v );
}

// And one for const.
template<typename Base, typename... Args>
Base const& variant_base( variant<Args...> const& v ) {
  return std::visit(
      []<typename T>( T const& e ) -> Base const& {
        static_assert(
            std::is_base_of_v<Base, T>,
            "all types in the variant must inherit from Base" );
        return e; //
      },
      v );
}

// This is the famouse Overload Pattern. When constructed from a
// list of callables, it will yield an object that is also
// callable, but whose callable operator consists of an overload
// set of all the given functions.
template<typename... T>
struct overload : T... {
  // This constructor is optional, but according to SuperV it is
  // good because it allows perfect forwarding of the function
  // objects. Not sure if that would happen without this. We need
  // new (separate) template arguments here (not T...) because if
  // we did T&& it would force it to be an rvalue reference to T,
  // whereas we want a /forwarding/ reference.
  template<typename... TFwds>
  overload( TFwds&&... args )
    : T{ std::forward<TFwds>( args ) }... {}

  // Need this in order to bring all the call operators into this
  // scope; this is because the compiler does name resolution be-
  // fore it does overload resolution, or so says SuperV.
  using T::operator()...;
};

// Deduction guide: when C++20 fully drops, we should be able to
// delete this.
template<typename... T>
overload( T... ) -> overload<T...>;

namespace detail {

// Will make sure that the Func takes an argument that is in the
// (type list) of variant alternatives.
template<typename Func, typename Alternatives>
inline constexpr bool overload_is_alternative_or_auto_v =
    std::is_same_v<mp::callable_arg_types_t<Func>,
                   mp::list<mp::Auto>>
        ? true
        : mp::list_contains_v<
              Alternatives,
              std::decay_t<
                  mp::head_t<mp::callable_arg_types_t<Func>>>>;

template<typename...>
struct visit_checks;

template<typename... VarArgs, typename... Funcs>
struct visit_checks<mp::list<VarArgs...>, mp::list<Funcs...>> {
  consteval static void go() {
    constexpr bool all_overloads_are_variants =
        mp::and_v<detail::overload_is_alternative_or_auto_v<
            Funcs, mp::list<VarArgs...>>...>;
    static_assert( all_overloads_are_variants,
                   "One of the overloads takes an argument of a "
                   "type that is not in the variant." );
    static_assert(
        sizeof...( Funcs ) <= sizeof...( VarArgs ),
        "Number of visitor functions must be less or "
        "equal to number of alternatives in variant." );
    // We need to add an lvalue reference to the VarArgs so that
    // when std::is_invocable calls declval on it it will produce
    // an lvalue reference. is_invocable then adds an rvalue ref-
    // erence to it, which will collapse back into an lvalue ref-
    // erence. This will then allow any type of function argument
    // (value, lvalue ref, rvalue ref) to bind to it. If we
    // hadn't done this then overloads with lvalue ref parameters
    // would appear to not be callable on our variants and this
    // assertion would trigger unnecessarily, even though the
    // overload could indeed bind to the variant.
    // See https://stackoverflow.com/a/64883147 for more info.
    constexpr bool invocable_for_all_variants =
        mp::and_v<std::is_invocable_v<
            decltype( overload{ std::declval<Funcs>()... } ),
            std::add_lvalue_reference_t<VarArgs>>...>;
    static_assert( invocable_for_all_variants,
                   "The visitor is missing at least one case!" );
  }
};

} // namespace detail

// Variant Visitor for Overload Sets
//
// Use like so:
//
//   struct A {}; struct B {}; struct C {}; struct D {};
//
//   using V = base::variant<A, B, C, D>;
//   V v = B{};
//
//   visit( v,
//     []( A const&    ) { fmt::print( "A"    ); },
//     []( B const&    ) { fmt::print( "B"    ); },
//     []( C const&    ) { fmt::print( "C"    ); },
//     []( auto const& ) { fmt::print( "auto" ); }
//   );
//
// Features:
//
//   * Duplicate overload causes error.
//   * Default is supported via auto.
//   * Multiple defaults cause error.
//   * Missing overload with no default causes error.
//   * Overload with incorrect type causes error.
//   * All overloads present + default causes error since
//     default is not needed.
//
template<typename... VarArgs, typename... Funcs>
decltype( auto ) overload_visit( variant<VarArgs...> const& v,
                                 Funcs&&... funcs ) {
  detail::visit_checks<mp::list<VarArgs...>,
                       mp::list<Funcs...>>::go();
  return std::visit( overload{ std::forward<Funcs>( funcs )... },
                     v );
}

template<typename... VarArgs, typename... Funcs>
decltype( auto ) overload_visit( variant<VarArgs...>& v,
                                 Funcs&&... funcs ) {
  detail::visit_checks<mp::list<VarArgs...>,
                       mp::list<Funcs...>>::go();
  return std::visit( overload{ std::forward<Funcs>( funcs )... },
                     v );
}

// Allows forcing return type.
template<typename Ret, typename... VarArgs, typename... Funcs>
decltype( auto ) overload_visit( variant<VarArgs...> const& v,
                                 Funcs&&... funcs ) {
  detail::visit_checks<mp::list<VarArgs...>,
                       mp::list<Funcs...>>::go();
  return std::visit<Ret>(
      overload{ std::forward<Funcs>( funcs )... }, v );
}

// Allows forcing return type.
template<typename Ret, typename... VarArgs, typename... Funcs>
decltype( auto ) overload_visit( variant<VarArgs...>& v,
                                 Funcs&&... funcs ) {
  detail::visit_checks<mp::list<VarArgs...>,
                       mp::list<Funcs...>>::go();
  return std::visit<Ret>(
      overload{ std::forward<Funcs>( funcs )... }, v );
}

// A wrapper around visit which allows taking the variant as the
// first argument.
template<typename Variant, typename Func>
decltype( auto ) visit( Variant&& v, Func&& func ) {
  return std::visit( std::forward<Func>( func ),
                     std::forward<Variant>( v ) );
}

} // namespace base
