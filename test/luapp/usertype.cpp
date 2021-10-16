/****************************************************************
**usertype.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-23.
*
* Description: Unit tests for the src/luapp/usertype.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/usertype.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/cast.hpp"
#include "src/luapp/ruserdata.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {

using namespace std;

using ::base::valid;

struct CppOwnedType {
  int    n = 5;
  double d = 9.9;
  string s = "hello";

  int const n_const = 10;

  int get_n() const { return n; }
  int get_n_plus( int m ) const { return n + m; }

  string say( string const& what ) {
    return string( "saying: " ) + what;
  }
};

LUA_USERDATA_TRAITS( CppOwnedType, owned_by_cpp ){};
static_assert( HasUserdataOwnershipModel<CppOwnedType> );

struct LuaOwnedType {
  int    n = 5;
  double d = 9.9;
  string s = "hello";

  int const n_const = 10;

  int get_n() const { return n; }
  int get_n_plus( int m ) const { return n + m; }

  string say( string const& what ) {
    return string( "saying: " ) + what;
  }
};

LUA_USERDATA_TRAITS( LuaOwnedType, owned_by_lua ){};
static_assert( HasUserdataOwnershipModel<LuaOwnedType> );

struct HasMemberFunctionsWithCppTypesAsArgs {
  // For parameters, we can do anything except pass rvalue refer-
  // ences.
  void lua_owned( LuaOwnedType ) const {}
  void cpp_owned( CppOwnedType ) const {}
  void lua_owned_ref( LuaOwnedType& ) const {}
  void cpp_owned_ref( CppOwnedType& ) const {}
  void lua_owned_const_ref( LuaOwnedType const& ) const {}
  void cpp_owned_const_ref( CppOwnedType const& ) const {}

  // For return values, these are the only two that will work
  // with return values.
  LuaOwnedType  lua_owned_r() const { return {}; }
  CppOwnedType& cpp_owned_ref_r() const {
    static CppOwnedType o;
    return o;
  }
};

LUA_USERDATA_TRAITS( HasMemberFunctionsWithCppTypesAsArgs,
                     owned_by_cpp ){};
static_assert( HasUserdataOwnershipModel<
               HasMemberFunctionsWithCppTypesAsArgs> );

struct NotValid {};

} // namespace lua

DEFINE_FORMAT( ::lua::CppOwnedType,
               "CppOwnedType{{n={},d={},s={}}}", o.n, o.d, o.s );
DEFINE_FORMAT( ::lua::LuaOwnedType,
               "LuaOwnedType{{n={},d={},s={}}}", o.n, o.d, o.s );

namespace lua {
namespace {

/****************************************************************
** static tests
*****************************************************************/
static_assert(
    std::experimental::is_detected_v<usertype, CppOwnedType> );
static_assert(
    std::experimental::is_detected_v<usertype, LuaOwnedType> );
static_assert(
    !std::experimental::is_detected_v<usertype, NotValid> );

