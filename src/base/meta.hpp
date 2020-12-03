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
};

// Pointer to member-function.
template<typename T, typename R, typename... Arg>
struct callable_traits_impl<R ( T::* )( Arg... ) const>
  : public callable_traits_impl<R( Arg... )> {};

// Const Pointer to member-function.
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
struct callable_traits<
    O, typename std::enable_if_t<
           !std::is_function_v<std::remove_cvref_t<O>> &&
           !std::is_pointer_v<std::remove_cvref_t<O>> &&
           !detail::is_single_arg_generic_lambda_v<O> &&
           !std::is_member_function_pointer_v<
               std::remove_cvref_t<O>>>>
  : public detail::callable_traits_impl<
        decltype( &O::operator() )> {};

// Single Arg Generic Lambda.
template<typename O>
struct callable_traits<
    O, typename std::enable_if_t<
           !std::is_function_v<std::remove_cvref_t<O>> &&
           !std::is_pointer_v<std::remove_cvref_t<O>> &&
           detail::is_single_arg_generic_lambda_v<O> &&
           !std::is_member_function_pointer_v<
               std::remove_cvref_t<O>>>> {
  using arg_types = type_list<Auto>;
  /* no ret_type since we cannot know it. */
};

template<typename F>
using callable_ret_type_t =
    typename callable_traits<F>::ret_type;

template<typename F>
using callable_arg_types_t =
    typename callable_traits<F>::arg_types;

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

} // namespace mp
