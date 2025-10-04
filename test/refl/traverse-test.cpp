/****************************************************************
**traverse-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-10-04.
*
* Description: Unit tests for the refl/traverse module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/refl/traverse.hpp"

// Testing.
#include "test/rds/testing.rds.hpp"

// refl
#include "src/refl/to-str.hpp"

// traverse
#include "src/traverse/ext-base.hpp"
#include "src/traverse/ext.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

using namespace std;

/****************************************************************
** MyStruct2Wrapper
*****************************************************************/
namespace rn {
namespace {

struct MyStruct2Wrapper {
  MyStruct2Wrapper( MyStruct2 ms2 )
    : wrapped( std::move( ms2 ) ) {}

  // Implement refl::WrapsReflected.
  [[maybe_unused]] MyStruct2 const& refl() const {
    return wrapped;
  }
  static constexpr string_view refl_ns   = "rn";
  static constexpr string_view refl_name = "MyStruct2Wrapper";

  MyStruct2 wrapped;
};

} // namespace
} // namespace rn

static_assert( refl::WrapsReflected<rn::MyStruct2Wrapper> );
static_assert( base::Show<rn::MyStruct2Wrapper> );

namespace refl {
namespace {

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[refl/traverse] struct" ) {
  ::rn::MyStruct2 ms2;

  auto const f = [&] [[clang::noinline]] ( auto const& fn ) {
    trv::traverse( ms2, fn );
  };

  ms2.xxx               = 5;
  ms2.yyy               = 4.7;
  ms2.zzz_map["hello"]  = "world";
  ms2.zzz_map["hello2"] = "world2";

  vector<string> v;
  f( [&]<typename V>( V const& val, string_view const name ) {
    // Field name.
    v.push_back( string( name ) );

    // Type.
    if constexpr( is_same_v<V, int> )
      v.push_back( "int" );
    else if constexpr( is_same_v<V, double> )
      v.push_back( "double" );
    else if constexpr( is_same_v<V, map<string, string>> )
      v.push_back( "map<string,string>" );
    else
      static_assert( is_same_v<void, V>, "unexpected type" );

    // Value.
    v.push_back( format( "val: {}", val ) );
  } );

  REQUIRE( v == vector<string>{
                  "xxx",                              //
                  "int",                              //
                  "val: 5",                           //
                  "yyy",                              //
                  "double",                           //
                  "val: 4.7",                         //
                  "zzz_map",                          //
                  "map<string,string>",               //
                  "val: {hello=world,hello2=world2}", //
                } );
}

TEST_CASE( "[refl/traverse] wrapped" ) {
  ::rn::MyStruct2 ms2;
  ms2.xxx               = 5;
  ms2.yyy               = 4.7;
  ms2.zzz_map["hello"]  = "world";
  ms2.zzz_map["hello2"] = "world2";

  ::rn::MyStruct2Wrapper ms2w( ms2 );

  auto const f = [&] [[clang::noinline]] ( auto const& fn ) {
    trv::traverse( ms2w, fn );
  };

  vector<string> v;
  f( [&]<typename V>( V const& val, trv::none_t const ) {
    // Type.
    if constexpr( is_same_v<V, ::rn::MyStruct2> )
      v.push_back( "MyStruct2" );
    else
      static_assert( is_same_v<void, V>, "unexpected type" );

    // Value.
    v.push_back( format( "val: {}", val ) );
  } );

  REQUIRE(
      v ==
      vector<string>{
        "MyStruct2",                                         //
        "val: MyStruct2{xxx=5,yyy=4.7,zzz_map={hello=world," //
        "hello2=world2}}",                                   //
      } );
}

TEST_CASE( "[refl/traverse] reflected variants" ) {
  using namespace ::rdstest;
  MyVariant2 mv;

  auto const f = [&] [[clang::noinline]] ( auto const& o,
                                           auto const& fn ) {
    trv::traverse( o, fn );
  };

  vector<string> v;

  auto const go = [&] [[clang::noinline]] ( auto const& o ) {
    f( o, [&]<typename V, typename K>( V const& val,
                                       K const& key ) {
      if constexpr( is_same_v<K, trv::none_t> )
        v.push_back( "none_t" );
      else if constexpr( base::Show<K> )
        v.push_back( string( key ) );
      else
        static_assert( is_same_v<void, K>, "unexpected type" );
      v.push_back( format( "val: {}", val ) );
    } );
  };

  v.clear();
  mv = MyVariant2::first{ .name = "hello", .b = true };
  go( mv );
  REQUIRE(
      v ==
      vector<string>{
        "none_t",
        "val: rdstest::MyVariant2::first{name=hello,b=true}" } );

  v.clear();
  mv = MyVariant2::first{ .name = "hello", .b = true };
  go( mv.as_base() );
  REQUIRE(
      v ==
      vector<string>{
        "first",
        "val: rdstest::MyVariant2::first{name=hello,b=true}" } );

  v.clear();
  mv = MyVariant2::second{ .flag1 = true, .flag2 = false };
  go( mv.as_base() );
  REQUIRE( v == vector<string>{ "second",
                                "val: "
                                "rdstest::MyVariant2::second{"
                                "flag1=true,flag2=false}" } );

  v.clear();
  mv = MyVariant2::third{ .cost = 3 };
  go( mv.as_base() );
  REQUIRE( v == vector<string>{
                  "third",
                  "val: rdstest::MyVariant2::third{cost=3}" } );
}

} // namespace
} // namespace refl
