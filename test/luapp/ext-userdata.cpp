/****************************************************************
**ext-userdata.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-21.
*
* Description: Unit tests for the src/luapp/ext-userdata.*
*              module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/ext-userdata.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/cast.hpp"
#include "src/luapp/ext-base.hpp"
#include "src/luapp/func-push.hpp"
#include "src/luapp/ruserdata.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {

struct CppOwned {
  int n = 5;
};

LUA_USERDATA_TRAITS( CppOwned, owned_by_cpp ){};
static_assert( HasUserdataOwnershipModel<CppOwned> );
static_assert( HasRefUserdataOwnershipModel<CppOwned> );
static_assert( !HasValueUserdataOwnershipModel<CppOwned> );

struct CppOwnedNonCopyable {
  int n = 5;

  CppOwnedNonCopyable()                             = default;
  CppOwnedNonCopyable( CppOwnedNonCopyable const& ) = delete;
  CppOwnedNonCopyable& operator=( CppOwnedNonCopyable const& ) =
      delete;
  CppOwnedNonCopyable( CppOwnedNonCopyable&& ) = default;
  CppOwnedNonCopyable& operator=( CppOwnedNonCopyable&& ) =
      default;
};

LUA_USERDATA_TRAITS( CppOwnedNonCopyable, owned_by_cpp ){};
static_assert( HasUserdataOwnershipModel<CppOwnedNonCopyable> );
static_assert(
    HasRefUserdataOwnershipModel<CppOwnedNonCopyable> );
static_assert(
    !HasValueUserdataOwnershipModel<CppOwnedNonCopyable> );

struct LuaOwned {
  int n = 5;
};

LUA_USERDATA_TRAITS( LuaOwned, owned_by_lua ){};
static_assert( HasUserdataOwnershipModel<LuaOwned> );
static_assert( !HasRefUserdataOwnershipModel<LuaOwned> );
static_assert( HasValueUserdataOwnershipModel<LuaOwned> );

struct LuaOwnedNonCopyable {
  int n = 5;

  LuaOwnedNonCopyable()                             = default;
  LuaOwnedNonCopyable( LuaOwnedNonCopyable const& ) = delete;
  LuaOwnedNonCopyable& operator=( LuaOwnedNonCopyable const& ) =
      delete;
  LuaOwnedNonCopyable( LuaOwnedNonCopyable&& ) = default;
  LuaOwnedNonCopyable& operator=( LuaOwnedNonCopyable&& ) =
      default;
};

LUA_USERDATA_TRAITS( LuaOwnedNonCopyable, owned_by_lua ){};
static_assert( HasUserdataOwnershipModel<LuaOwnedNonCopyable> );
static_assert(
    !HasRefUserdataOwnershipModel<LuaOwnedNonCopyable> );
static_assert(
    HasValueUserdataOwnershipModel<LuaOwnedNonCopyable> );

struct NoOwnershipModel {};
static_assert( !HasTraitsNvalues<NoOwnershipModel> );
static_assert( !HasUserdataOwnershipModel<NoOwnershipModel> );

struct NoOwnershipModelButHasTraits {};
template<>
struct type_traits<NoOwnershipModelButHasTraits> {
  static constexpr int nvalues = 1;
};
static_assert( HasTraitsNvalues<NoOwnershipModelButHasTraits> );
static_assert(
    !HasUserdataOwnershipModel<NoOwnershipModelButHasTraits> );

} // namespace lua

DEFINE_FORMAT( ::lua::CppOwned, "CppOwned{{n={}}}", o.n );
DEFINE_FORMAT( ::lua::LuaOwned, "LuaOwned{{n={}}}", o.n );

namespace lua {

namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;
using ::base::valid;
using ::Catch::Matches;

LUA_TEST_CASE( "[ext-userdata] cpp owned" ) {
  C.openlibs();
  static_assert( !is_assignable_v<decltype( st["x"] ),
                                  decltype( CppOwned{} )> );
  static_assert( !Pushable<CppOwned&&> );
  static_assert( !Pushable<CppOwned> );
  static_assert( Pushable<CppOwned&> );
  static_assert( !Gettable<CppOwned> );
  static_assert( !Gettable<CppOwned&&> );
  static_assert( Gettable<CppOwned&> );
  static_assert( Gettable<CppOwned const&> );
  static_assert( StorageGettable<CppOwned> );
  static_assert( !StorageGettable<CppOwned&&> );
  static_assert( StorageGettable<CppOwned&> );
  static_assert( StorageGettable<CppOwned const&> );
  static_assert( !Castable<decltype( st["x"] ), CppOwned> );
  static_assert( !Castable<decltype( st["x"] ), CppOwned&&> );
  static_assert( Castable<decltype( st["x"] ), CppOwned&> );

  CppOwned o;
  st["x"] = o;
  any a   = st["x"];
  REQUIRE( type_of( a ) == type::userdata );
  userdata ud = cast<userdata>( a );
  REQUIRE_THAT(
      fmt::format( "{}", ud ),
      Matches(
          "lua::CppOwned&@0x[0-9a-z]+: CppOwned\\{n=5\\}" ) );

  decltype( auto ) o2 = cast<CppOwned&>( st["x"] );
  static_assert( is_same_v<decltype( o2 ), CppOwned&> );

  REQUIRE( o2.n == 5 );
  REQUIRE( &o2 == &o );

  REQUIRE( cast<bool>( st["x"]["is_owned_by_lua"] ) == false );
  REQUIRE( C.dostring( "assert( not x.is_owned_by_lua )" ) ==
           valid );

  st["foo"] = []( CppOwned const& o0, CppOwned& o1,
                  CppOwned o2 ) { return o0.n + o1.n + o2.n; };
  REQUIRE( cast<int>( st["foo"]( o, o, o ) ) == 5 * 3 );

  static_assert( !Pushable<maybe<CppOwned>> );
  static_assert( Pushable<maybe<CppOwned&>> );
  static_assert( !Pushable<maybe<CppOwned const&>> );
  static_assert( !Gettable<maybe<CppOwned>> );
  static_assert( Gettable<maybe<CppOwned&>> );
  static_assert( Gettable<maybe<CppOwned const&>> );
  static_assert( !StorageGettable<maybe<CppOwned>> );
  static_assert( StorageGettable<maybe<CppOwned&>> );
  static_assert( StorageGettable<maybe<CppOwned const&>> );
  static_assert(
      !Castable<decltype( st["x"] ), maybe<CppOwned>> );
  static_assert(
      Castable<decltype( st["x"] ), maybe<CppOwned&>> );
  static_assert(
      Castable<decltype( st["x"] ), maybe<CppOwned const&>> );

  REQUIRE( cast<maybe<CppOwned&>>( st["foo"]( o, o, o ) ) ==
           nothing );
  REQUIRE( cast<maybe<CppOwned&>>( st["x"] ).value().n == 5 );
}

LUA_TEST_CASE( "[ext-userdata] cpp owned non copyable" ) {
  C.openlibs();
  static_assert(
      !is_assignable_v<decltype( st["x"] ),
                       decltype( CppOwnedNonCopyable{} )> );
  static_assert( !Pushable<CppOwnedNonCopyable&&> );
  static_assert( !Pushable<CppOwnedNonCopyable> );
  static_assert( Pushable<CppOwnedNonCopyable&> );
  static_assert( !Gettable<CppOwnedNonCopyable> );
  static_assert( !Gettable<CppOwnedNonCopyable&&> );
  static_assert( Gettable<CppOwnedNonCopyable&> );
  static_assert( Gettable<CppOwnedNonCopyable const&> );
  static_assert( !StorageGettable<CppOwnedNonCopyable> );
  static_assert( !StorageGettable<CppOwnedNonCopyable&&> );
  static_assert( StorageGettable<CppOwnedNonCopyable&> );
  static_assert( StorageGettable<CppOwnedNonCopyable const&> );
  static_assert(
      !Castable<decltype( st["x"] ), CppOwnedNonCopyable> );
  static_assert(
      !Castable<decltype( st["x"] ), CppOwnedNonCopyable&&> );
  static_assert(
      Castable<decltype( st["x"] ), CppOwnedNonCopyable&> );

  CppOwnedNonCopyable o;
  st["x"] = o;
  any a   = st["x"];
  REQUIRE( type_of( a ) == type::userdata );
  userdata ud = cast<userdata>( a );
  REQUIRE_THAT(
      fmt::format( "{}", ud ),
      Matches( "lua::CppOwnedNonCopyable&: 0x[0-9a-z]+" ) );

  decltype( auto ) o2 = cast<CppOwnedNonCopyable&>( st["x"] );
  static_assert(
      is_same_v<decltype( o2 ), CppOwnedNonCopyable&> );

  REQUIRE( o2.n == 5 );
  REQUIRE( &o2 == &o );

  REQUIRE( cast<bool>( st["x"]["is_owned_by_lua"] ) == false );
  REQUIRE( C.dostring( "assert( not x.is_owned_by_lua )" ) ==
           valid );

  st["foo"] = []( CppOwnedNonCopyable const& o0,
                  CppOwnedNonCopyable&       o1 ) {
    return o0.n + o1.n;
  };
  REQUIRE( cast<int>( st["foo"]( o, o ) ) == 5 * 2 );

  static_assert( !Pushable<maybe<CppOwnedNonCopyable>> );
  static_assert( Pushable<maybe<CppOwnedNonCopyable&>> );
  static_assert( !Pushable<maybe<CppOwnedNonCopyable const&>> );
  static_assert( !Gettable<maybe<CppOwnedNonCopyable>> );
  static_assert( Gettable<maybe<CppOwnedNonCopyable&>> );
  static_assert( Gettable<maybe<CppOwnedNonCopyable const&>> );
  static_assert( !StorageGettable<maybe<CppOwnedNonCopyable>> );
  static_assert( StorageGettable<maybe<CppOwnedNonCopyable&>> );
  static_assert(
      StorageGettable<maybe<CppOwnedNonCopyable const&>> );
  static_assert( !Castable<decltype( st["x"] ),
                           maybe<CppOwnedNonCopyable>> );
  static_assert( Castable<decltype( st["x"] ),
                          maybe<CppOwnedNonCopyable&>> );
  static_assert( Castable<decltype( st["x"] ),
                          maybe<CppOwnedNonCopyable const&>> );

  REQUIRE( cast<maybe<CppOwnedNonCopyable&>>(
               st["foo"]( o, o ) ) == nothing );
  REQUIRE(
      cast<maybe<CppOwnedNonCopyable&>>( st["x"] ).value().n ==
      5 );
}

LUA_TEST_CASE( "[ext-userdata] lua owned" ) {
  C.openlibs();
  static_assert( is_assignable_v<decltype( st["x"] ),
                                 decltype( LuaOwned{} )> );
  static_assert( Pushable<LuaOwned&&> );
  static_assert( Pushable<LuaOwned> );
  static_assert( !Pushable<LuaOwned&> );
  static_assert( !Gettable<LuaOwned> );
  static_assert( !Gettable<LuaOwned&&> );
  static_assert( Gettable<LuaOwned&> );
  static_assert( Gettable<LuaOwned const&> );
  static_assert( StorageGettable<LuaOwned> );
  static_assert( !StorageGettable<LuaOwned&&> );
  static_assert( StorageGettable<LuaOwned&> );
  static_assert( StorageGettable<LuaOwned const&> );
  static_assert( !Castable<decltype( st["x"] ), LuaOwned> );
  static_assert( !Castable<decltype( st["x"] ), LuaOwned&&> );
  static_assert( Castable<decltype( st["x"] ), LuaOwned&> );

  LuaOwned o;
  st["x"] = std::move( o );
  any a   = st["x"];
  REQUIRE( type_of( a ) == type::userdata );
  userdata ud = cast<userdata>( a );
  REQUIRE_THAT(
      fmt::format( "{}", ud ),
      Matches(
          "lua::LuaOwned@0x[0-9a-z]+: LuaOwned\\{n=5\\}" ) );

  decltype( auto ) o2 = cast<LuaOwned&>( st["x"] );
  static_assert( is_same_v<decltype( o2 ), LuaOwned&> );

  REQUIRE( o2.n == 5 );
  REQUIRE( &o2 != &o );

  REQUIRE( cast<bool>( st["x"]["is_owned_by_lua"] ) == true );
  REQUIRE( C.dostring( "assert( x.is_owned_by_lua )" ) ==
           valid );

  st["foo"] = []( LuaOwned const& o0, LuaOwned& o1,
                  LuaOwned o2 ) { return o0.n + o1.n + o2.n; };
  REQUIRE( cast<int>( st["foo"]( LuaOwned{}, LuaOwned{},
                                 LuaOwned{} ) ) == 5 * 3 );

  static_assert( Pushable<maybe<LuaOwned>> );
  static_assert( !Pushable<maybe<LuaOwned&>> );
  static_assert( !Pushable<maybe<LuaOwned const&>> );
  static_assert( !Gettable<maybe<LuaOwned>> );
  static_assert( Gettable<maybe<LuaOwned&>> );
  static_assert( Gettable<maybe<LuaOwned const&>> );
  static_assert( !StorageGettable<maybe<LuaOwned>> );
  static_assert( StorageGettable<maybe<LuaOwned&>> );
  static_assert( StorageGettable<maybe<LuaOwned const&>> );
  static_assert(
      !Castable<decltype( st["x"] ), maybe<LuaOwned>> );
  static_assert(
      Castable<decltype( st["x"] ), maybe<LuaOwned&>> );
  static_assert(
      Castable<decltype( st["x"] ), maybe<LuaOwned const&>> );

  REQUIRE( cast<maybe<LuaOwned&>>( st["foo"](
               LuaOwned{}, LuaOwned{}, LuaOwned{} ) ) ==
           nothing );
  REQUIRE( cast<maybe<LuaOwned&>>( st["x"] ).value().n == 5 );
}

LUA_TEST_CASE( "[ext-userdata] lua owned non copyable" ) {
  C.openlibs();
  static_assert(
      is_assignable_v<decltype( st["x"] ),
                      decltype( LuaOwnedNonCopyable{} )> );
  static_assert( Pushable<LuaOwnedNonCopyable&&> );
  static_assert( Pushable<LuaOwnedNonCopyable> );
  static_assert( !Pushable<LuaOwnedNonCopyable&> );
  static_assert( !Gettable<LuaOwnedNonCopyable> );
  static_assert( !Gettable<LuaOwnedNonCopyable&&> );
  static_assert( Gettable<LuaOwnedNonCopyable&> );
  static_assert( !StorageGettable<LuaOwnedNonCopyable> );
  static_assert( !StorageGettable<LuaOwnedNonCopyable&&> );
  static_assert( StorageGettable<LuaOwnedNonCopyable&> );
  static_assert( StorageGettable<LuaOwnedNonCopyable const&> );
  static_assert(
      !Castable<decltype( st["x"] ), LuaOwnedNonCopyable> );
  static_assert(
      !Castable<decltype( st["x"] ), LuaOwnedNonCopyable&&> );
  static_assert(
      Castable<decltype( st["x"] ), LuaOwnedNonCopyable&> );

  LuaOwnedNonCopyable o;
  st["x"] = std::move( o );
  any a   = st["x"];
  REQUIRE( type_of( a ) == type::userdata );
  userdata ud = cast<userdata>( a );
  REQUIRE_THAT(
      fmt::format( "{}", ud ),
      Matches( "lua::LuaOwnedNonCopyable: 0x[0-9a-z]+" ) );

  decltype( auto ) o2 = cast<LuaOwnedNonCopyable&>( st["x"] );
  static_assert(
      is_same_v<decltype( o2 ), LuaOwnedNonCopyable&> );

  REQUIRE( o2.n == 5 );
  REQUIRE( &o2 != &o );

  REQUIRE( cast<bool>( st["x"]["is_owned_by_lua"] ) == true );
  REQUIRE( C.dostring( "assert( x.is_owned_by_lua )" ) ==
           valid );

  st["foo"] = []( LuaOwnedNonCopyable const& o0,
                  LuaOwnedNonCopyable&       o1 ) {
    return o0.n + o1.n;
  };
  REQUIRE( cast<int>( st["foo"]( LuaOwnedNonCopyable{},
                                 LuaOwnedNonCopyable{} ) ) ==
           5 * 2 );

  static_assert( Pushable<maybe<LuaOwnedNonCopyable>> );
  static_assert( !Pushable<maybe<LuaOwnedNonCopyable&>> );
  static_assert( !Pushable<maybe<LuaOwnedNonCopyable const&>> );
  static_assert( !Gettable<maybe<LuaOwnedNonCopyable>> );
  static_assert( Gettable<maybe<LuaOwnedNonCopyable&>> );
  static_assert( Gettable<maybe<LuaOwnedNonCopyable const&>> );
  static_assert( !StorageGettable<maybe<LuaOwnedNonCopyable>> );
  static_assert( StorageGettable<maybe<LuaOwnedNonCopyable&>> );
  static_assert(
      StorageGettable<maybe<LuaOwnedNonCopyable const&>> );
  static_assert( !Castable<decltype( st["x"] ),
                           maybe<LuaOwnedNonCopyable>> );
  static_assert( Castable<decltype( st["x"] ),
                          maybe<LuaOwnedNonCopyable&>> );
  static_assert( Castable<decltype( st["x"] ),
                          maybe<LuaOwnedNonCopyable const&>> );

  REQUIRE( cast<maybe<LuaOwnedNonCopyable&>>(
               st["foo"]( LuaOwnedNonCopyable{},
                          LuaOwnedNonCopyable{} ) ) == nothing );
  REQUIRE(
      cast<maybe<LuaOwnedNonCopyable&>>( st["x"] ).value().n ==
      5 );
}

} // namespace
} // namespace lua
