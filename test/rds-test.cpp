/****************************************************************
**rds.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-11-02.
*
* Description: Unit tests for the rds language.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "rds/testing.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/build-properties.hpp"
#include "base/fmt.hpp"
#include "base/fs.hpp"
#include "base/io.hpp"
#include "base/to-str-ext-std.hpp"

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

  friend void to_str( my_optional const& o, std::string& out,
                      base::ADL_t ) {
    to_str( o.t, out, base::ADL );
  }
};

struct String {
  String( char const* s ) : s_( s ) {}

  friend void to_str( String const& o, std::string& out,
                      base::ADL_t ) {
    out += o.s_;
  }

  std::string s_;
};
} // namespace rn

namespace rn {

namespace {

using namespace std;
using namespace rdstest;

using Catch::Contains;

static_assert( is_same_v<::rdstest::Maybe<int>, Maybe<int>> );
static_assert(
    is_same_v<::rdstest::inner::MyVariant3, inner::MyVariant3> );

TEST_CASE( "[rds] Maybe" ) {
  Maybe<int> maybe;
  REQUIRE( fmt::format( "{}", maybe ) ==
           "rdstest::Maybe<int>::nothing" );

  maybe = Maybe<int>::nothing{};
  REQUIRE( fmt::format( "{}", maybe ) ==
           "rdstest::Maybe<int>::nothing" );

  auto just = Maybe<int>::just{ 5 };
  static_assert( is_same_v<decltype( just.val ), int> );
  maybe = just;
  REQUIRE( fmt::format( "{}", maybe ) ==
           "rdstest::Maybe<int>::just{val=5}" );

  switch( maybe.to_enum() ) {
    case Maybe<int>::e::nothing: //
      REQUIRE( false );
      break;
    case Maybe<int>::e::just: {
      auto& val = maybe.get<Maybe<int>::just>();
      REQUIRE( val.val == 5 );
      break;
    }
  }

  Maybe<rn::my_optional<char>> maybe_op_str;
  REQUIRE( fmt::format( "{}", maybe_op_str ) ==
           "rdstest::Maybe<rn::my_optional<char>>::nothing" );
  maybe_op_str = Maybe<rn::my_optional<char>>::just{ { 'c' } };
  REQUIRE(
      fmt::format( "{}", maybe_op_str ) ==
      "rdstest::Maybe<rn::my_optional<char>>::just{val=c}" );
}

TEST_CASE( "[rds] MyVariant1" ) {
  static_assert( base::Show<MyVariant1> );
  MyVariant1 my1;
  my1    = MyVariant1::happy{ { 'c', 4 } };
  my1    = MyVariant1::excited{};
  bool b = true;
  my1    = MyVariant1::sad{ true, &b };
  switch( my1.to_enum() ) {
    case MyVariant1::e::happy: //
      REQUIRE( false );
      break;
    case MyVariant1::e::excited:
      REQUIRE( false );
      static_assert( sizeof( MyVariant1::excited ) == 1 );
      break;
    case MyVariant1::e::sad:
      auto& val = my1.get<MyVariant1::sad>();
      static_assert( is_same_v<decltype( val.hello ), bool> );
      static_assert( is_same_v<decltype( val.ptr ), bool*> );
      REQUIRE( val.hello == true );
      REQUIRE( *val.ptr == true );
      break;
  }
}

TEST_CASE( "[rds] MyVariant2" ) {
  static_assert( base::Show<MyVariant2> );
  MyVariant2 my2;
  my2 = MyVariant2::first{ "hello", true };
  my2 = MyVariant2::second{ true, false };
  my2 = MyVariant2::third{ 7 };
  switch( my2.to_enum() ) {
    case MyVariant2::e::first: {
      auto& val = my2.get<MyVariant2::first>();
      static_assert( is_same_v<decltype( val.name ), string> );
      static_assert( is_same_v<decltype( val.b ), bool> );
      break;
    }
    case MyVariant2::e::second: {
      auto& val = my2.get<MyVariant2::second>();
      static_assert( is_same_v<decltype( val.flag1 ), bool> );
      static_assert( is_same_v<decltype( val.flag2 ), bool> );
      break;
    }
    case MyVariant2::e::third: {
      auto& val = my2.get<MyVariant2::third>();
      static_assert( is_same_v<decltype( val.cost ), int> );
      break;
    }
  }
  REQUIRE( fmt::format( "{}", my2 ) ==
           "rdstest::MyVariant2::third{cost=7}" );
}

TEST_CASE( "[rds] MyVariant3" ) {
  static_assert( base::Show<inner::MyVariant3> );
  inner::MyVariant3 my3;
  my3 = inner::MyVariant3::a1{ monostate{} };
  my3 = inner::MyVariant3::a2{ monostate{}, MyVariant2{} };
  my3 = inner::MyVariant3::a3{ 'r' };
  switch( my3.to_enum() ) {
    case inner::MyVariant3::e::a1: {
      REQUIRE( false );
      auto& val = my3.get<inner::MyVariant3::a1>();
      static_assert(
          is_same_v<decltype( val.var0 ), monostate> );
      break;
    }
    case inner::MyVariant3::e::a2: {
      auto& val = my3.get<inner::MyVariant3::a2>();
      REQUIRE( false );
      static_assert(
          is_same_v<decltype( val.var1 ), monostate> );
      static_assert(
          is_same_v<decltype( val.var2 ), MyVariant2> );
      break;
    }
    case inner::MyVariant3::e::a3: {
      auto& val = my3.get<inner::MyVariant3::a3>();
      REQUIRE( val.c == 'r' );
      break;
    }
  }
}

TEST_CASE( "[rds] MyVariant4" ) {
  static_assert( base::Show<inner::MyVariant4> );
  inner::MyVariant4 my4;
  my4 = inner::MyVariant4::first{ 1, 'r', true, { 3 } };
  my4 = inner::MyVariant4::_2nd{};
  my4 = inner::MyVariant4::third{ "hello",
                                  inner::MyVariant3::a3{ 'e' } };
  switch( my4.to_enum() ) {
    case inner::MyVariant4::e::first: {
      REQUIRE( false );
      auto& val = my4.get<inner::MyVariant4::first>();
      static_assert(
          is_same_v<decltype( val.op ), maybe<uint32_t>> );
      break;
    }
    case inner::MyVariant4::e::_2nd: {
      REQUIRE( false );
      break;
    }
    case inner::MyVariant4::e::third: {
      auto& val = my4.get<inner::MyVariant4::third>();
      REQUIRE( val.s == "hello" );
      switch( val.var3.to_enum() ) {
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
              val.var3.get<inner::MyVariant3::a3>();
          REQUIRE( val_inner.c == 'e' );
          break;
        }
      }
      break;
    }
  }
  REQUIRE( fmt::format( "{}", my4 ) ==
           "rdstest::inner::MyVariant4::third{s=hello,var3="
           "rdstest::inner::MyVariant3::a3{c=e}}" );
}

TEST_CASE( "[rds] CompositeTemplateTwo" ) {
  using V =
      inner::CompositeTemplateTwo<rn::my_optional<int>, short>;
  string out;
  base::to_str( V{}, out, base::ADL_t{} );
  // static_assert( base::Show<V> );
  V v = inner::CompositeTemplateTwo<rn::my_optional<int>,
                                    short>::first{
      .ttp = inner::TemplateTwoParams<rn::my_optional<int>,
                                      short>::third_alternative{
          .hello =
              Maybe<rn::my_optional<int>>::just{
                  rn::my_optional<int>{ 3 } }, //
          .u = 9                               //
      } };
  using second_t =
      inner::CompositeTemplateTwo<rn::my_optional<int>,
                                  short>::second;
  static_assert( sizeof( second_t ) == 1 );
  switch( v.to_enum() ) {
    case V::e::first:  //
      break;
    case V::e::second: //
      REQUIRE( false );
      break;
  }
  REQUIRE(
      fmt::format( "{}", v ) ==
      "rdstest::inner::CompositeTemplateTwo<rn::my_optional<int>"
      ",short>::first{ttp=rdstest::inner::TemplateTwoParams<rn::"
      "my_optional<int>,short>::third_alternative{hello=rdstest:"
      ":Maybe<rn::my_optional<int>>::just{val=3},u=9}}" );
}

TEST_CASE( "[rds] Equality" ) {
  // Maybe
  REQUIRE( Maybe<int>::nothing{} == Maybe<int>::nothing{} );
  REQUIRE( Maybe<int>::just{ 5 } == Maybe<int>::just{ 5 } );
  REQUIRE( Maybe<int>::just{ 5 } != Maybe<int>::just{ 6 } );

  // TemplateTwoParams_t
  using T =
      inner::TemplateTwoParams<string, int>::first_alternative;
  REQUIRE( T{ "a", 'c' } == T{ "a", 'c' } );
  REQUIRE( T{ "a", 'b' } != T{ "a", 'c' } );
  REQUIRE( T{ "b", 'a' } != T{ "c", 'a' } );
}

TEST_CASE( "[rds] Rds File Golden Comparison" ) {
  auto golden = base::read_text_file_as_string(
      testing::data_dir() / "rds-testing-golden.rds.hpp" );
  REQUIRE( golden.has_value() );
  fs::path root      = base::build_output_root();
  auto     generated = base::read_text_file_as_string(
      root / "test" / "rds" / "testing.rds.hpp" );
  REQUIRE( generated.has_value() );
  // Do this comparison outside of the REQUIRE macro so that
  // Catch2 doesn't try to print the values when they are not
  // equal.
  bool eq = ( generated == golden );
  REQUIRE( eq );
}

TEST_CASE( "[rds] Associated Enums" ) {
  using namespace rdstest;
  MyVariant2 v =
      MyVariant2::second{ .flag1 = false, .flag2 = true };
  switch( v.to_enum() ) {
    case MyVariant2::e::first: //
      REQUIRE( false );
      break;
    case MyVariant2::e::second: {
      // Make sure that we can get a non-const reference.
      MyVariant2::second& snd = v.get<MyVariant2::second>();
      REQUIRE( fmt::format( "{}", snd ) ==
               "rdstest::MyVariant2::second{flag1=false,flag2="
               "true}" );
      break;
    }
    case MyVariant2::e::third: //
      REQUIRE( false );
      break;
  }

  Maybe<String> maybe = Maybe<String>::just{ .val = "hello" };
  switch( maybe.to_enum() ) {
    case Maybe<String>::e::nothing: //
      REQUIRE( false );
      break;
    case Maybe<String>::e::just: {
      auto& just = maybe.get<Maybe<String>::just>();
      REQUIRE( fmt::format( "{}", just ) ==
               "rdstest::Maybe<rn::String>::just{val=hello}" );
      break;
    }
  }
}

TEST_CASE( "[rds] enums" ) {
  // e_empty
  static_assert( refl::enum_count<e_empty> == 0 );
  static_assert( refl::traits<e_empty>::name == "e_empty" );
  static_assert( refl::enum_values<e_empty>.size() == 0 );
  static_assert( refl::enum_from_integral<e_empty>( 0 ) ==
                 nothing );
  static_assert( refl::enum_from_integral<e_empty>( 1 ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_empty>( "" ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_empty>( "hello" ) ==
                 nothing );

  // e_single
  static_assert( refl::enum_count<e_single> == 1 );
  static_assert( refl::traits<e_single>::name == "e_single" );
  static_assert( refl::enum_values<e_single>.size() == 1 );
  static_assert( refl::enum_value_name<e_single>(
                     e_single::hello ) == "hello" );
  static_assert( refl::enum_from_integral<e_single>( 0 ) ==
                 e_single::hello );
  static_assert( refl::enum_from_integral<e_single>( 1 ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_single>( "" ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_single>( "hello" ) ==
                 e_single::hello );
  static_assert( refl::enum_from_string<e_single>( "hellox" ) ==
                 nothing );

  // e_two
  static_assert( refl::enum_count<e_two> == 2 );
  static_assert( refl::traits<e_two>::name == "e_two" );
  static_assert( refl::enum_values<e_two>.size() == 2 );
  static_assert( refl::enum_value_name<e_two>( e_two::hello ) ==
                 "hello" );
  static_assert( refl::enum_value_name<e_two>( e_two::world ) ==
                 "world" );
  static_assert( refl::enum_from_integral<e_two>( 0 ) ==
                 e_two::hello );
  static_assert( refl::enum_from_integral<e_two>( 1 ) ==
                 e_two::world );
  static_assert( refl::enum_from_integral<e_two>( 2 ) ==
                 nothing );
  static_assert( refl::enum_from_integral<e_two>( 10 ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_two>( "" ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_two>( "hello" ) ==
                 e_two::hello );
  static_assert( refl::enum_from_string<e_two>( "hellox" ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_two>( "world" ) ==
                 e_two::world );

  // e_color
  static_assert( refl::enum_count<e_color> == 3 );
  static_assert( refl::traits<e_color>::name == "e_color" );
  static_assert( refl::enum_values<e_color>.size() == 3 );
  static_assert(
      refl::enum_value_name<e_color>( e_color::red ) == "red" );
  static_assert( refl::enum_value_name<e_color>(
                     e_color::blue ) == "blue" );
  static_assert( refl::enum_from_integral<e_color>( 0 ) ==
                 e_color::red );
  static_assert( refl::enum_from_integral<e_color>( 1 ) ==
                 e_color::green );
  static_assert( refl::enum_from_integral<e_color>( 2 ) ==
                 e_color::blue );
  static_assert( refl::enum_from_integral<e_color>( 3 ) ==
                 nothing );
  static_assert( refl::enum_from_integral<e_color>( 10 ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_color>( "" ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_color>( "hello" ) ==
                 nothing );
  static_assert( refl::enum_from_string<e_color>( "green" ) ==
                 e_color::green );

  // base/derived.
  static_assert( refl::enum_value_as<e_count_short>(
                     e_count::two ) == e_count_short::two );
  static_assert(
      refl::enum_derives_from<e_count_short, e_count>() );
  static_assert(
      !refl::enum_derives_from<e_count, e_count_short>() );
}

TEST_CASE( "[rds] structs" ) {
  SECTION( "EmptyStruct" ) {
    using Tr = refl::traits<EmptyStruct>;
    static_assert( sizeof( EmptyStruct ) == 1 );
    static_assert( is_same_v<Tr::type, EmptyStruct> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rn" );
    static_assert( Tr::name == "EmptyStruct" );
    static_assert( is_same_v<Tr::template_types, tuple<>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 0 );
  }
  SECTION( "MyStruct" ) {
    using Tr = refl::traits<MyStruct>;
    static_assert( is_same_v<Tr::type, MyStruct> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rn" );
    static_assert( Tr::name == "MyStruct" );
    static_assert( is_same_v<Tr::template_types, tuple<>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 3 );
    MyStruct ms{
        .xxx     = 5,
        .yyy     = 2.3,
        .zzz_map = { { "hello", "1" }, { "world", "2" } },
    };
    { // field 0
      auto& [name, acc, off] = std::get<0>( Tr::fields );
      static_assert( name == "xxx" );
      REQUIRE( ms.xxx == 5 );
      ( ms.*acc ) = 6;
      REQUIRE( ms.xxx == 6 );
      REQUIRE( ( off.index() == 1 && off.get<size_t>() == 0 ) );
    }
    { // field 1
      auto& [name, acc, off] = std::get<1>( Tr::fields );
      static_assert( name == "yyy" );
      REQUIRE( ms.yyy == 2.3 );
      ( ms.*acc ) = 3.2;
      REQUIRE( ms.yyy == 3.2 );
      REQUIRE( ( off.index() == 1 && off.get<size_t>() > 0 ) );
    }
    { // field 2
      auto& [name, acc, off] = std::get<2>( Tr::fields );
      static_assert( name == "zzz_map" );
      REQUIRE( ms.zzz_map ==
               unordered_map<string, string>{
                   { "hello", "1" }, { "world", "2" } } );
      ( ms.*acc ) =
          unordered_map<string, string>{ { "one", "two" } };
      REQUIRE( ms.zzz_map == unordered_map<string, string>{
                                 { "one", "two" } } );
      REQUIRE( ( off.index() == 1 && off.get<size_t>() > 0 ) );
    }
  }
  SECTION( "MyTemplateStruct" ) {
    using Tr = refl::traits<test::MyTemplateStruct<int, string>>;
    static_assert(
        is_same_v<Tr::type,
                  test::MyTemplateStruct<int, string>> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rn::test" );
    static_assert( Tr::name == "MyTemplateStruct" );
    static_assert(
        is_same_v<Tr::template_types, tuple<int, string>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 3 );
    test::MyTemplateStruct<int, string> mts{
        .xxx     = 5,
        .yyy     = 2.3,
        .zzz_map = { { "hello", "1" }, { "world", "2" } },
    };
    { // field 0
      auto& [name, acc, off] = std::get<0>( Tr::fields );
      static_assert( name == "xxx" );
      REQUIRE( mts.xxx == 5 );
      ( mts.*acc ) = 6;
      REQUIRE( mts.xxx == 6 );
      REQUIRE( off.index() == 0 );
    }
    { // field 1
      auto& [name, acc, off] = std::get<1>( Tr::fields );
      static_assert( name == "yyy" );
      REQUIRE( mts.yyy == 2.3 );
      ( mts.*acc ) = 3.2;
      REQUIRE( mts.yyy == 3.2 );
      REQUIRE( off.index() == 0 );
    }
    { // field 2
      auto& [name, acc, off] = std::get<2>( Tr::fields );
      static_assert( name == "zzz_map" );
      REQUIRE( mts.zzz_map ==
               unordered_map<string, string>{
                   { "hello", "1" }, { "world", "2" } } );
      ( mts.*acc ) =
          unordered_map<string, string>{ { "one", "two" } };
      REQUIRE( mts.zzz_map == unordered_map<string, string>{
                                  { "one", "two" } } );
      REQUIRE( off.index() == 0 );
    }
  }
}

TEST_CASE( "[rds] sumtype reflection" ) {
  SECTION( "none" ) {
    using Tr = refl::traits<MySumtype::none>;
    static_assert( is_same_v<Tr::type, MySumtype::none> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rn::MySumtype" );
    static_assert( Tr::name == "none" );
    static_assert( is_same_v<Tr::template_types, tuple<>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 0 );
  }
  SECTION( "some" ) {
    using Tr = refl::traits<MySumtype::some>;
    static_assert( is_same_v<Tr::type, MySumtype::some> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rn::MySumtype" );
    static_assert( Tr::name == "some" );
    static_assert( is_same_v<Tr::template_types, tuple<>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 2 );
    MySumtype::some ms{
        .s = "hello",
        .y = 5,
    };
    { // field 0
      auto& [name, acc, off] = std::get<0>( Tr::fields );
      static_assert( name == "s" );
      REQUIRE( ms.s == "hello" );
      ( ms.*acc ) = "world";
      REQUIRE( ms.s == "world" );
      REQUIRE( off.index() == 0 );
    }
    { // field 1
      auto& [name, acc, off] = std::get<1>( Tr::fields );
      static_assert( name == "y" );
      REQUIRE( ms.y == 5 );
      ( ms.*acc ) = 6;
      REQUIRE( ms.y == 6 );
      REQUIRE( off.index() == 0 );
    }
  }
  SECTION( "more" ) {
    using Tr = refl::traits<MySumtype::more>;
    static_assert( is_same_v<Tr::type, MySumtype::more> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rn::MySumtype" );
    static_assert( Tr::name == "more" );
    static_assert( is_same_v<Tr::template_types, tuple<>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 1 );
    MySumtype::more ms{
        .d = 2.3,
    };
    { // field 0
      auto& [name, acc, off] = std::get<0>( Tr::fields );
      static_assert( name == "d" );
      REQUIRE( ms.d == 2.3 );
      ( ms.*acc ) = 3.2;
      REQUIRE( ms.d == 3.2 );
      REQUIRE( off.index() == 0 );
    }
  }
}

TEST_CASE( "[rds] sumtype reflection w/ templates" ) {
  SECTION( "nothing" ) {
    using Tr = refl::traits<rdstest::Maybe<int>::nothing>;
    static_assert(
        is_same_v<Tr::type, rdstest::Maybe<int>::nothing> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rdstest::Maybe" );
    static_assert( Tr::name == "nothing" );
    static_assert( is_same_v<Tr::template_types, tuple<int>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 0 );
  }
  SECTION( "just" ) {
    using Tr = refl::traits<rdstest::Maybe<int>::just>;
    static_assert(
        is_same_v<Tr::type, rdstest::Maybe<int>::just> );
    static_assert( Tr::kind == refl::type_kind::struct_kind );
    static_assert( Tr::ns == "rdstest::Maybe" );
    static_assert( Tr::name == "just" );
    static_assert( is_same_v<Tr::template_types, tuple<int>> );

    static_assert( tuple_size_v<decltype( Tr::fields )> == 1 );
    rdstest::Maybe<int>::just ms{
        .val = 2,
    };
    { // field 0
      auto& [name, acc, off] = std::get<0>( Tr::fields );
      static_assert( name == "val" );
      REQUIRE( ms.val == 2 );
      ( ms.*acc ) = 3;
      REQUIRE( ms.val == 3 );
      REQUIRE( off.index() == 0 );
    }
  }
}

TEST_CASE( "[rds] validate method" ) {
  static_assert( !refl::ValidatableStruct<EmptyStruct> );
  static_assert( !refl::ValidatableStruct<EmptyStruct2> );
  static_assert( !refl::ValidatableStruct<MyStruct> );
  static_assert( refl::ValidatableStruct<StructWithValidation> );
  static_assert( !refl::ValidatableStruct<
                 test::MyTemplateStruct<int, int>> );
}

TEST_CASE( "[rds] equality_comparable" ) {
  static_assert( equality_comparable<EmptyStruct> );
  static_assert( !equality_comparable<EmptyStruct2> );
  static_assert( equality_comparable<MyStruct> );
  static_assert( equality_comparable<StructWithValidation> );
  static_assert(
      equality_comparable<test::MyTemplateStruct<int, int>> );
}

TEST_CASE( "[rds] config" ) {
  REQUIRE( test::config_testing.some_field == 5 );
}

} // namespace
} // namespace rn