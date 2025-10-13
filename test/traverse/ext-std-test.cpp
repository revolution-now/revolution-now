/****************************************************************
**ext-std-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-04.
*
* Description: Unit tests for the traverse/ext-std module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/traverse/ext-std.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace trv {
namespace {

using namespace std;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[traverse/ext-std] pair" ) {
  using T          = pair<string, int>;
  using K_expected = string_view;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o.first  = "hello";
  o.second = 3;
  f();

  REQUIRE( v == vector<string>{
                  "first",
                  "string",
                  "hello",
                  "second",
                  "int",
                  "3",
                } );
}

TEST_CASE( "[traverse/ext-std] tuple" ) {
  using T          = tuple<string, int, double>;
  using K_expected = string_view;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, double> ) v.push_back( "double" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  get<0>( o ) = "hello";
  get<1>( o ) = 3;
  get<2>( o ) = 3.3;
  f();

  REQUIRE( v == vector<string>{
                  "<0>",
                  "string",
                  "hello",
                  "<1>",
                  "int",
                  "3",
                  "<2>",
                  "double",
                  "3.3",
                } );
}

TEST_CASE( "[traverse/ext-std] vector (range)" ) {
  using T          = vector<string>;
  using K_expected = int;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o = { "hello", "world" };
  f();

  REQUIRE( v == vector<string>{
                  "0",
                  "string",
                  "hello",
                  "1",
                  "string",
                  "world",
                } );
}

TEST_CASE( "[traverse/ext-std] unordered_map" ) {
  using T          = unordered_map<string, int>;
  using K_expected = string;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o["hello"] = 3;
  o["world"] = 4;
  f();

  sort( v.begin(), v.end() );
  REQUIRE( v == vector<string>{
                  "3",
                  "4",
                  "hello",
                  "int",
                  "int",
                  "world",
                } );
}

TEST_CASE( "[traverse/ext-std] unordered_set" ) {
  using T          = unordered_set<int>;
  using K_expected = trv::none_t;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const ) {
    static_assert( is_same_v<K, K_expected> );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o.insert( 3 );
  o.insert( 5 );
  f();

  sort( v.begin(), v.end() );
  REQUIRE( v == vector<string>{
                  "3",
                  "5",
                  "int",
                  "int",
                } );
}

TEST_CASE( "[traverse/ext-std] map" ) {
  using T          = map<string, int>;
  using K_expected = string;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const key ) {
    static_assert( is_same_v<K, K_expected> );
    v.push_back( format( "{}", key ) );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o["hello"] = 3;
  o["world"] = 4;
  f();

  REQUIRE( v == vector<string>{
                  "hello",
                  "int",
                  "3",
                  "world",
                  "int",
                  "4",
                } );
}

TEST_CASE( "[traverse/ext-std] set" ) {
  using T          = set<int>;
  using K_expected = trv::none_t;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const ) {
    static_assert( is_same_v<K, K_expected> );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o.insert( 3 );
  o.insert( 5 );
  f();

  REQUIRE( v == vector<string>{
                  "int",
                  "3",
                  "int",
                  "5",
                } );
}

TEST_CASE( "[traverse/ext-std] unique_ptr" ) {
  using T          = unique_ptr<int>;
  using K_expected = trv::none_t;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const ) {
    static_assert( is_same_v<K, K_expected> );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o = make_unique<int>( 7 );
  f();

  REQUIRE( v == vector<string>{
                  "int",
                  "7",
                } );
}

TEST_CASE( "[traverse/ext-std] shared_ptr" ) {
  using T          = shared_ptr<int>;
  using K_expected = trv::none_t;
  T o;

  vector<string> v;
  auto const traversing_fn = [&]<typename V, typename K>(
                                 V const& val, K const ) {
    static_assert( is_same_v<K, K_expected> );
    if constexpr( is_same_v<V, int> ) v.push_back( "int" );
    if constexpr( is_same_v<V, string> ) v.push_back( "string" );
    v.push_back( format( "{}", val ) );
  };

  auto const f = [&] [[clang::noinline]] {
    trv::traverse( o, traversing_fn );
  };

  o = make_shared<int>( 7 );
  f();

  REQUIRE( v == vector<string>{
                  "int",
                  "7",
                } );
}

} // namespace
} // namespace trv
