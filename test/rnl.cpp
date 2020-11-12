/****************************************************************
**rnl.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-02.
*
* Description: Unit tests for the rnl language.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "rnl/testing.hpp"

// base-util
#include "base-util/variant.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rnltest;

using Catch::Contains;

static_assert(
    is_same_v<::rnltest::Maybe_t<int>, Maybe_t<int>> );
static_assert( is_same_v<::rnltest::inner::MyVariant3_t,
                         inner::MyVariant3_t> );

TEST_CASE( "[rnl] Monostate" ) {
  static_assert( is_same_v<Monostate_t, std::monostate> );
  static_assert( is_same_v<MyVariant0_t, std::monostate> );
  REQUIRE( fmt::format( "{}", MyVariant0_t{} ) == "monostate" );
}

TEST_CASE( "[rnl] Maybe" ) {
  Maybe_t<int> maybe;
  REQUIRE( fmt::format( "{}", maybe ) == "Maybe::nothing<int>" );

  maybe = Maybe::nothing<int>{};
  REQUIRE( fmt::format( "{}", maybe ) == "Maybe::nothing<int>" );

  auto just = Maybe::just<int>{ 5 };
  static_assert( is_same_v<decltype( just.val ), int> );
  maybe = just;
  REQUIRE( fmt::format( "{}", maybe ) ==
           "Maybe::just<int>{val=5}" );

  switch_( maybe ) {
    case_( Maybe::nothing<int> ) {
      // Should not be here.
      REQUIRE( false );
    }
    case_( Maybe::just<int> ) { //
      REQUIRE( val.val == 5 );
    }
    switch_exhaustive;
  }

  Maybe_t<optional<char>> maybe_op_str;
  REQUIRE( fmt::format( "{}", maybe_op_str ) ==
           "Maybe::nothing<std::optional<char>>" );
  maybe_op_str = Maybe::just<optional<char>>{ { 'c' } };
  REQUIRE( fmt::format( "{}", maybe_op_str ) ==
           "Maybe::just<std::optional<char>>{val=c}" );
}

TEST_CASE( "[rnl] MyVariant1" ) {
  static_assert( !has_fmt<MyVariant1_t> );
  MyVariant1_t my1;
  my1    = MyVariant1::happy{ { 'c', 4 } };
  my1    = MyVariant1::excited{};
  bool b = true;
  my1    = MyVariant1::sad{ true, &b };
  switch_( my1 ) {
    case_( MyVariant1::happy ) {
      REQUIRE( false );
      static_assert(
          is_same_v<decltype( val.p ), pair<char, int>> );
    }
    case_( MyVariant1::excited ) {
      REQUIRE( false );
      static_assert( sizeof( MyVariant1::excited ) == 1 );
    }
    case_( MyVariant1::sad ) {
      static_assert( is_same_v<decltype( val.hello ), bool> );
      static_assert( is_same_v<decltype( val.ptr ), bool*> );
      REQUIRE( val.hello == true );
      REQUIRE( *val.ptr == true );
    }
    switch_exhaustive;
  }
}

TEST_CASE( "[rnl] MyVariant2" ) {
  static_assert( has_fmt<MyVariant2_t> );
  MyVariant2_t my2;
  my2 = MyVariant2::first{ "hello", true };
  my2 = MyVariant2::second{ true, false };
  my2 = MyVariant2::third{ 7 };
  switch_( my2 ) {
    case_( MyVariant2::first ) {
      static_assert( is_same_v<decltype( val.name ), string> );
      static_assert( is_same_v<decltype( val.b ), bool> );
    }
    case_( MyVariant2::second ) {
      static_assert( is_same_v<decltype( val.flag1 ), bool> );
      static_assert( is_same_v<decltype( val.flag2 ), bool> );
    }
    case_( MyVariant2::third ) {
      static_assert( is_same_v<decltype( val.cost ), int> );
    }
    switch_exhaustive;
  }
  REQUIRE( fmt::format( "{}", my2 ) ==
           "MyVariant2::third{cost=7}" );
}

TEST_CASE( "[rnl] MyVariant3" ) {
  static_assert( has_fmt<inner::MyVariant3_t> );
  inner::MyVariant3_t my3;
  my3 = inner::MyVariant3::a1{ MyVariant0_t{} };
  my3 = inner::MyVariant3::a2{ MyVariant0_t{}, MyVariant2_t{} };
  my3 = inner::MyVariant3::a3{ 'r' };
  switch_( my3 ) {
    case_( inner::MyVariant3::a1 ) {
      REQUIRE( false );
      static_assert(
          is_same_v<decltype( val.var0 ), MyVariant0_t> );
    }
    case_( inner::MyVariant3::a2 ) {
      REQUIRE( false );
      static_assert(
          is_same_v<decltype( val.var1 ), MyVariant0_t> );
      static_assert(
          is_same_v<decltype( val.var2 ), MyVariant2_t> );
    }
    case_( inner::MyVariant3::a3 ) { REQUIRE( val.c == 'r' ); }
    switch_exhaustive;
  }
}

TEST_CASE( "[rnl] MyVariant4" ) {
  static_assert( has_fmt<inner::MyVariant4_t> );
  inner::MyVariant4_t my4;
  my4 = inner::MyVariant4::first{ 1, 'r', true, { 3 } };
  my4 = inner::MyVariant4::_2nd{};
  my4 = inner::MyVariant4::third{ "hello",
                                  inner::MyVariant3::a3{ 'e' } };
  switch_( my4 ) {
    case_( inner::MyVariant4::first ) {
      REQUIRE( false );
      static_assert( is_same_v<decltype( val.op ),
                               std::optional<uint32_t>> );
    }
    case_( inner::MyVariant4::_2nd ) { REQUIRE( false ); }
    case_( inner::MyVariant4::third ) {
      REQUIRE( val.s == "hello" );
      switch_( val.var3 ) {
        case_( inner::MyVariant3::a1 ) { REQUIRE( false ); }
        case_( inner::MyVariant3::a2 ) { REQUIRE( false ); }
        case_( inner::MyVariant3::a3 ) {
          REQUIRE( val.c == 'e' );
        }
        switch_exhaustive;
      }
    }
    switch_exhaustive;
  }
  REQUIRE(
      fmt::format( "{}", my4 ) ==
      "MyVariant4::third{s=hello,var3=MyVariant3::a3{c=e}}" );
}

TEST_CASE( "[rnl] CompositeTemplateTwo" ) {
  using V = inner::CompositeTemplateTwo_t<optional<int>, short>;
  static_assert( has_fmt<V> );
  V v = inner::CompositeTemplateTwo::first<optional<int>, short>{
      .ttp = inner::TemplateTwoParams::third_alternative<
          optional<int>, short>{
          .hello = Maybe::just<optional<int>>{ optional<int>{
              3 } }, //
          .u     = 9 //
      } };
  using first_t =
      inner::CompositeTemplateTwo::first<optional<int>, short>;
  using second_t =
      inner::CompositeTemplateTwo::second<optional<int>, short>;
  switch_( v ) {
    case_( first_t ) {
      //
    }
    case_( second_t ) {
      REQUIRE( false );
      static_assert( sizeof( second_t ) == 1 );
    }
    switch_exhaustive;
  }
  REQUIRE(
      fmt::format( "{}", v ) ==
      "CompositeTemplateTwo::first<std::optional<int>,short>{"
      "ttp=TemplateTwoParams::third_alternative<std::optional<"
      "int>,short>{hello=Maybe::just<std::optional<int>>{val=3},"
      "u=9}}" );
}

} // namespace
} // namespace rn