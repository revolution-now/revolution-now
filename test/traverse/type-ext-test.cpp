/****************************************************************
**type-ext-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-12.
*
* Description: Unit tests for the traverse/type-ext module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/traverse/type-ext.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

// base
#include "src/base/cc-specific.hpp"
#include "src/base/odr.hpp"
#include "src/base/string.hpp"

namespace trv {
namespace {

using namespace std;

/****************************************************************
** Types
*****************************************************************/
struct String {};

template<typename T>
struct Z {};

template<typename T>
struct A {};

template<typename U, typename V>
struct B {};

template<typename M>
struct D {};

template<typename M>
struct Vec {};

template<typename K, typename V>
struct Map {};

} // namespace

/****************************************************************
** Impl
*****************************************************************/
TRV_TYPE_TRAVERSE( String );
TRV_TYPE_TRAVERSE( Z, T );
TRV_TYPE_TRAVERSE( A, T );
TRV_TYPE_TRAVERSE( D, T );
TRV_TYPE_TRAVERSE( B, U, V );
TRV_TYPE_TRAVERSE( Vec, T );
TRV_TYPE_TRAVERSE( Map, K, V );

namespace {

/****************************************************************
** Test
*****************************************************************/
vector<string> registrations;

template<typename T>
struct C {
  using type = T;

  inline static string const kTypeName = base::str_replace_all(
      base::demangled_typename<T>(),
      { { "trv::(anonymous namespace)::", "" },
        { " >", ">" } } );

  inline static int _ = [] {
    registrations.push_back( kTypeName );
    return 0;
  }();
  ODR_USE_MEMBER( _ );
};

using Foo = Map<int, Vec<B<D<char>, A<Z<B<Vec<int>, String>>>>>>;

TRV_RUN_TYPE_TRAVERSE( C, Foo );

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[traverse/type-ext] traverse Foo" ) {
  vector<string> const expected{
    "int",                                                  //
    "char",                                                 //
    "D<char>",                                              //
    "Vec<int>",                                             //
    "String",                                               //
    "B<Vec<int>, String>",                                  //
    "Z<B<Vec<int>, String>>",                               //
    "A<Z<B<Vec<int>, String>>>",                            //
    "B<D<char>, A<Z<B<Vec<int>, String>>>>",                //
    "Vec<B<D<char>, A<Z<B<Vec<int>, String>>>>>",           //
    "Map<int, Vec<B<D<char>, A<Z<B<Vec<int>, String>>>>>>", //
  };
  REQUIRE( registrations == expected );
}

} // namespace
} // namespace trv
