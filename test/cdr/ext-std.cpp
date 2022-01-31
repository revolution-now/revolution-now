/****************************************************************
**ext-std.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-01-28.
*
* Description: Unit tests for the src/cdr/ext-std.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/cdr/ext-std.hpp"

// cdr
#include "src/cdr/ext-builtin.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;

converter conv( "test" );

TEST_CASE( "[cdr/ext-std] string" ) {
  SECTION( "to_canonical" ) {
    REQUIRE( to_canonical( "hello"s ) == string( "hello" ) );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv.from<string>( "hello" ) == "hello" );
    REQUIRE( conv.from<string>( 5 ) ==
             error( "producing a std::string requires type "
                    "string, instead found type integer." ) );
  }
}

TEST_CASE( "[cdr/ext-std] std::filesystem::path" ) {
  SECTION( "to_canonical" ) {
    REQUIRE( to_canonical( fs::path( "hello" ) ) ==
             string( "hello" ) );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv.from<fs::path>( "hello" ) == "hello" );
    REQUIRE(
        conv.from<fs::path>( 5 ) ==
        error( "producing a std::filesystem::path requires type "
               "string, instead found type integer." ) );
  }
}

TEST_CASE( "[cdr/ext-std] chrono::seconds" ) {
  SECTION( "to_canonical" ) {
    REQUIRE( to_canonical( chrono::seconds{ 5 } ) == 5 );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv.from<chrono::seconds>( 5 ) ==
             chrono::seconds{ 5 } );
    REQUIRE(
        conv.from<chrono::seconds>( "5" ) ==
        error( "producing a std::chrono::seconds requires type "
               "integer, instead found type string." ) );
  }
}

TEST_CASE( "[cdr/ext-std] pair" ) {
  SECTION( "to_canonical" ) {
    pair<int, bool> p{ 5, true };
    REQUIRE( to_canonical( p ) ==
             table{ { "key", 5 }, { "val", true } } );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv.from<pair<int, bool>>(
                 table{ { "key", 5 }, { "val", true } } ) ==
             pair<int, bool>{ 5, true } );
    REQUIRE( conv.from<pair<int, bool>>(
                 table{ { "fxt", 5 }, { "val", true } } ) ==
             error( "table must have both a 'key' and 'val' "
                    "field for conversion to std::pair." ) );
    REQUIRE( conv.from<pair<int, bool>>( 5 ) ==
             error( "producing a std::pair requires type "
                    "table, instead found type integer." ) );
  }
}

TEST_CASE( "[cdr/ext-std] vector" ) {
  SECTION( "to_canonoical" ) {
    vector<int> empty;
    REQUIRE( to_canonical( empty ) == list{} );
    vector<int> vec{ 3, 4, 5 };
    REQUIRE( to_canonical( vec ) == list{ 3, 4, 5 } );
  }
  SECTION( "from_canonoical" ) {
    REQUIRE( conv.from<vector<double>>( list{ 5.5, 7.7 } ) ==
             vector<double>{ 5.5, 7.7 } );
    REQUIRE( conv.from<vector<double>>( table{} ) ==
             error( "producing a std::vector requires type "
                    "list, instead found type table." ) );
    REQUIRE( conv.from<vector<double>>( list{ true } ) ==
             error( "failed to convert cdr value of type "
                    "boolean to double." ) );
  }
}

TEST_CASE( "[cdr/ext-std] array" ) {
  SECTION( "to_canonoical" ) {
    array<int, 0> empty;
    REQUIRE( to_canonical( empty ) == list{} );
    array<int, 3> arr{ 3, 4, 5 };
    REQUIRE( to_canonical( arr ) == list{ 3, 4, 5 } );
  }
  SECTION( "from_canonoical" ) {
    REQUIRE( conv.from<array<int, 2>>( list{ 5, 7 } ) ==
             array<int, 2>{ 5, 7 } );
    REQUIRE(
        conv.from<array<int, 2>>( list{ 5 } ) ==
        error(
            "expected list of size 2 for producing std::array "
            "of that same size, instead found size 1." ) );
    REQUIRE( conv.from<array<int, 2>>( 5.5 ) ==
             error( "producing a std::array requires type list, "
                    "instead found type floating." ) );
    REQUIRE(
        conv.from<array<int, 2>>( list{ true, false } ) ==
        error( "failed to convert cdr value of type boolean to "
               "int." ) );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_map (list)" ) {
  SECTION( "to_canonoical" ) {
    unordered_map<int, double> empty;
    REQUIRE( to_canonical( empty ) == list{} );
    unordered_map<int, double> m1{ { 3, 5.5 }, { 4, 7.7 } };
    value                      v1 = to_canonical( m1 );
    REQUIRE(
        ( ( v1 ==
            list{ table{ { "key", 3 }, { "val", 5.5 } },
                  table{ { "key", 4 }, { "val", 7.7 } } } ) ||
          ( v1 ==
            list{ table{ { "key", 4 }, { "val", 7.7 } },
                  table{ { "key", 3 }, { "val", 5.5 } } } ) ) );
  }
  SECTION( "from_canonoical" ) {
    // Here we can convert from either a table or a list, but
    // here we are testing the list version.
    using M = unordered_map<string, int>;
    M     expected{ { "one", 1 }, { "two", 2 } };
    value v = list{ table{ { "key", "one" }, { "val", 1 } },
                    table{ { "key", "two" }, { "val", 2 } } };
    REQUIRE( conv.from<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_map (table)" ) {
  SECTION( "to_canonoical" ) {
    // Here we have string keys, so it should convert to a Cdr
    // table.
    unordered_map<string, double> empty;
    REQUIRE( to_canonical( empty ) == table{} );
    unordered_map<string, double> m1{ { "3", 5.5 },
                                      { "4", 7.7 } };

    value v1 = to_canonical( m1 );
    REQUIRE( v1 == table{ { "3", 5.5 }, { "4", 7.7 } } );
    unordered_map<string_view, double> m2{ { "3", 5.5 },
                                           { "4", 7.7 } };

    value v2 = to_canonical( m2 );
    REQUIRE( v2 == table{ { "3", 5.5 }, { "4", 7.7 } } );
  }
  SECTION( "from_canonoical" ) {
    // Here we can convert from either a table or a list, but
    // here we are testing the table version.
    using M = unordered_map<string, int>;
    M     expected{ { "one", 1 }, { "two", 2 } };
    value v = table{ { "one", 1 }, { "two", 2 } };
    REQUIRE( conv.from<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_set" ) {
  SECTION( "to_canonoical" ) {
    unordered_set<string> empty;
    REQUIRE( to_canonical( empty ) == list{} );
    unordered_set<string> m1{ "hello", "world" };
    value                 v1 = to_canonical( m1 );
    REQUIRE( ( ( v1 == list{ "hello", "world" } ) ||
               ( v1 == list{ "world", "hello" } ) ) );
  }
  SECTION( "from_canonoical" ) {
    using M = unordered_set<int>;
    M     expected{ 1, 2, 3 };
    value v = list{ 2, 3, 3, 1, 2, 1, 1 };
    REQUIRE( conv.from<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] unique_ptr" ) {
  SECTION( "to_canonoical" ) {
    unique_ptr<string> empty;
    REQUIRE( to_canonical( empty ) == null );
    auto  u = make_unique<string>( "hello" );
    value v = to_canonical( u );
    REQUIRE( v == "hello" );
  }
  SECTION( "from_canonoical" ) {
    REQUIRE( conv.from<unique_ptr<string>>( null ) ==
             unique_ptr<string>( nullptr ) );
    value                      v = "hello";
    result<unique_ptr<string>> res =
        conv.from<unique_ptr<string>>( v );
    REQUIRE( res.has_value() );
    REQUIRE( *res != nullptr );
    REQUIRE( **res == "hello" );
  }
}

} // namespace
} // namespace cdr
