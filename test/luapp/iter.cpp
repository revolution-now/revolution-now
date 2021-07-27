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
#include "src/luapp/cast.hpp"

// Must be last.
#include "test/catch-common.hpp"

FMT_TO_CATCH( ::lua::type );

namespace lua {
namespace {

using namespace std;

using ::Catch::UnorderedEquals;

LUA_TEST_CASE( "[iter] empty" ) {
  st["tbl"] = st.table.create();

  vector<pair<string, int>> pairs;

  table tbl = cast<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    pairs.push_back( { cast<string>( k ), cast<int>( v ) } );

  vector<pair<string, int>> expected{};

  REQUIRE_THAT( pairs, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] single" ) {
  st["tbl"]          = st.table.create();
  st["tbl"]["hello"] = 5;

  vector<pair<string, int>> pairs;

  table tbl = cast<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    pairs.push_back( { cast<string>( k ), cast<int>( v ) } );

  vector<pair<string, int>> expected{ { "hello", 5 } };

  REQUIRE_THAT( pairs, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] iterate" ) {
  st["tbl"]            = st.table.create();
  st["tbl"]["hello"]   = 5;
  st["tbl"]["world"]   = 6;
  st["tbl"]["the_end"] = 7;

  vector<pair<string, int>> pairs;

  table tbl = cast<table>( st["tbl"] );

  for( auto& [k, v] : tbl )
    pairs.push_back( { cast<string>( k ), cast<int>( v ) } );

  vector<pair<string, int>> expected{
      { "hello", 5 }, { "world", 6 }, { "the_end", 7 } };

  REQUIRE_THAT( pairs, Catch::UnorderedEquals( expected ) );
}

LUA_TEST_CASE( "[iter] iterate with numbers" ) {
  st["tbl"]          = st.table.create();
  st["tbl"][5]       = 5;
  st["tbl"]["world"] = 6;
  st["tbl"][1]       = 7;
  st["tbl"][2]       = 9;

  vector<pair<string, int>> pairs;

  table tbl = cast<table>( st["tbl"] );

  auto it = begin( tbl );

  REQUIRE( it != end( tbl ) );
  REQUIRE( it->first == 1 );
  REQUIRE( it->second == 7 );
  ++it;

  REQUIRE( it != end( tbl ) );
  REQUIRE( it->first == 2 );
  REQUIRE( it->second == 9 );
  ++it;

  REQUIRE( it != end( tbl ) );
  REQUIRE( it->first == "world" );
  REQUIRE( it->second == 6 );
  ++it;

  REQUIRE( it != end( tbl ) );
  REQUIRE( it->first == 5 );
  REQUIRE( it->second == 5 );
  ++it;

  REQUIRE( it == end( tbl ) );
}

} // namespace
} // namespace lua
