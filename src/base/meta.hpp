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

#include "config.hpp"

// C++ standard library
#include <experimental/type_traits>
#include <tuple>
#include <type_traits>

namespace mp {

template<int N>
struct disambiguate;

template<typename...>
struct type_list;

// This is used to represent `auto` for meta functions that are
// to return the type of function arguments; it is used when a
// lambda with a generic argument is is given to such a meta
// function.
struct Auto {};

/****************************************************************
** Overload Pattern
*****************************************************************/
// This is the famouse Overload Pattern. When constructed from a
// list of callables, it will yield an object that is also
// callable, but whose callable operator consists of an overload
// set of all the given functions.
template<typename... T>
struct overload : T... {
  using T::operator()...;
};

// Deduction guide: when C++20 fully drops, we should be able to
// delete this.
template<typename... T>
overload( T... ) -> overload<T...>;

/****************************************************************
** Is Callable Overloaded
*****************************************************************/
namespace detail {
template<typename T>
using call_op_type = decltype( &T::operator() );
} // namespace detail

// This should only be used on callable types.
template<typename T>
inline constexpr bool is_overloaded_v =
    !std::experimental::is_detected<detail::call_op_type,
                                    T>::value;

/****************************************************************
** Get Callable Signature
*****************************************************************/
namespace detail {

// This is kind of hacky and probably doesn't work in many cases.
template<typename Callable>
inline constexpr bool is_single_arg_generic_lambda_v =
    is_overloaded_v<Callable>;

template<typename...>
struct callable_traits_impl;

// Function.
template<typename R, typename... Arg>
struct callable_traits_impl<R( Arg... )> {
  using arg_types = type_list<Arg...>;
  using ret_type  = R;
  using func_type = R( Arg... );

  static constexpr bool abominable_const = false;
};

// Function const (abominable function type).
template<typename R, typename... Arg>
struct callable_traits_impl<R( Arg... ) const> {
  using arg_types = type_list<Arg...>;
  using ret_type  = R;
  using func_type = R( Arg... ) const;

