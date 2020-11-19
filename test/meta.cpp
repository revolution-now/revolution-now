/****************************************************************
**meta.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-17.
*
* Description: Unit tests for the meta module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/meta.hpp"

// Must be last.
#include "catch-common.hpp"

// C++ standard library
#include <type_traits>

namespace rn {
namespace {

using namespace std;
using namespace mp;

/****************************************************************
** Callable Traits
*****************************************************************/
namespace callable_traits_test {

struct A {};
struct B {};
struct C {};
struct Foo {};

using F1 = A();
using F2 = void( A );
using F3 = A( B );
using F4 = A( B, C );
using F5 = A ( * )( B );
using F6 = A ( & )( B, C );
using F7 = A const ( Foo::* )( B );

inline constexpr auto lambda = []( C const& ) -> A* {
  return nullptr;
};
using F8 = decltype( lambda );

// Generic lambdas.
inline constexpr auto auto_lambda     = []( auto ) {};
inline constexpr auto auto_lambda_ref = []( auto const& ) {};
using F9                              = decltype( auto_lambda );
using F10 = decltype( auto_lambda_ref );

static_assert( std::is_same_v<A, callable_ret_type_t<F1>> );
static_assert( std::is_same_v<void, callable_ret_type_t<F2>> );
static_assert( std::is_same_v<A, callable_ret_type_t<F3>> );
static_assert( std::is_same_v<A, callable_ret_type_t<F4>> );
static_assert( std::is_same_v<A, callable_ret_type_t<F5>> );
static_assert( std::is_same_v<A, callable_ret_type_t<F6>> );
static_assert(
    std::is_same_v<A const, callable_ret_type_t<F7>> );
static_assert( std::is_same_v<A*, callable_ret_type_t<F8>> );

static_assert(
    std::is_same_v<type_list<>, callable_arg_types_t<F1>> );
static_assert(
    std::is_same_v<type_list<A>, callable_arg_types_t<F2>> );
static_assert(
    std::is_same_v<type_list<B>, callable_arg_types_t<F3>> );
static_assert(
    std::is_same_v<type_list<B, C>, callable_arg_types_t<F4>> );
static_assert(
    std::is_same_v<type_list<B>, callable_arg_types_t<F5>> );
static_assert(
    std::is_same_v<type_list<B, C>, callable_arg_types_t<F6>> );
static_assert(
    std::is_same_v<type_list<B>, callable_arg_types_t<F7>> );
static_assert( std::is_same_v<type_list<C const&>,
                              callable_arg_types_t<F8>> );
static_assert( std::is_same_v<type_list<mp::Auto>,
                              callable_arg_types_t<F9>> );
static_assert( std::is_same_v<type_list<mp::Auto>,
                              callable_arg_types_t<F10>> );

} // namespace callable_traits_test

/****************************************************************
** List contains element
*****************************************************************/
static_assert( list_contains_v<type_list<>, int> == false );
static_assert( list_contains_v<type_list<void>, int> == false );
static_assert( list_contains_v<type_list<int>, int> == true );
static_assert( list_contains_v<type_list<char, int>, int> ==
               true );
static_assert( list_contains_v<type_list<char, char>, int> ==
               false );
static_assert(
    list_contains_v<type_list<char, char, int>, int> == true );
static_assert(
    list_contains_v<type_list<char, char, void>, int> == false );

/****************************************************************
** head
*****************************************************************/
static_assert( std::is_same_v<head_t<type_list<int>>, int> );
static_assert(
    std::is_same_v<head_t<type_list<char, char>>, char> );
static_assert(
    std::is_same_v<
        head_t<type_list<type_list<int>, char, int, char, int>>,
        type_list<int>> );

/****************************************************************
** type_list_size
*****************************************************************/
static_assert( type_list_size_v<type_list<>> == 0 );
static_assert( type_list_size_v<type_list<int>> == 1 );
static_assert( type_list_size_v<type_list<int, char>> == 2 );
static_assert(
    type_list_size_v<type_list<int, char, int, char, int>> ==
    5 );

/****************************************************************
** and
*****************************************************************/
#if 0 // remove duplicate code from base-util and enable this.
static_assert( and_v<true> == true );
static_assert( and_v<false> == false );
static_assert( and_v<true, false> == false );
static_assert( and_v<false, true> == false );
static_assert( and_v<true, true> == true );
static_assert( and_v<false, false> == false );
static_assert( and_v<true, true, true> == true );
static_assert( and_v<true, true, false> == false );
#endif

/****************************************************************
** any
*****************************************************************/
static_assert( any_v<true> == true );
static_assert( any_v<false> == false );
static_assert( any_v<true, false> == true );
static_assert( any_v<false, true> == true );
static_assert( any_v<true, true> == true );
static_assert( any_v<false, false> == false );
static_assert( any_v<true, true, true> == true );
static_assert( any_v<true, true, false> == true );
static_assert( any_v<false, true, false> == true );

} // namespace
} // namespace rn