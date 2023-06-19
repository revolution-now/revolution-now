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
#include "test/testing.hpp"

// Under test.
#include "base/meta.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <type_traits>

namespace rn {
namespace {

using namespace std;
using namespace mp;

/****************************************************************
** list to tuple
*****************************************************************/
using tl0 = list<>;
using tl1 = list<int>;
using tl2 = list<int, char>;
static_assert( is_same_v<to_tuple_t<tl0>, tuple<>> );
static_assert( is_same_v<to_tuple_t<tl1>, tuple<int>> );
static_assert( is_same_v<to_tuple_t<tl2>, tuple<int, char>> );

/****************************************************************
** tuple to list
*****************************************************************/
using tu0 = std::tuple<>;
using tu1 = std::tuple<int>;
using tu2 = std::tuple<int, char>;
static_assert( is_same_v<to_list_t<tu0>, list<>> );
static_assert( is_same_v<to_list_t<tu1>, list<int>> );
static_assert( is_same_v<to_list_t<tu2>, list<int, char>> );

/****************************************************************
** Is Callable Overloaded
*****************************************************************/
struct foo_overloaded {
  [[maybe_unused]] void operator()( int ) {}
  [[maybe_unused]] void operator()( char ) {}
};
struct foo_non_overloaded {
  [[maybe_unused]] void operator()( int ) {}
};

static_assert( is_overloaded_v<foo_overloaded> == true );
static_assert( is_overloaded_v<foo_non_overloaded> == false );

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
using F11 = A ( *const )( B );
using F12 = A( B ) const;

// Lambda with capture.
[[maybe_unused]] auto F13 = [x = 1.0]( A* ) -> int {
  (void)x;
  return 0;
};

// mutable
[[maybe_unused]] auto F14 = [x = 1.0]( A* ) mutable -> int {
  (void)x;
  return 0;
};

struct Stateful {
  [[maybe_unused]] void operator()() {}
  int                   x;
  [[maybe_unused]] int  foo() const { return 0; }
};

struct HasMembers {
  [[maybe_unused]] void operator()() {}
  int                   x;
  int const             y;
  [[maybe_unused]] int  foo() const { return 0; }
};

using get_x_t = decltype( &HasMembers::x );
static_assert( is_same_v<int&, decltype( declval<HasMembers&>().*
                                         declval<get_x_t>() )> );
static_assert( is_same_v<int const&,
                         decltype( declval<HasMembers const&>().*
                                   declval<get_x_t>() )> );

static_assert( is_same_v<A(), callable_func_type_t<F1>> );
static_assert( is_same_v<void( A ), callable_func_type_t<F2>> );
static_assert( is_same_v<A( B ), callable_func_type_t<F3>> );
static_assert( is_same_v<A( B, C ), callable_func_type_t<F4>> );
static_assert( is_same_v<A( B ), callable_func_type_t<F5>> );
static_assert( is_same_v<A( B, C ), callable_func_type_t<F6>> );
static_assert(
    is_same_v<A const( B ), callable_func_type_t<F7>> );
static_assert(
    is_same_v<A ( * )(), callable_func_ptr_type_t<F1>> );
static_assert(
    is_same_v<void ( * )( A ), callable_func_ptr_type_t<F2>> );
static_assert(
    is_same_v<A ( * )( B ), callable_func_ptr_type_t<F3>> );
static_assert(
    is_same_v<A ( * )( B, C ), callable_func_ptr_type_t<F4>> );
static_assert(
    is_same_v<A ( * )( B ), callable_func_ptr_type_t<F5>> );
static_assert(
    is_same_v<A ( * )( B, C ), callable_func_ptr_type_t<F6>> );
static_assert( is_same_v<A const ( * )( B ),
                         callable_func_ptr_type_t<F7>> );
static_assert( is_same_v<A const ( Foo::* )( B ),
                         callable_member_func_type_t<F7>> );
static_assert(
    is_same_v<Foo, callable_traits<F7>::object_type> );
static_assert(
    is_same_v<A const( Foo*, B ),
              callable_member_func_flattened_type_t<F7>> );
static_assert(
    is_same_v<A*( C const& ) const, callable_func_type_t<F8>> );
static_assert(
    is_same_v<A* (*)(C const&), callable_func_ptr_type_t<F8>> );
static_assert( is_same_v<A* (F8::*)( C const& ) const,
                         callable_member_func_type_t<F8>> );
static_assert( is_same_v<F8, callable_traits<F8>::object_type> );
static_assert( is_same_v<F8 const, // same if/when const
                         callable_traits<F8>::object_type> );
static_assert(
    is_same_v<A*(F8*, C const&),
              callable_member_func_flattened_type_t<F8>> );
static_assert( is_same_v<A( B ), callable_func_type_t<F11>> );
static_assert(
    is_same_v<A ( * )( B ), callable_func_ptr_type_t<F11>> );
static_assert(
    is_same_v<int( A* ) const,
              callable_func_type_t<decltype( F13 )>> );
static_assert(
    is_same_v<int ( * )( A* ),
              callable_func_ptr_type_t<decltype( F13 )>> );
static_assert(
    is_same_v<int ( decltype( F13 )::* )( A* ) const,
              callable_member_func_type_t<decltype( F13 )>> );
static_assert( is_same_v<int( decltype( F13 ) const*, A* ),
                         callable_member_func_flattened_type_t<
                             decltype( F13 )>> );
static_assert(
    is_same_v<int( A* ),
              callable_func_type_t<decltype( F14 )>> );
static_assert(
    is_same_v<int ( * )( A* ),
              callable_func_ptr_type_t<decltype( F14 )>> );
static_assert(
    is_same_v<int ( decltype( F14 )::* )( A* ),
              callable_member_func_type_t<decltype( F14 )>> );
static_assert(
    is_same_v<decltype( F14 ),
              callable_traits<decltype( F14 )>::object_type> );
static_assert(
    !is_same_v<decltype( F14 ) const, // not const
               callable_traits<decltype( F14 )>::object_type> );
static_assert( is_same_v<int( decltype( F14 )*, A* ),
                         callable_member_func_flattened_type_t<
                             decltype( F14 )>> );
static_assert(
    is_same_v<void(), callable_func_type_t<Stateful>> );
static_assert( is_same_v<void ( * )(),
                         callable_func_ptr_type_t<Stateful>> );
static_assert(
    is_same_v<void(),
              callable_func_type_t<decltype( Stateful{} )>> );
static_assert(
    is_same_v<void ( * )(), callable_func_ptr_type_t<
                                decltype( Stateful{} )>> );
static_assert(
    is_same_v<void(), callable_func_type_t<
                          decltype( &Stateful::operator() )>> );
static_assert(
    is_same_v<void ( * )(),
              callable_func_ptr_type_t<
                  decltype( &Stateful::operator() )>> );
static_assert(
    is_same_v<void ( Stateful::* )(),
              callable_member_func_type_t<
                  decltype( &Stateful::operator() )>> );
static_assert(
    is_same_v<
        Stateful,
        callable_traits<
            decltype( &Stateful::operator() )>::object_type> );
static_assert(
    is_same_v<void( Stateful* ),
              callable_member_func_flattened_type_t<
                  decltype( &Stateful::operator() )>> );

static_assert(
    is_same_v<void(),
              callable_func_type_t<
                  decltype( &HasMembers::operator() )>> );
static_assert(
    is_same_v<void ( * )(),
              callable_func_ptr_type_t<
                  decltype( &HasMembers::operator() )>> );
static_assert(
    is_same_v<void ( HasMembers::* )(),
              callable_member_func_type_t<
                  decltype( &HasMembers::operator() )>> );
static_assert(
    is_same_v<
        HasMembers,
        callable_traits<
            decltype( &HasMembers::operator() )>::object_type> );
static_assert(
    is_same_v<void( HasMembers* ),
              callable_member_func_flattened_type_t<
                  decltype( &HasMembers::operator() )>> );
static_assert(
    is_same_v<int() const, callable_func_type_t<
                               decltype( &HasMembers::foo )>> );
static_assert(
    is_same_v<int ( * )(), callable_func_ptr_type_t<
                               decltype( &HasMembers::foo )>> );
static_assert( is_same_v<int ( HasMembers::* )() const,
                         callable_member_func_type_t<
                             decltype( &HasMembers::foo )>> );
static_assert(
    is_same_v<HasMembers const,
              callable_traits<
                  decltype( &HasMembers::foo )>::object_type> );
static_assert( is_same_v<int( HasMembers const* ),
                         callable_member_func_flattened_type_t<
                             decltype( &HasMembers::foo )>> );

static_assert( is_same_v<int HasMembers::*,
                         callable_member_func_type_t<
                             decltype( &HasMembers::x )>> );
static_assert(
    is_same_v<HasMembers,
              callable_traits<
                  decltype( &HasMembers::x )>::object_type> );
static_assert( is_same_v<int( HasMembers* ),
                         callable_member_func_flattened_type_t<
                             decltype( &HasMembers::x )>> );
static_assert(
    is_same_v<int, callable_traits<
                       decltype( &HasMembers::x )>::var_type> );
static_assert(
    is_same_v<
        int const,
        callable_traits<decltype( &HasMembers::y )>::var_type> );

static_assert( is_same_v<A, callable_ret_type_t<F1>> );
static_assert( is_same_v<void, callable_ret_type_t<F2>> );
static_assert( is_same_v<A, callable_ret_type_t<F3>> );
static_assert( is_same_v<A, callable_ret_type_t<F4>> );
static_assert( is_same_v<A, callable_ret_type_t<F5>> );
static_assert( is_same_v<A, callable_ret_type_t<F6>> );
static_assert( is_same_v<A const, callable_ret_type_t<F7>> );
static_assert( is_same_v<A*, callable_ret_type_t<F8>> );
static_assert( is_same_v<A, callable_ret_type_t<F11>> );
static_assert(
    is_same_v<int, callable_ret_type_t<decltype( F13 )>> );

static_assert( is_same_v<list<>, callable_arg_types_t<F1>> );
static_assert( is_same_v<list<A>, callable_arg_types_t<F2>> );
static_assert( is_same_v<list<B>, callable_arg_types_t<F3>> );
static_assert( is_same_v<list<B, C>, callable_arg_types_t<F4>> );
static_assert( is_same_v<list<B>, callable_arg_types_t<F5>> );
static_assert( is_same_v<list<B, C>, callable_arg_types_t<F6>> );
static_assert( is_same_v<list<B>, callable_arg_types_t<F7>> );
static_assert(
    is_same_v<list<C const&>, callable_arg_types_t<F8>> );
static_assert(
    is_same_v<list<mp::Auto>, callable_arg_types_t<F9>> );
static_assert(
    is_same_v<list<mp::Auto>, callable_arg_types_t<F10>> );
static_assert( is_same_v<list<B>, callable_arg_types_t<F11>> );
static_assert(
    is_same_v<list<A*>, callable_arg_types_t<decltype( F13 )>> );

// Abominable types.
static_assert(
    is_same_v<int( A* ) const,
              callable_func_type_t<int( A* ) const>> );
static_assert(
    is_same_v<int( A* ), callable_func_type_t<int( A* )>> );
static_assert(
    is_same_v<int( A* ) const,
              callable_func_type_t<int( A* ) const&>> );
static_assert(
    is_same_v<int( A* ), callable_func_type_t<int( A* ) &>> );
static_assert(
    is_same_v<int( A* ) const,
              callable_func_type_t<int( A* ) const&&>> );
static_assert(
    is_same_v<int( A* ), callable_func_type_t<int( A* ) &&>> );

static_assert( !callable_traits<int( A* )>::abominable_const );
static_assert(
    callable_traits<int( A* ) const>::abominable_const );

} // namespace callable_traits_test

/****************************************************************
** Make function type from list of args.
*****************************************************************/
static_assert(
    is_same_v<function_type_from_typelist_t<void, list<>>,
              void()> );
static_assert(
    is_same_v<function_type_from_typelist_t<int, list<>>,
              int()> );
static_assert(
    is_same_v<function_type_from_typelist_t<void, list<char>>,
              void( char )> );
static_assert(
    is_same_v<
        function_type_from_typelist_t<void, list<int, float>>,
        void( int, float )> );
static_assert(
    is_same_v<
        function_type_from_typelist_t<double, list<int const&>>,
        double( int const& )> );

/****************************************************************
** List contains element
*****************************************************************/
static_assert( list_contains_v<list<>, int> == false );
static_assert( list_contains_v<list<void>, int> == false );
static_assert( list_contains_v<list<int>, int> == true );
static_assert( list_contains_v<list<char, int>, int> == true );
static_assert( list_contains_v<list<char, char>, int> == false );
static_assert( list_contains_v<list<char, char, int>, int> ==
               true );
static_assert( list_contains_v<list<char, char, void>, int> ==
               false );

/****************************************************************
** tail
*****************************************************************/
static_assert( is_same_v<tail_t<list<int>>, list<>> );
static_assert( is_same_v<tail_t<list<char, char>>, list<char>> );
static_assert(
    is_same_v<tail_t<list<list<int>, char, int, char, int>>,
              list<char, int, char, int>> );

/****************************************************************
** head
*****************************************************************/
static_assert( is_same_v<head_t<list<int>>, int> );
static_assert( is_same_v<head_t<list<char, char>>, char> );
static_assert(
    is_same_v<head_t<list<list<int>, char, int, char, int>>,
              list<int>> );

/****************************************************************
** select_last/last
*****************************************************************/
static_assert( is_same_v<select_last_t<int>, int> );
static_assert( is_same_v<select_last_t<char, char>, char> );
static_assert( is_same_v<select_last_t<std::string, char, int,
                                       char, double>,
                         double> );

static_assert( is_same_v<last_t<list<int>>, int> );
static_assert( is_same_v<last_t<list<char, char>>, char> );
static_assert(
    is_same_v<last_t<list<std::string, char, int, char, double>>,
              double> );

/****************************************************************
** list_size
*****************************************************************/
static_assert( list_size_v<list<>> == 0 );
static_assert( list_size_v<list<int>> == 1 );
static_assert( list_size_v<list<int, char>> == 2 );
static_assert( list_size_v<list<int, char, int, char, int>> ==
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

/****************************************************************
** for_index_seq
*****************************************************************/
TEST_CASE( "[meta] for_index_seq" ) {
  SECTION( "zero" ) {
    for_index_seq<0>(
        [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
          static_assert( Idx != Idx, "should not be here" );
        } );
  }
  SECTION( "non-stop" ) {
    std::tuple<int, int, int, int, int> t{};

    REQUIRE( std::get<0>( t ) == 0 );
    REQUIRE( std::get<1>( t ) == 0 );
    REQUIRE( std::get<2>( t ) == 0 );
    REQUIRE( std::get<3>( t ) == 0 );
    REQUIRE( std::get<4>( t ) == 0 );

    for_index_seq<5>(
        [&]<size_t Idx>( std::integral_constant<size_t, Idx> ) {
          std::get<Idx>( t ) = Idx;
        } );

    REQUIRE( std::get<0>( t ) == 0 );
    REQUIRE( std::get<1>( t ) == 1 );
    REQUIRE( std::get<2>( t ) == 2 );
    REQUIRE( std::get<3>( t ) == 3 );
    REQUIRE( std::get<4>( t ) == 4 );
  }
  SECTION( "non-stop with macro" ) {
    std::tuple<int, int, int, int, int> t{};

    REQUIRE( std::get<0>( t ) == 0 );
    REQUIRE( std::get<1>( t ) == 0 );
    REQUIRE( std::get<2>( t ) == 0 );
    REQUIRE( std::get<3>( t ) == 0 );
    REQUIRE( std::get<4>( t ) == 0 );

    FOR_CONSTEXPR_IDX( Idx, 5 ) { std::get<Idx>( t ) = Idx; };

    REQUIRE( std::get<0>( t ) == 0 );
    REQUIRE( std::get<1>( t ) == 1 );
    REQUIRE( std::get<2>( t ) == 2 );
    REQUIRE( std::get<3>( t ) == 3 );
    REQUIRE( std::get<4>( t ) == 4 );
  }
  SECTION( "stop early" ) {
    std::tuple<int, int, int, int, int> t{};

    REQUIRE( std::get<0>( t ) == 0 );
    REQUIRE( std::get<1>( t ) == 0 );
    REQUIRE( std::get<2>( t ) == 0 );
    REQUIRE( std::get<3>( t ) == 0 );
    REQUIRE( std::get<4>( t ) == 0 );

    for_index_seq<5>(
        [&]<size_t Idx>(
            std::integral_constant<size_t, Idx> ) -> bool {
          std::get<Idx>( t ) = Idx;
          if( Idx == 2 ) return true;
          return false;
        } );

    REQUIRE( std::get<0>( t ) == 0 );
    REQUIRE( std::get<1>( t ) == 1 );
    REQUIRE( std::get<2>( t ) == 2 );
    REQUIRE( std::get<3>( t ) == 0 );
    REQUIRE( std::get<4>( t ) == 0 );
  }
}

/****************************************************************
** tuple_tail
*****************************************************************/
TEST_CASE( "[meta] tuple_tail" ) {
  std::tuple<int, string, double, float, int> t1{ 5, "hello",
                                                  4.5, 5.6f, 3 };

  auto t2 = mp::tuple_tail( t1 );
  REQUIRE( t2 == std::tuple<string, double, float, int>{
                     "hello", 4.5, 5.6f, 3 } );

  auto t3 = mp::tuple_tail( t2 );
  REQUIRE( t3 ==
           std::tuple<double, float, int>{ 4.5, 5.6f, 3 } );

  auto t4 = mp::tuple_tail( t3 );
  REQUIRE( t4 == std::tuple<float, int>{ 5.6f, 3 } );

  auto t5 = mp::tuple_tail( t4 );
  REQUIRE( t5 == std::tuple<int>{ 3 } );

  auto t6 = mp::tuple_tail( t5 );
  REQUIRE( t6 == std::tuple<>{} );
}

} // namespace
} // namespace rn