/****************************************************************
** runtime tests
*****************************************************************/
LUA_TEST_CASE( "[usertype] cpp owned" ) {
  C.openlibs();
  usertype<CppOwnedType> ut( L );
  REQUIRE( C.stack_size() == 0 );
  // ut.set_constructor([]{} );

  auto flattened_member_fn_1 = []( CppOwnedType& o, int n ) {
    return o.n + n;
  };

  ut["n"]          = &CppOwnedType::n;
  ut["n_const"]    = &CppOwnedType::n_const;
  ut["d"]          = &CppOwnedType::d;
  ut["s"]          = &CppOwnedType::s;
  ut["get_n"]      = &CppOwnedType::get_n;
  ut["get_n_plus"] = &CppOwnedType::get_n_plus;
  ut["say"]        = &CppOwnedType::say;
  ut["flat1"]      = +flattened_member_fn_1;
  ut["flat2"] = [n = 3]( CppOwnedType& o ) { return o.n + n; };
  REQUIRE( C.stack_size() == 0 );

  // Check if the various members tables have been set.
  CppOwnedType o;
  st["o"] = o;

  REQUIRE( cast<userdata>( st["o"] )[metatable_key]["__name"] ==
           "lua::CppOwnedType&" );
  REQUIRE( cast<userdata>( st["o"] ).name() ==
           "lua::CppOwnedType&" );

  REQUIRE( C.stack_size() == 0 );

  // Make sure that the wrong type name is not used.
  lua::push( L, st["o"] );
  // This is wrong because it's not a reference.
  C.udata_getmetatable(
      userdata_typename<CppOwnedType>().c_str() );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.type_of( -1 ) == type::nil );
  C.pop( 2 );

  // Make sure that the metatable has been populated correctly.
  // Try this one this time to make sure it works.
  auto metatable = cast<table>( st["o"][metatable_key] );
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( ut[metatable_key] == metatable );
  REQUIRE( metatable["__name"] == "lua::CppOwnedType&" );
  REQUIRE( ut[metatable_key]["__name"] == "lua::CppOwnedType&" );
  table member_setters =
      cast<table>( metatable["member_setters"] );
  table member_types = cast<table>( metatable["member_types"] );
  table member_getters =
      cast<table>( metatable["member_getters"] );
  REQUIRE( member_types["n"] == false );
  REQUIRE( member_types["d"] == false );
  REQUIRE( member_types["s"] == false );
  REQUIRE( member_types["get_n"] == true );
  REQUIRE( member_types["get_n_plus"] == true );
  REQUIRE( member_types["say"] == true );
  REQUIRE( member_types["flat1"] == true );
  REQUIRE( member_types["flat2"] == true );

  REQUIRE( member_getters["n"].type() == type::function );
  REQUIRE( member_getters["d"].type() == type::function );
  REQUIRE( member_getters["s"].type() == type::function );
  REQUIRE( member_getters["get_n"].type() == type::function );
  REQUIRE( member_getters["get_n_plus"].type() ==
           type::function );
  REQUIRE( member_getters["say"].type() == type::function );
  REQUIRE( member_getters["flat1"].type() == type::function );
  REQUIRE( member_getters["flat2"].type() == type::function );

  REQUIRE( member_setters["n"].type() == type::function );
  REQUIRE( member_setters["d"].type() == type::function );
  REQUIRE( member_setters["s"].type() == type::function );
  REQUIRE( member_setters["get_n"] == nil );
  REQUIRE( member_setters["get_n_plus"] == nil );
  REQUIRE( member_setters["say"] == nil );
  REQUIRE( member_setters["flat1"] == nil );
  REQUIRE( member_setters["flat2"] == nil );

  lua_valid res = st.script.run_safe( R"(
    function assert_eq( l, r )
      if l == r then return end
      error( tostring( l ) .. ' is not equal to ' ..
             tostring( r ) )
    end
    function assert_suffix( l, r )
      local substr = l:sub( #l-#r+1 )
      if substr == r then return end
      error( substr .. ' is not equal to ' ..  r )
    end
    assert_suffix( tostring( o ),
                  'CppOwnedType{n=5,d=9.9,s=hello}' )
    assert_eq( o.a, nil )
    assert_eq( o.n, 5 )
    assert_suffix( tostring( o ),
                  'CppOwnedType{n=5,d=9.9,s=hello}' )
    o.n = o.n + 1
    assert_suffix( tostring( o ),
                  'CppOwnedType{n=6,d=9.9,s=hello}' )
    assert_eq( o.d, 9.9 )
    o.d = 1.2
    assert_eq( o.d, 1.2 )
    assert_suffix( tostring( o ),
                  'CppOwnedType{n=6,d=1.2,s=hello}' )
    assert_eq( type( o.get_n ), 'function' )
    assert_eq( o:get_n(), 6 )
    assert_eq( o:get_n_plus( 4 ), 10 )
    assert_eq( o:say( 'hello' ), 'saying: hello' )
    assert_eq( o:flat1( 4 ), 6+4 )
    assert_eq( o:flat2(), 6+3 )
    assert_eq( o.s, 'hello' )
    o.s = o.s .. o.s
    assert_eq( o.s, 'hellohello' )
    assert_suffix( tostring( o ),
                  'CppOwnedType{n=6,d=1.2,s=hellohello}' )
    assert_eq( o.n_const, 10 )
  )" );
  REQUIRE( res == valid );

  char const* err_non_existent =
      "attempt to set nonexistent field `non_existent'.\n"
      "stack traceback:\n"
      "\t[C]: in metamethod 'newindex'\n"
      "\t[string \"...\"]:5: in main chunk";

  REQUIRE( st.script.run_safe( R"(
    -- This should cover calling non-existent values as well as
    -- setting them.
    assert( o.non_existent == nil )
    o.non_existent = 5
  )" ) == lua_invalid( err_non_existent ) );

  char const* err_const =
      "attempt to set const field `n_const'.\n"
      "stack traceback:\n"
      "\t[C]: in metamethod 'newindex'\n"
      "\t[string \"...\"]:3: in main chunk";

  REQUIRE( st.script.run_safe( R"(
    o.n       = 5 -- ok
    o.n_const = 5 -- boom!
  )" ) == lua_invalid( err_const ) );
}

LUA_TEST_CASE( "[usertype] lua owned" ) {
  C.openlibs();
  usertype<LuaOwnedType> ut( L );
  REQUIRE( C.stack_size() == 0 );

  auto flattened_member_fn_1 = []( LuaOwnedType& o, int n ) {
    return o.n + n;
  };

  ut["n"]          = &LuaOwnedType::n;
  ut["n_const"]    = &LuaOwnedType::n_const;
  ut["d"]          = &LuaOwnedType::d;
  ut["s"]          = &LuaOwnedType::s;
  ut["get_n"]      = &LuaOwnedType::get_n;
  ut["get_n_plus"] = &LuaOwnedType::get_n_plus;
  ut["say"]        = &LuaOwnedType::say;
  ut["flat1"]      = +flattened_member_fn_1;
  ut["flat2"] = [n = 3]( LuaOwnedType& o ) { return o.n + n; };
  REQUIRE( C.stack_size() == 0 );

  st["o"] = LuaOwnedType{};
  REQUIRE( C.stack_size() == 0 );

  REQUIRE( cast<userdata>( st["o"] )[metatable_key]["__name"] ==
           "lua::LuaOwnedType" );
  REQUIRE( cast<userdata>( st["o"] ).name() ==
           "lua::LuaOwnedType" );
  REQUIRE( ut[metatable_key]["__name"] == "lua::LuaOwnedType" );

  // Make sure that the wrong type name is not used.
  lua::push( L, st["o"] );
  // This is wrong because it's a reference.
  C.udata_getmetatable(
      userdata_typename<LuaOwnedType&>().c_str() );
  REQUIRE( C.stack_size() == 2 );
  REQUIRE( C.type_of( -1 ) == type::nil );
  C.pop( 2 );
  REQUIRE( C.stack_size() == 0 );

  // Make sure that the metatable has been populated correctly.
  lua::push( L, st["o"] );
  // Try this one this time to make sure it works.
  C.getmetatable( -1 );
  REQUIRE( C.stack_size() == 2 );
  table metatable( L, C.ref_registry() );
  C.pop();
  REQUIRE( C.stack_size() == 0 );
  REQUIRE( ut[metatable_key] == metatable );
  REQUIRE( ut[metatable_key]["__name"] == metatable["__name"] );
  table member_setters =
      cast<table>( metatable["member_setters"] );
  table member_types = cast<table>( metatable["member_types"] );
  table member_getters =
      cast<table>( metatable["member_getters"] );

  REQUIRE( member_types["n"] == false );
  REQUIRE( member_types["d"] == false );
  REQUIRE( member_types["s"] == false );
  REQUIRE( member_types["get_n"] == true );
  REQUIRE( member_types["get_n_plus"] == true );
  REQUIRE( member_types["say"] == true );
  REQUIRE( member_types["flat1"] == true );
  REQUIRE( member_types["flat2"] == true );

  REQUIRE( member_getters["n"].type() == type::function );
  REQUIRE( member_getters["d"].type() == type::function );
  REQUIRE( member_getters["s"].type() == type::function );
  REQUIRE( member_getters["get_n"].type() == type::function );
  REQUIRE( member_getters["get_n_plus"].type() ==
           type::function );
  REQUIRE( member_getters["say"].type() == type::function );
  REQUIRE( member_getters["flat1"].type() == type::function );
  REQUIRE( member_getters["flat2"].type() == type::function );

  REQUIRE( member_setters["n"].type() == type::function );
  REQUIRE( member_setters["d"].type() == type::function );
  REQUIRE( member_setters["s"].type() == type::function );
  REQUIRE( member_setters["get_n"] == nil );
  REQUIRE( member_setters["get_n_plus"] == nil );
  REQUIRE( member_setters["say"] == nil );
  REQUIRE( member_setters["flat1"] == nil );
  REQUIRE( member_setters["flat2"] == nil );

  lua_valid res = st.script.run_safe( R"(
    function assert_eq( l, r )
      if l == r then return end
      error( tostring( l ) .. ' is not equal to ' ..
             tostring( r ) )
    end
    function assert_suffix( l, r )
      local substr = l:sub( #l-#r+1 )
      if substr == r then return end
      error( substr .. ' is not equal to ' ..  r )
    end
    assert_suffix( tostring( o ),
                  'LuaOwnedType{n=5,d=9.9,s=hello}' )
    assert_eq( o.a, nil )
    assert_eq( o.n, 5 )
    assert_suffix( tostring( o ),
                  'LuaOwnedType{n=5,d=9.9,s=hello}' )
    o.n = o.n + 1
    assert_suffix( tostring( o ),
                  'LuaOwnedType{n=6,d=9.9,s=hello}' )
    assert_eq( o.d, 9.9 )
    o.d = 1.2
    assert_eq( o.d, 1.2 )
    assert_suffix( tostring( o ),
                  'LuaOwnedType{n=6,d=1.2,s=hello}' )
    assert_eq( type( o.get_n ), 'function' )
    assert_eq( o:get_n(), 6 )
    assert_eq( o:get_n_plus( 4 ), 10 )
    assert_eq( o:say( 'hello' ), 'saying: hello' )
    assert_eq( o:flat1( 4 ), 6+4 )
    assert_eq( o:flat2(), 6+3 )
    assert_eq( o.s, 'hello' )
    o.s = o.s .. o.s
    assert_eq( o.s, 'hellohello' )
    assert_suffix( tostring( o ),
                  'LuaOwnedType{n=6,d=1.2,s=hellohello}' )
    assert_eq( o.n_const, 10 )
  )" );
  REQUIRE( res == valid );

  char const* err_non_existent =
      "attempt to set nonexistent field `non_existent'.\n"
      "stack traceback:\n"
      "\t[C]: in metamethod 'newindex'\n"
      "\t[string \"...\"]:5: in main chunk";

  REQUIRE( st.script.run_safe( R"(
    -- This should cover calling non-existent values as well as
    -- setting them.
    assert( o.non_existent == nil )
    o.non_existent = 5
  )" ) == lua_invalid( err_non_existent ) );

  char const* err_const =
      "attempt to set const field `n_const'.\n"
      "stack traceback:\n"
      "\t[C]: in metamethod 'newindex'\n"
      "\t[string \"...\"]:3: in main chunk";

  REQUIRE( st.script.run_safe( R"(
    o.n       = 5 -- ok
    o.n_const = 5 -- boom!
  )" ) == lua_invalid( err_const ) );
}

LUA_TEST_CASE( "[usertype] lua owned constructor" ) {
  C.openlibs();
  usertype<LuaOwnedType> ut( L );
  REQUIRE( C.stack_size() == 0 );

  ut["n"]          = &LuaOwnedType::n;
  ut["n_const"]    = &LuaOwnedType::n_const;
  ut["d"]          = &LuaOwnedType::d;
  ut["s"]          = &LuaOwnedType::s;
  ut["get_n"]      = &LuaOwnedType::get_n;
  ut["get_n_plus"] = &LuaOwnedType::get_n_plus;
  ut["say"]        = &LuaOwnedType::say;
  REQUIRE( C.stack_size() == 0 );

  st["LuaOwned"] = []( int n ) {
    LuaOwnedType lo;
    lo.n = n;
    return lo;
  };

  int res = st.script.run<int>( R"(
    lo = LuaOwned( 7 )
    return lo.n + lo.n_const
  )" );
  REQUIRE( res == 7 + 10 );

  LuaOwnedType& lo = cast<LuaOwnedType&>( st["lo"] );
  REQUIRE( lo.n == 7 );

  st.script.run( R"(
    lo.d = 11
    lo.n = 21
  )" );

  REQUIRE( lo.d == 11.0 );
  REQUIRE( lo.n == 21 );
}

LUA_TEST_CASE(
    "[usertype] pass cpp type by value in member functions" ) {
  C.openlibs();
  using U = HasMemberFunctionsWithCppTypesAsArgs;
  usertype<U> ut( L );
  REQUIRE( C.stack_size() == 0 );

  ut["lua_owned"]           = &U::lua_owned;
  ut["cpp_owned"]           = &U::cpp_owned;
  ut["lua_owned_ref"]       = &U::lua_owned_ref;
  ut["cpp_owned_ref"]       = &U::cpp_owned_ref;
  ut["lua_owned_const_ref"] = &U::lua_owned_const_ref;
  ut["cpp_owned_const_ref"] = &U::cpp_owned_const_ref;

  ut["lua_owned_r"]     = &U::lua_owned_r;
  ut["cpp_owned_ref_r"] = &U::cpp_owned_ref_r;
}

} // namespace
} // namespace lua
