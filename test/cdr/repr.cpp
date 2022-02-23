/****************************************************************
**repr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-17.
*
* Description: Unit tests for the src/cdr/repr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/repr.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;

static_assert( std::ranges::range<table> );
static_assert( std::ranges::range<list> );

TEST_CASE( "[cdr] value" ) {
  using namespace ::cdr::literals;

  value v;
  REQUIRE( v.is<null_t>() );
  REQUIRE( type_name( v ) == "null" );

  v = null;
  REQUIRE( v.is<null_t>() );
  REQUIRE( type_name( v ) == "null" );

  v = 5.5;
  REQUIRE( v.is<double>() );
  REQUIRE( type_name( v ) == "floating" );

  v = 5.5_val;
  REQUIRE( v.is<double>() );
  REQUIRE( type_name( v ) == "floating" );

  v = 5;
  REQUIRE( v.is<integer_type>() );
  REQUIRE( type_name( v ) == "integer" );

  value v2{ 5 };
  REQUIRE( v2.is<integer_type>() );
  REQUIRE( type_name( v ) == "integer" );

  v = 5_val;
  REQUIRE( v.is<integer_type>() );
  REQUIRE( type_name( v ) == "integer" );

  v = true;
  REQUIRE( v.is<bool>() );
  REQUIRE( type_name( v ) == "boolean" );

  v = false;
  REQUIRE( v.is<bool>() );
  REQUIRE( type_name( v ) == "boolean" );

  v = "hello"s;
  REQUIRE( v.is<string>() );
  REQUIRE( type_name( v ) == "string" );

  v = table{};
  REQUIRE( v.is<table>() );
  REQUIRE( type_name( v ) == "table" );
  REQUIRE( v.as<table>().size() == 0 );
  REQUIRE( v.as<table>().ssize() == 0 );

  table from_il{ { "one", 1_val }, { "two", 2 } };

  v = std::move( from_il );
  REQUIRE( v.is<table>() );
  REQUIRE( v.as<table>().size() == 2 );
  REQUIRE( v.as<table>().ssize() == 2 );
  REQUIRE( v.as<table>()["one"] == 1_val );
  REQUIRE( v.as<table>()["two"] == 2_val );
  REQUIRE( v.as<table>().contains( "one" ) );
  REQUIRE( v.as<table>().contains( "two" ) );
  REQUIRE( !v.as<table>().contains( "three" ) );

  v = table{
      { "one", 1_val },
      { "two", 2_val },
  };
  REQUIRE( v.is<table>() );
  REQUIRE( type_name( v ) == "table" );
  REQUIRE( v.as<table>().size() == 2 );
  REQUIRE( v.as<table>().ssize() == 2 );
  table const& t = v.as<table>();
  REQUIRE( t["one"] == 1_val );
  REQUIRE( t["two"] == 2_val );
  REQUIRE( t["three"] == base::nothing );
  REQUIRE( v.as<table>().contains( "one" ) );
  REQUIRE( v.as<table>().contains( "two" ) );
  REQUIRE( !v.as<table>().contains( "three" ) );
  table& t2 = v.as<table>();
  // The non-const version will create a non-existent entry.
  REQUIRE( t2["three"] == null );
  // Note that `null` is not the same as base::nothing.
  REQUIRE( t["three"] == null );
  REQUIRE( v.as<table>().contains( "one" ) );
  REQUIRE( v.as<table>().contains( "two" ) );
  REQUIRE( v.as<table>().contains( "three" ) );

  v = list{};
  REQUIRE( v.is<list>() );
  REQUIRE( type_name( v ) == "list" );
  REQUIRE( v.as<list>().size() == 0 );
  REQUIRE( v.as<list>().ssize() == 0 );

  list from_il2{ 4_val, "one" };

  v = from_il2;
  REQUIRE( v.is<list>() );
  REQUIRE( type_name( v ) == "list" );
  REQUIRE( v.as<list>().size() == 2 );
  REQUIRE( v.as<list>().ssize() == 2 );
  REQUIRE( v.as<list>()[0] == 4_val );
  REQUIRE( v.as<list>()[1] == "one" );

  v = list{ 1_val, 2_val, 3_val, 4_val };
  REQUIRE( v.is<list>() );
  REQUIRE( type_name( v ) == "list" );
  REQUIRE( v.as<list>().size() == 4 );
  REQUIRE( v.as<list>().ssize() == 4 );
  list& l = v.as<list>();
  REQUIRE( l[0] == 1_val );
  REQUIRE( l[1] == 2_val );
  REQUIRE( l[2] == 3_val );
  REQUIRE( l[3] == 4_val );

  l.push_back( 2 );
  l.emplace_back( 2.3 );
  REQUIRE( v.as<list>().size() == 6 );
  REQUIRE( v.as<list>().ssize() == 6 );
  REQUIRE( l[0] == 1_val );
  REQUIRE( l[1] == 2_val );
  REQUIRE( l[2] == 3_val );
  REQUIRE( l[3] == 4_val );
  REQUIRE( l[4] == 2_val );
  REQUIRE( l[5] == 2.3_val );
}

TEST_CASE( "[cdr] complex" ) {
  using namespace ::cdr::literals;

  table doc{
      { "one", list{ 2, 3, "hello" } },
      { "two",
        table{
            { "three", 3.3 },
            { "four", true },
        } },
      { "three",
        list{
            table{
                { "hello", "world" },
                { "yes", 333 },
            },
            table{},
            3,
        } },
  };

  REQUIRE( doc["three"].is<list>() );
  REQUIRE( doc["three"].as<list>()[0].is<table>() );
  REQUIRE( doc["three"]
               .as<list>()[0]
               .as<table>()["yes"]
               .is<integer_type>() );
  REQUIRE( doc["three"].as<list>()[0].as<table>()["yes"] ==
           333 );

  REQUIRE( doc["one"].is<list>() );
  REQUIRE( doc["one"].as<list>()[1] == 3 );
  REQUIRE( doc["one"].as<list>()[2].is<string>() );
  REQUIRE( doc["one"].as<list>()[2].as<string>() == "hello" );

  REQUIRE( doc["two"].is<table>() );
  REQUIRE( doc["two"].as<table>()["four"].is<bool>() );
  REQUIRE( doc["two"].as<table>()["four"] == true );

  table t2 = doc;

  REQUIRE( t2["one"].is<list>() );
  REQUIRE( t2["one"].as<list>()[1] == 3 );

  REQUIRE( t2["two"].is<table>() );
  REQUIRE( t2["two"].as<table>()["four"] == true );

  // Make sure that the deep copy happened.
  REQUIRE( &doc["two"].as<table>()["four"] ==
           &doc["two"].as<table>()["four"] );

  REQUIRE( &doc["two"].as<table>()["four"] !=
           &t2["two"].as<table>()["four"] );

  value* address = &t2["two"].as<table>()["four"];

  table t3 = std::move( t2 );
  // Ensure that a move happened.
  REQUIRE( &t3["two"].as<table>()["four"] == address );
}

TEST_CASE( "[cdr] k=v syntax" ) {
  using namespace ::cdr::literals;

  table doc{
      "one"_key = list{ 2, 3, "hello" },
      "two"_key =
          table{
              "three"_key = 3.3,
              "four"_key  = true,
          },
      "three"_key =
          list{
              table{
                  "hello"_key = "world",
                  "yes"_key   = 333,
              },
              table{},
              3,
          },
  };

  REQUIRE( doc["three"].is<list>() );
  REQUIRE( doc["three"].as<list>()[0].is<table>() );
  REQUIRE( doc["three"]
               .as<list>()[0]
               .as<table>()["yes"]
               .is<integer_type>() );
  REQUIRE( doc["three"].as<list>()[0].as<table>()["yes"] ==
           333 );

  REQUIRE( doc["one"].is<list>() );
  REQUIRE( doc["one"].as<list>()[1] == 3 );
  REQUIRE( doc["one"].as<list>()[2].is<string>() );
  REQUIRE( doc["one"].as<list>()[2].as<string>() == "hello" );

  REQUIRE( doc["two"].is<table>() );
  REQUIRE( doc["two"].as<table>()["four"].is<bool>() );
  REQUIRE( doc["two"].as<table>()["four"] == true );

  table t2 = doc;

  REQUIRE( t2["one"].is<list>() );
  REQUIRE( t2["one"].as<list>()[1] == 3 );

  REQUIRE( t2["two"].is<table>() );
  REQUIRE( t2["two"].as<table>()["four"] == true );
}

TEST_CASE( "[cdr] to_str" ) {
  value v;

  // null
  v = null;
  REQUIRE( base::to_str( v ) == "null" );

  // double
  v = 5.5;
  REQUIRE( base::to_str( v ) == "5.5" );

  // integer_type
  v = 5;
  REQUIRE( base::to_str( v ) == "5" );

  // bool
  v = true;
  REQUIRE( base::to_str( v ) == "true" );
  v = false;
  REQUIRE( base::to_str( v ) == "false" );

  // string
  v = "hello";
  REQUIRE( base::to_str( v ) == "hello" );

  // table
  v = table{};
  REQUIRE( base::to_str( v ) == "{}" );
  v = table{ { "one", 1 } };
  REQUIRE( base::to_str( v ) == "{one=1}" );
  v = table{ { "one", 1 }, { "two", 2 } };
  REQUIRE( base::to_str( v ) == "{one=1,two=2}" );
  v = table{ { "one", 1 }, { "two", 2 }, { "three", "3" } };
  REQUIRE( base::to_str( v ) == "{one=1,three=3,two=2}" );

  // list
  v = list{};
  REQUIRE( base::to_str( v ) == "[]" );
  v = list{ 5 };
  REQUIRE( base::to_str( v ) == "[5]" );
  v = list{ 5, "hello" };
  REQUIRE( base::to_str( v ) == "[5,hello]" );
  v = list{ 5, "hello", 4.4 };
  REQUIRE( base::to_str( v ) == "[5,hello,4.4]" );

  table doc{
      { "one", list{ 2, 3, "hello" } },
      { "two",
        table{
            { "three", 3.3 },
            { "four", true },
        } },
      { "three",
        list{
            table{
                { "hello", "world" },
                { "yes", 333 },
            },
            table{},
            3,
        } },
  };
  REQUIRE( base::to_str( doc ) ==
           "{one=[2,3,hello],three=[{hello=world,yes=333},{},3],"
           "two={four=true,three=3.3}}" );
}

} // namespace
} // namespace cdr
