/****************************************************************
**model.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Unit tests for the src/model/model.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/model/model.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace model {
namespace {

using namespace std;

TEST_CASE( "[model] value" ) {
  using namespace ::model::literals;

  value v;
  REQUIRE( v.is<null_t>() );

  v = null;
  REQUIRE( v.is<null_t>() );

  v = 5.5;
  REQUIRE( v.is<double>() );

  v = 5.5_val;
  REQUIRE( v.is<double>() );

  v = 5;
  REQUIRE( v.is<int>() );

  v = 5_val;
  REQUIRE( v.is<int>() );

  v = true;
  REQUIRE( v.is<bool>() );

  v = false;
  REQUIRE( v.is<bool>() );

  v = "hello"s;
  REQUIRE( v.is<string>() );

  v = table{};
  REQUIRE( v.is<table>() );
  REQUIRE( v.as<table>().size() == 0 );
  REQUIRE( v.as<table>().ssize() == 0 );

  v = unordered_map<string, value>{
      { "one", 1_val },
      { "two", 2_val },
  };
  REQUIRE( v.is<table>() );
  REQUIRE( v.as<table>().size() == 2 );
  REQUIRE( v.as<table>().ssize() == 2 );
  table const& t = v.as<table>();
  REQUIRE( t["one"] == 1_val );
  REQUIRE( t["two"] == 2_val );
  REQUIRE( t["three"] == base::nothing );
  table& t2 = v.as<table>();
  // The non-const version will create a non-existent entry.
  REQUIRE( t2["three"] == null );
  REQUIRE( t["three"] == null );

  v = list{};
  REQUIRE( v.is<list>() );
  REQUIRE( v.as<list>().size() == 0 );
  REQUIRE( v.as<list>().ssize() == 0 );

  v = vector<value>{ 1_val, 2_val, 3_val, 4_val };
  REQUIRE( v.is<list>() );
  REQUIRE( v.as<list>().size() == 4 );
  REQUIRE( v.as<list>().ssize() == 4 );
  list const& l = v.as<list>();
  REQUIRE( l[0] == 1_val );
  REQUIRE( l[1] == 2_val );
  REQUIRE( l[2] == 3_val );
  REQUIRE( l[3] == 4_val );
}

TEST_CASE( "[model] complex" ) {
  using namespace ::model::literals;

  table t = unordered_map<string, value>{
      { "one",
        vector<value>{ 2_val, 3_val, value( "hello"s ) } },
      { "two",
        unordered_map<string, value>{
            { "three", 3.3_val },
            { "four", 4_val },
        } },
  };

  REQUIRE( t["one"].is<list>() );
  REQUIRE( t["one"].as<list>()[1] == 3 );

  REQUIRE( t["two"].is<table>() );
  REQUIRE( t["two"].as<table>()["four"] == 4_val );

  table t2 = t;

  REQUIRE( t2["one"].is<list>() );
  REQUIRE( t2["one"].as<list>()[1] == 3 );

  REQUIRE( t2["two"].is<table>() );
  REQUIRE( t2["two"].as<table>()["four"] == 4_val );

  // Make sure that the deep copy happened.
  REQUIRE( &t["two"].as<table>()["four"] ==
           &t["two"].as<table>()["four"] );

  REQUIRE( &t["two"].as<table>()["four"] !=
           &t2["two"].as<table>()["four"] );

  value* address = &t2["two"].as<table>()["four"];

  table t3 = std::move( t2 );
  REQUIRE( &t3["two"].as<table>()["four"] == address );
}

} // namespace
} // namespace model
