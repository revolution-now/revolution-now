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

// Must be last.
#include "test/catch-common.hpp"

namespace cdr {
namespace {

using namespace std;

TEST_CASE( "[cdr/ext-std] pair" ) {
  SECTION( "to_canonical" ) {
    pair<int, bool> p{ 5, true };
    REQUIRE( to_canonical( p ) ==
             table{ { "fst", 5 }, { "snd", true } } );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( from_canonical<pair<int, bool>>(
                 table{ { "fst", 5 }, { "snd", true } } ) ==
             pair<int, bool>{ 5, true } );
    REQUIRE( from_canonical<pair<int, bool>>(
                 table{ { "fxt", 5 }, { "snd", true } } ) ==
             error::build{ "test" }(
                 "table must have both a 'fst' and 'snd' "
                 "field for conversion to std::pair." ) );
    REQUIRE( from_canonical<pair<int, bool>>( 5 ) ==
             error::build{ "test" }(
                 "producing a std::pair requires type "
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
    REQUIRE( from_canonical<vector<double>>( list{
                 5.5, 7.7 } ) == vector<double>{ 5.5, 7.7 } );
    REQUIRE( from_canonical<vector<double>>( table{} ) ==
             error::build{ "test" }(
                 "producing a std::vector requires type "
                 "list, instead found type table." ) );
    REQUIRE( from_canonical<vector<double>>( list{ true } ) ==
             error::build{ "test" }(
                 "failed to convert cdr value of type "
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
    REQUIRE( from_canonical<array<int, 2>>( list{ 5, 7 } ) ==
             array<int, 2>{ 5, 7 } );
    REQUIRE(
        from_canonical<array<int, 2>>( list{ 5 } ) ==
        error::build{ "test" }(
            "expected list of size 2 for producing std::array "
            "of that same size, instead found size 1." ) );
    REQUIRE( from_canonical<array<int, 2>>( 5.5 ) ==
             error::build{ "test" }(
                 "producing a std::array requires type list, "
                 "instead found type floating." ) );
    REQUIRE(
        from_canonical<array<int, 2>>( list{ true, false } ) ==
        error::build{ "test" }(
            "failed to convert cdr value of type boolean to "
            "int." ) );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_map" ) {
  SECTION( "to_canonoical" ) {
    unordered_map<int, double> empty;
    REQUIRE( to_canonical( empty ) == list{} );
    unordered_map<int, double> m1{ { 3, 5.5 }, { 4, 7.7 } };
    value                      v1 = to_canonical( m1 );
    REQUIRE(
        ( ( v1 ==
            list{ table{ { "fst", 3 }, { "snd", 5.5 } },
                  table{ { "fst", 4 }, { "snd", 7.7 } } } ) ||
          ( v1 ==
            list{ table{ { "fst", 4 }, { "snd", 7.7 } },
                  table{ { "fst", 3 }, { "snd", 5.5 } } } ) ) );
  }
  SECTION( "from_canonoical" ) {
    using M = unordered_map<string, int>;
    M     expected{ { "one", 1 }, { "two", 2 } };
    value v = list{ table{ { "fst", "one" }, { "snd", 1 } },
                    table{ { "fst", "two" }, { "snd", 2 } } };
    REQUIRE( from_canonical<M>( v ) == expected );
  }
}

} // namespace
} // namespace cdr