  static constexpr bool abominable_const = true;
};

// Pointer to member-function.
template<typename T, typename R, typename... Arg>
struct callable_traits_impl<R ( T::* )( Arg... )>
  : public callable_traits_impl<R( Arg... )> {};

// Const Pointer to member-function.
template<typename T, typename R, typename... Arg>
struct callable_traits_impl<R ( T::*const )( Arg... )>
  : public callable_traits_impl<R( Arg... )> {};

// Pointer to member-function (const abominable).
template<typename T, typename R, typename... Arg>
struct callable_traits_impl<R ( T::* )( Arg... ) const>
  : public callable_traits_impl<R( Arg... )> {};

// Const Pointer to member-function (const abominable).
template<typename T, typename R, typename... Arg>
struct callable_traits_impl<R ( T::*const )( Arg... ) const>
  : public callable_traits_impl<R( Arg... )> {};

// Function pointer.
template<typename R, typename... Arg>
struct callable_traits_impl<R ( * )( Arg... )>
  : public callable_traits_impl<R( Arg... )> {};

// Const Function pointer.
template<typename R, typename... Arg>
struct callable_traits_impl<R ( *const )( Arg... )>
  : public callable_traits_impl<R( Arg... )> {};

// Function reference.
template<typename R, typename... Arg>
struct callable_traits_impl<R ( & )( Arg... )>
  : public callable_traits_impl<R( Arg... )> {};

} // namespace detail

template<typename T, typename Enable = void>
struct callable_traits;

// Function.
template<typename R, typename... Arg>
struct callable_traits<R( Arg... )>
  : public detail::callable_traits_impl<R( Arg... )> {};

// Function const (abominable function type).
template<typename R, typename... Arg>
struct callable_traits<R( Arg... ) const>
  : public detail::callable_traits_impl<R( Arg... ) const> {};

// Function ref-qualified (abominable function type).
template<typename R, typename... Arg>
struct callable_traits<R( Arg... )&>
  : public detail::callable_traits_impl<R( Arg... )> {};

// Function refref-qualified (abominable function type).
template<typename R, typename... Arg>
struct callable_traits<R( Arg... ) &&>
  : public detail::callable_traits_impl<R( Arg... )> {};

// Function const ref-qualified (abominable function type).
template<typename R, typename... Arg>
struct callable_traits<R( Arg... ) const&>
  : public detail::callable_traits_impl<R( Arg... ) const> {};

// Function const refref-qualified (abominable function type).
template<typename R, typename... Arg>
struct callable_traits<R( Arg... ) const&&>
  : public detail::callable_traits_impl<R( Arg... ) const> {};

// Pointer.
template<typename O>
struct callable_traits<
    O, typename std::enable_if_t<
           std::is_pointer_v<std::remove_cvref_t<O>>>>
  : public detail::callable_traits_impl<O> {};

// Function reference.
template<typename R, typename... Arg>
struct callable_traits<R ( & )( Arg... )>
  : public detail::callable_traits_impl<R( Arg... )> {};

// Pointer to member function.
template<typename R, typename C, typename... Arg>
struct callable_traits<R ( C::* )( Arg... )>
  : public detail::callable_traits_impl<R( Arg... )> {};

// Object.
template<typename O>
// clang-format off
requires(
    !std::is_function_v<std::remove_cvref_t<O>> &&
    !std::is_pointer_v<std::remove_cvref_t<O>> &&
    !detail::is_single_arg_generic_lambda_v<O> &&
    !std::is_member_function_pointer_v<std::remove_cvref_t<O>> )
struct callable_traits<O>
  // clang-format on
  : public detail::callable_traits_impl<
        decltype( &O::operator() )> {};

// clang-format off
// Single Arg Generic Lambda.
template<typename O>
requires(
    !std::is_function_v<std::remove_cvref_t<O>> &&
    !std::is_pointer_v<std::remove_cvref_t<O>> &&
     detail::is_single_arg_generic_lambda_v<O> &&
    !std::is_member_function_pointer_v<std::remove_cvref_t<O>> )
struct callable_traits<O> {
  // clang-format on
  using arg_types = type_list<Auto>;
  /* no ret_type since we cannot know it. */
};

template<typename F>
using callable_ret_type_t =
    typename callable_traits<F>::ret_type;

template<typename F>
using callable_arg_types_t =
    typename callable_traits<F>::arg_types;

template<typename F>
using callable_func_type_t =
    typename callable_traits<F>::func_type;

/****************************************************************
** Make function type from type_list of args.
*****************************************************************/
namespace detail {

template<typename Ret, typename... Args>
struct func_type_from_typelist;

template<typename Ret, typename... Args>
struct func_type_from_typelist<Ret, type_list<Args...>> {
  using type = Ret( Args... );
};

} // namespace detail

template<typename Ret, typename TypeList>
using function_type_from_typelist_t =
    typename detail::func_type_from_typelist<Ret,
                                             TypeList>::type;

/****************************************************************
** List contains element
*****************************************************************/
namespace detail {

template<typename...>
struct list_contains_impl;

template<typename T>
struct list_contains_impl<mp::type_list<>, T> {
  static constexpr bool value = false;
};

template<typename T, typename Arg1, typename... Args>
struct list_contains_impl<mp::type_list<Arg1, Args...>, T> {
  static constexpr bool value =
      std::is_same_v<T, Arg1> ||
      list_contains_impl<mp::type_list<Args...>, T>::value;
};

} // namespace detail

template<typename List, typename T>
inline constexpr bool list_contains_v =
    detail::list_contains_impl<List, T>::value;

/****************************************************************
** type_list to tuple
*****************************************************************/
template<typename... Args>
auto type_list_to_tuple_impl( type_list<Args...> const& )
    -> std::tuple<Args...>;

template<typename List>
using to_tuple_t = decltype( type_list_to_tuple_impl(
    std::declval<List const&>() ) );

/****************************************************************
** tuple to type_list
*****************************************************************/
template<typename... Args>
auto tuple_to_type_list_impl( std::tuple<Args...> const& )
    -> type_list<Args...>;

template<typename Tuple>
using to_type_list_t = decltype( tuple_to_type_list_impl(
    std::declval<Tuple const&>() ) );

/****************************************************************
** tail
*****************************************************************/
template<typename...>
struct tail;

template<typename Arg1, typename... Args>
struct tail<type_list<Arg1, Args...>> {
  using type = type_list<Args...>;
};

template<typename List>
using tail_t = typename tail<List>::type;

/****************************************************************
** head
*****************************************************************/
template<typename...>
struct head;

template<typename Arg1, typename... Args>
struct head<type_list<Arg1, Args...>> {
  using type = Arg1;
};

template<typename List>
using head_t = typename head<List>::type;

/****************************************************************
** type_list_size
*****************************************************************/
template<typename...>
struct type_list_size;

template<typename... Args>
struct type_list_size<type_list<Args...>> {
  static constexpr std::size_t size = sizeof...( Args );
};

template<typename List>
inline constexpr auto type_list_size_v =
    type_list_size<List>::size;

/****************************************************************
** and
*****************************************************************/
#if 0 // remove duplicate code from base-util and enable this.
template<bool...>
constexpr const bool and_v = false;

template<bool Bool>
constexpr const bool and_v<Bool> = Bool;

template<bool First, bool... Bools>
constexpr bool and_v<First, Bools...> = First&& and_v<Bools...>;
#endif

/****************************************************************
** any
*****************************************************************/
template<bool...>
constexpr const bool any_v = false;

template<bool Bool>
constexpr const bool any_v<Bool> = Bool;

template<bool First, bool... Bools>
constexpr bool any_v<First, Bools...> = First || any_v<Bools...>;

/****************************************************************
** reference_wrapper
*****************************************************************/
template<typename>
inline constexpr bool is_reference_wrapper_v = false;

template<typename T>
inline constexpr bool
    is_reference_wrapper_v<::std::reference_wrapper<T>> = true;

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

/****************************************************************
** has_reserve_method
*****************************************************************/
template<typename, typename = void>
inline constexpr bool has_reserve_method = false;

template<class T>
inline constexpr bool has_reserve_method<
    T, std::void_t<decltype( std::declval<T>().reserve( 0 ) )>> =
    true;

/****************************************************************
** for_index_seq
*****************************************************************/
// Do a compile-time iteration from [0, Index) and call the given
// lambda function once for each, passing the index to the lambda
// function by way of an integral constant. Example:
//
//   auto func =
//     [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
//       ... Do some work...
//       ... with Idx available as a compile-time constant...
//     };
//
//   for_index_seq<5>( func );
//
// `func` will be called 5 times with Idx = 0, 1, 2, 3, 4. Op-
// tionally, the function can return a bool. If it does, then the
// bool will be checked and the iteration will stop when true is
// returned or the iterations are finished.
//
template<size_t Index, typename Func>
constexpr auto for_index_seq( Func&& func ) {
  if constexpr( Index == 0 ) {
    // Zero iterations. We have to bail early here because other-
    // wise we'd try to determine the return type of the function
    // by calling it with an integral constant of 0 which it
    // might not be expecting, since an Index == 0 implies to the
    // caller that the Func will never get called (or instanti-
    // ated if it is a template lambda).
    return;
  } else {
    bool finished_early = false;
    using ret_t =
        decltype( std::declval<decltype( std::forward<Func>(
                      func ) )>()(
            std::integral_constant<size_t, 0>{} ) );
    constexpr bool returns_bool = std::is_same_v<bool, ret_t>;
    if constexpr( !returns_bool ) {
      static_assert(
          std::is_same_v<ret_t, void>,
          "Either the callback should return bool (indicating "
          "when to stop iterating) or return nothing at all." );
    }
    auto done_checker =
        [&]<size_t Idx>(
            std::integral_constant<size_t, Idx> i ) {
          if constexpr( returns_bool ) {
            if( !finished_early )
              finished_early = std::forward<Func>( func )( i );
          } else {
            (void)finished_early;
            std::forward<Func>( func )( i );
          }
        };
    auto for_index_seq_impl = [&]<size_t... Idxs>(
        std::index_sequence<Idxs...> ) {
      ( done_checker( std::integral_constant<size_t, Idxs>{} ),
        ... );
    };
    for_index_seq_impl( std::make_index_sequence<Index>() );
  }
}

/****************************************************************
** tuple_tail
*****************************************************************/
// Given a tuple, return a new tuple containing the same values
// but without the first element.
template<typename... Ts>
constexpr auto tuple_tail( std::tuple<Ts...> const input ) {
  using tuple_tail_t =
      to_tuple_t<tail_t<to_type_list_t<std::tuple<Ts...>>>>;
  tuple_tail_t smaller_tuple;
  for_index_seq<std::tuple_size_v<tuple_tail_t>>(
      [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
        std::get<Idx>( smaller_tuple ) =
            std::get<Idx + 1>( input );
      } );
  return smaller_tuple;
}

} // namespace mp
