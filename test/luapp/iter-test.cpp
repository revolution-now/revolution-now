/****************************************************************
**iter.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-06-21.
*
* Description: Unit tests for the src/luapp/iter.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/luapp/iter.hpp"

// Testing
#include "test/luapp/common.hpp"

// luapp
#include "src/luapp/as.hpp"

// base
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace lua {
namespace {

using namespace std;

using ::Catch::UnorderedEquals;

LUA_TEST_CASE( "[iter] raw_table_iterator|empty" ) {
  st["tbl"] = st.table.create();

  vector<pair<string, int>> pairs;

  table tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    pairs.push_back( { as<string>( k ), as<int>( v ) } );

  vector<pair<string, int>> expected{};

  REQUIRE_THAT( pairs, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] raw_table_iterator|single" ) {
  st["tbl"]          = st.table.create();
  st["tbl"]["hello"] = 5;

  vector<pair<string, int>> pairs;

  table tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    pairs.push_back( { as<string>( k ), as<int>( v ) } );

  vector<pair<string, int>> expected{ { "hello", 5 } };

  REQUIRE_THAT( pairs, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] raw_table_iterator|iterate" ) {
  st["tbl"]            = st.table.create();
  st["tbl"]["hello"]   = 5;
  st["tbl"]["world"]   = 6;
  st["tbl"]["the_end"] = 7;

  vector<pair<string, int>> pairs;

  table tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    pairs.push_back( { as<string>( k ), as<int>( v ) } );

  vector<pair<string, int>> expected{
    { "hello", 5 }, { "world", 6 }, { "the_end", 7 } };

  REQUIRE_THAT( pairs, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE(
    "[iter] raw_table_iterator|iterate with numbers" ) {
  st["tbl"]          = st.table.create();
  st["tbl"][5]       = 5;
  st["tbl"]["world"] = 6;
  st["tbl"][1]       = 7;
  st["tbl"][2]       = 9;

  vector<pair<string, int>> pairs;

  table tbl = as<table>( st["tbl"] );

  // To test iteration we must use these maps because the itera-
  // tion order of keys when using lua_next is not specified,
  // even for numeric indices. (To iterate through the indices of
  // an array, use a numerical-for loop, or C equivalent).
  unordered_map<int, int> int_keys{
    { 5, 5 }, { 1, 7 }, { 2, 9 } };
  unordered_map<string, int> str_keys{ { "world", 6 } };

  auto it = begin( tbl );

  while( true ) {
    if( int_keys.empty() && str_keys.empty() ) break;
    // We have more that we're expecting.
    INFO( fmt::format( "int_keys: {}", int_keys ) );
    INFO( fmt::format( "str_keys: {}", str_keys ) );
    REQUIRE( it != end( tbl ) );
    type key_type = type_of( it->first );
    int val       = as<int>( it->second );
    switch( key_type ) {
      case type::number: {
        int key = as<int>( it->first );
        INFO( fmt::format( "key: {}", key ) );
        REQUIRE( int_keys.contains( key ) );
        REQUIRE( int_keys[key] == val );
        int_keys.erase( key );
        break;
      }
      case type::string: {
        string key = as<string>( it->first );
        INFO( fmt::format( "key: {}", key ) );
        REQUIRE( str_keys.contains( key ) );
        REQUIRE( str_keys[key] == val );
        str_keys.erase( key );
        break;
      }
      default: {
        INFO( fmt::format( "unexpected type: {}", key_type ) );
        REQUIRE( false );
      }
    }
    it++;
  }

  REQUIRE( it == end( tbl ) );
}

LUA_TEST_CASE( "[iter] raw_table_iterator|has __pairs" ) {
  st.lib.open_all();

  string const script = R"lua(
    tbl = {}
    local mt = {
      __pairs=function( tbl )
        local inside = { x=1, y=2, z=3 }
        return next, inside, nil
      end
    }
    setmetatable( tbl, mt )
  )lua";
  st.script.run( script );

  vector<pair<string, int>> res;

  table const tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    res.push_back( { as<string>( k ), as<int>( v ) } );

  // Got nothing.
  vector<pair<string, int>> expected;

  REQUIRE_THAT( res, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] lua_iterator|iterate no __pairs" ) {
  st.lib.open_all(); // need pairs method.

  st["tbl"]            = st.table.create();
  st["tbl"]["hello"]   = 5;
  st["tbl"]["world"]   = 6;
  st["tbl"]["the_end"] = 7;

  vector<pair<string, int>> res;

  table tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : pairs( tbl ) )
    res.push_back( { as<string>( k ), as<int>( v ) } );

  vector<pair<string, int>> expected{
    { "hello", 5 }, { "world", 6 }, { "the_end", 7 } };

  REQUIRE_THAT( res, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] lua_iterator|has __pairs, empty" ) {
  st.lib.open_all();

  string const script = R"lua(
    tbl = {}
    local mt = {
      __pairs=function( tbl )
        local inside = {}
        return next, inside, nil
      end
    }
    setmetatable( tbl, mt )
  )lua";
  st.script.run( script );

  vector<pair<string, int>> res;

  table const tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : pairs( tbl ) )
    res.push_back( { as<string>( k ), as<int>( v ) } );

  vector<pair<string, int>> expected;

  REQUIRE_THAT( res, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] lua_iterator|has __pairs" ) {
  st.lib.open_all();

  string const script = R"lua(
    tbl = {}
    local mt = {
      __pairs=function( tbl )
        local inside = { x=1, hello=2, z=3 }
        return next, inside, nil
      end
    }
    setmetatable( tbl, mt )
  )lua";
  st.script.run( script );

  vector<pair<string, int>> res;

  table const tbl = as<table>( st["tbl"] );

  for( auto& [k, v] : pairs( tbl ) )
    res.push_back( { as<string>( k ), as<int>( v ) } );

  vector<pair<string, int>> expected{
    { "x", 1 }, { "hello", 2 }, { "z", 3 } };

  REQUIRE_THAT( res, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] lua_iterator|_G two ways" ) {
  st.lib.open_all();

  vector<string> v1;
  for( auto& [k, v] : st.table.global() )
    v1.push_back( k.as<string>() );

  vector<string> v2;
  for( auto& [k, v] : pairs( st.table.global() ) )
    v2.push_back( k.as<string>() );

  REQUIRE_THAT( v1, Catch::UnorderedEquals( v2 ) );
}

} // namespace
} // namespace lua
