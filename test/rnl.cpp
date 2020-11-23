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
#include "base/build-properties.hpp"
#include "base/fs.hpp"

// base-util
#include "base-util/io.hpp"

// Must be last.
#include "catch-common.hpp"

#include <string>
#include <utility>

namespace rn {
// Use a fake optional and string because the type name (which we
// need to compute to test formatting) would otherwise either be
// platform-dependent (containing e.g. inline namespaces that we
// can't predict) or would contain a bunch of extra stuff (like
// allocator template arguments).
template<typename T>
struct my_optional {
  T t;
};

struct String {
  String( char const* s ) : s_( s ) {}
  std::string s_;
};
} // namespace rn

DEFINE_FORMAT_T( ( T ), (::rn::my_optional<T>), "{}", o.t );
DEFINE_FORMAT( ::rn::String, "{}", o.s_ );

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

  switch( enum_for( maybe ) ) {
    case Maybe::e::nothing: //
      REQUIRE( false );
    case Maybe::e::just: {
      auto& val = get_if_or_die<Maybe::just<int>>( maybe );
      REQUIRE( val.val == 5 );
      break;
    }
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
  switch( enum_for( my1 ) ) {
    case MyVariant1::e::happy: //
      REQUIRE( false );
      break;
    case MyVariant1::e::excited:
      REQUIRE( false );
      static_assert( sizeof( MyVariant1::excited ) == 1 );
      break;
    case MyVariant1::e::sad:
      auto& val = get_if_or_die<MyVariant1::sad>( my1 );
      static_assert( is_same_v<decltype( val.hello ), bool> );
      static_assert( is_same_v<decltype( val.ptr ), bool*> );
      REQUIRE( val.hello == true );
      REQUIRE( *val.ptr == true );
      break;
  }
}

