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

// Revolution Now
#include "fmt-helper.hpp"

// base
#include "base/fs.hpp"

// base-util
#include "base-util/io.hpp"
#include "base-util/variant.hpp"

// Must be last.
#include "catch-common.hpp"

#include <string>
#include <utility>

namespace rn {
// Use a fake optional because the type name (which we need to
// compute to test formatting) is platform-dependent due to in-
// line namespaces.
template<typename T>
struct my_optional {
  T t;
};
} // namespace rn
DEFINE_FORMAT_T( ( T ), (::rn::my_optional<T>), "{}", o.t );

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

  Maybe_t<rn::my_optional<char>> maybe_op_str;
  REQUIRE( fmt::format( "{}", maybe_op_str ) ==
           "Maybe::nothing<rn::my_optional<char>>" );
  maybe_op_str = Maybe::just<rn::my_optional<char>>{ { 'c' } };
  REQUIRE( fmt::format( "{}", maybe_op_str ) ==
           "Maybe::just<rn::my_optional<char>>{val=c}" );
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
  using V =
      inner::CompositeTemplateTwo_t<rn::my_optional<int>, short>;
  static_assert( has_fmt<V> );
  V v = inner::CompositeTemplateTwo::first<rn::my_optional<int>,
                                           short>{
      .ttp = inner::TemplateTwoParams::third_alternative<
          rn::my_optional<int>, short>{
          .hello =
              Maybe::just<rn::my_optional<int>>{
                  rn::my_optional<int>{ 3 } }, //
          .u = 9                               //
      } };
  using first_t =
      inner::CompositeTemplateTwo::first<rn::my_optional<int>,
                                         short>;
  using second_t =
      inner::CompositeTemplateTwo::second<rn::my_optional<int>,
                                          short>;
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
      "CompositeTemplateTwo::first<rn::my_optional<int>,short>{"
      "ttp=TemplateTwoParams::third_alternative<rn::my_optional<"
      "int>,short>{hello=Maybe::just<rn::my_optional<int>>{val="
      "3},"
      "u=9}}" );
}

TEST_CASE( "[rnl] Comparison" ) {
  // Maybe_t
  REQUIRE( Maybe::nothing<int>{} == Maybe::nothing<int>{} );
  REQUIRE( Maybe::just<int>{ 5 } == Maybe::just<int>{ 5 } );
  REQUIRE( Maybe::just<int>{ 5 } != Maybe::just<int>{ 6 } );
  // TODO: when we have the spaceship operator.
  // REQUIRE( Maybe::just<int>{ 5 } < Maybe::just<int>{ 6 } );
  // REQUIRE( Maybe::just<int>{ 6 } > Maybe::just<int>{ 5 } );
  // REQUIRE( Maybe::just<int>{ 5 } <= Maybe::just<int>{ 6 } );
  // REQUIRE( Maybe::just<int>{ 6 } >= Maybe::just<int>{ 5 } );
  // REQUIRE( ( Maybe::just<int>{ 5 } <=> Maybe::just<int>{ 5 } )
  // ==
  //         strong_ordering::equal );

  // TemplateTwoParams_t
  using T =
      inner::TemplateTwoParams::first_alternative<string, int>;
  REQUIRE( T{ "a", 'c' } == T{ "a", 'c' } );
  REQUIRE( T{ "a", 'b' } != T{ "a", 'c' } );
  REQUIRE( T{ "b", 'a' } != T{ "c", 'a' } );
  // TODO: when we have the spaceship operator.
  // REQUIRE( T{ "a", 'b' } < T{ "a", 'c' } );
  // REQUIRE( T{ "a", 'd' } > T{ "a", 'c' } );
  // REQUIRE( T{ "a", 'b' } < T{ "b", 'c' } );
  // REQUIRE( T{ "a", 'd' } < T{ "b", 'c' } );
  // REQUIRE( T{ "b", 'b' } >= T{ "a", 'c' } );
  // REQUIRE( T{ "a", 'd' } >= T{ "a", 'c' } );
}

TEST_CASE( "[rnl] Rnl File Golden Comparison" ) {
  Opt<Str> golden = util::read_file_as_string(
      testing::data_dir() / "rnl-testing-golden.hpp" );
  REQUIRE( golden.has_value() );
  fs::path root      = TO_STRING( RN_BUILD_OUTPUT_ROOT_DIR );
  Opt<Str> generated = util::read_file_as_string(
      root / fs::path( rnl_testing_genfile ) );
  REQUIRE( generated.has_value() );
  // Do this comparison outside of the REQUIRE macro so that
  // Catch2 doesn't try to print the values when they are not
  // equal.
  bool eq = ( generated == golden );
  REQUIRE( eq );
}

} // namespace
} // namespace rn