TEST_CASE( "[rnl] MyVariant2" ) {
  static_assert( has_fmt<MyVariant2_t> );
  MyVariant2_t my2;
  my2 = MyVariant2::first{ "hello", true };
  my2 = MyVariant2::second{ true, false };
  my2 = MyVariant2::third{ 7 };
  switch( enum_for( my2 ) ) {
    case MyVariant2::e::first: {
      auto& val = get_if_or_die<MyVariant2::first>( my2 );
      static_assert( is_same_v<decltype( val.name ), string> );
      static_assert( is_same_v<decltype( val.b ), bool> );
      break;
    }
    case MyVariant2::e::second: {
      auto& val = get_if_or_die<MyVariant2::second>( my2 );
      static_assert( is_same_v<decltype( val.flag1 ), bool> );
      static_assert( is_same_v<decltype( val.flag2 ), bool> );
      break;
    }
    case MyVariant2::e::third: {
      auto& val = get_if_or_die<MyVariant2::third>( my2 );
      static_assert( is_same_v<decltype( val.cost ), int> );
      break;
    }
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
  switch( enum_for( my3 ) ) {
    case inner::MyVariant3::e::a1: {
      REQUIRE( false );
      auto& val = get_if_or_die<inner::MyVariant3::a1>( my3 );
      static_assert(
          is_same_v<decltype( val.var0 ), MyVariant0_t> );
      break;
    }
    case inner::MyVariant3::e::a2: {
      auto& val = get_if_or_die<inner::MyVariant3::a2>( my3 );
      REQUIRE( false );
      static_assert(
          is_same_v<decltype( val.var1 ), MyVariant0_t> );
      static_assert(
          is_same_v<decltype( val.var2 ), MyVariant2_t> );
      break;
    }
    case inner::MyVariant3::e::a3: {
      auto& val = get_if_or_die<inner::MyVariant3::a3>( my3 );
      REQUIRE( val.c == 'r' );
      break;
    }
  }
}

TEST_CASE( "[rnl] MyVariant4" ) {
  static_assert( has_fmt<inner::MyVariant4_t> );
  inner::MyVariant4_t my4;
  my4 = inner::MyVariant4::first{ 1, 'r', true, { 3 } };
  my4 = inner::MyVariant4::_2nd{};
  my4 = inner::MyVariant4::third{ "hello",
                                  inner::MyVariant3::a3{ 'e' } };
  switch( enum_for( my4 ) ) {
    case inner::MyVariant4::e::first: {
      REQUIRE( false );
      auto& val = get_if_or_die<inner::MyVariant4::first>( my4 );
      static_assert( is_same_v<decltype( val.op ),
                               std::optional<uint32_t>> );
      break;
    }
    case inner::MyVariant4::e::_2nd: {
      REQUIRE( false );
      break;
    }
    case inner::MyVariant4::e::third: {
      auto& val = get_if_or_die<inner::MyVariant4::third>( my4 );
      REQUIRE( val.s == "hello" );
      switch( enum_for( val.var3 ) ) {
        case inner::MyVariant3::e::a1: {
          REQUIRE( false );
          break;
        }
        case inner::MyVariant3::e::a2: {
          REQUIRE( false );
          break;
        }
        case inner::MyVariant3::e::a3: {
          auto& val_inner =
              get_if_or_die<inner::MyVariant3::a3>( val.var3 );
          REQUIRE( val_inner.c == 'e' );
          break;
        }
      }
      break;
    }
  }
  REQUIRE(
      fmt::format( "{}", my4 ) ==
      "MyVariant4::third{s=hello,var3=MyVariant3::a3{c=e}}" );
}

TEST_CASE( "[rnl] CompositeTemplateTwo" ) {
  using V =
      inner::CompositeTemplateTwo_t<rn::my_optional<int>, short>;
  namespace V_ns = inner::CompositeTemplateTwo;
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
  using second_t =
      inner::CompositeTemplateTwo::second<rn::my_optional<int>,
                                          short>;
  static_assert( sizeof( second_t ) == 1 );
  switch( enum_for( v ) ) {
    case V_ns::e::first: //
      break;
    case V_ns::e::second: //
      REQUIRE( false );
      break;
  }
  REQUIRE(
      fmt::format( "{}", v ) ==
      "CompositeTemplateTwo::first<rn::my_optional<int>,short>{"
      "ttp=TemplateTwoParams::third_alternative<rn::my_optional<"
      "int>,short>{hello=Maybe::just<rn::my_optional<int>>{val="
      "3},"
      "u=9}}" );
}

TEST_CASE( "[rnl] Equality" ) {
  // Maybe_t
  REQUIRE( Maybe::nothing<int>{} == Maybe::nothing<int>{} );
  REQUIRE( Maybe::just<int>{ 5 } == Maybe::just<int>{ 5 } );
  REQUIRE( Maybe::just<int>{ 5 } != Maybe::just<int>{ 6 } );

  // TemplateTwoParams_t
  using T =
      inner::TemplateTwoParams::first_alternative<string, int>;
  REQUIRE( T{ "a", 'c' } == T{ "a", 'c' } );
  REQUIRE( T{ "a", 'b' } != T{ "a", 'c' } );
  REQUIRE( T{ "b", 'a' } != T{ "c", 'a' } );
}

TEST_CASE( "[rnl] Rnl File Golden Comparison" ) {
  Opt<Str> golden = util::read_file_as_string(
      testing::data_dir() / "rnl-testing-golden.hpp" );
  REQUIRE( golden.has_value() );
  fs::path root      = base::build_output_root();
  Opt<Str> generated = util::read_file_as_string(
      root / fs::path( rnl_testing_genfile ) );
  REQUIRE( generated.has_value() );
  // Do this comparison outside of the REQUIRE macro so that
  // Catch2 doesn't try to print the values when they are not
  // equal.
  bool eq = ( generated == golden );
  REQUIRE( eq );
}

TEST_CASE( "[rnl] Associated Enums" ) {
  using namespace rnltest;
  MyVariant2_t v =
      MyVariant2::second{ .flag1 = false, .flag2 = true };
  switch( enum_for( v ) ) {
    case MyVariant2::e::first: //
      REQUIRE( false );
      break;
    case MyVariant2::e::second: {
      // Make sure that we can get a non-const reference.
      MyVariant2::second& snd =
          get_if_or_die<MyVariant2::second>( v );
      REQUIRE( fmt::format( "{}", snd ) ==
               "MyVariant2::second{flag1=false,flag2=true}" );
      break;
    }
    case MyVariant2::e::third: //
      REQUIRE( false );
      break;
  }

  Maybe_t<String> maybe = Maybe::just<String>{ .val = "hello" };
  switch( enum_for( maybe ) ) {
    case Maybe::e::nothing: //
      REQUIRE( false );
      break;
    case Maybe::e::just: {
      auto& just = get_if_or_die<Maybe::just<String>>( maybe );
      REQUIRE( fmt::format( "{}", just ) ==
               "Maybe::just<rn::String>{val=hello}" );
      break;
    }
  }
}

} // namespace
} // namespace rn