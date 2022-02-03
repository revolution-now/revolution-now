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

using ::cdr::testing::conv_from_bt;

converter conv;

TEST_CASE( "[cdr/ext-std] string" ) {
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( "hello"s ) == string( "hello" ) );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<string>( "hello" ) == "hello" );
    REQUIRE( conv.from<string>( 5 ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
  }
}

TEST_CASE( "[cdr/ext-std] std::filesystem::path" ) {
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( fs::path( "hello" ) ) ==
             string( "hello" ) );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<fs::path>( "hello" ) == "hello" );
    REQUIRE( conv.from<fs::path>( 5 ) ==
             conv.err( "expected type string, instead found "
                       "type integer." ) );
  }
}

TEST_CASE( "[cdr/ext-std] chrono::seconds" ) {
  SECTION( "to_canonical" ) {
    REQUIRE( conv.to( chrono::seconds{ 5 } ) == 5 );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<chrono::seconds>( 5 ) ==
             chrono::seconds{ 5 } );
    REQUIRE( conv.from<chrono::seconds>( "5" ) ==
             conv.err( "expected type integer, instead found "
                       "type string." ) );
  }
}

TEST_CASE( "[cdr/ext-std] pair" ) {
  SECTION( "to_canonical" ) {
    pair<int, bool> p{ 5, true };
    REQUIRE( conv.to( p ) ==
             table{ { "key", 5 }, { "val", true } } );
  }
  SECTION( "from_canonical" ) {
    REQUIRE( conv_from_bt<pair<int, bool>>(
                 table{ { "key", 5 }, { "val", true } } ) ==
             pair<int, bool>{ 5, true } );
    REQUIRE( conv.from<pair<int, bool>>(
                 table{ { "fxt", 5 }, { "val", true } } ) ==
             conv.err( "key 'key' not found in table." ) );
    REQUIRE(
        conv.from<pair<int, bool>>( table{
            { "key", 9 }, { "fxt", 5 }, { "val", true } } ) ==
        conv.err( "unrecognized key 'fxt' in table." ) );
    REQUIRE( conv.from<pair<int, bool>>( 5 ) ==
             conv.err( "expected type table, instead found type "
                       "integer." ) );
  }
}

TEST_CASE( "[cdr/ext-std] vector" ) {
  SECTION( "to_canonoical" ) {
    vector<int> empty;
    REQUIRE( conv.to( empty ) == list{} );
    vector<int> vec{ 3, 4, 5 };
    REQUIRE( conv.to( vec ) == list{ 3, 4, 5 } );
  }
  SECTION( "from_canonoical" ) {
    REQUIRE( conv_from_bt<vector<double>>( list{ 5.5, 7.7 } ) ==
             vector<double>{ 5.5, 7.7 } );
    REQUIRE(
        conv.from<vector<double>>( table{} ) ==
        conv.err(
            "expected type list, instead found type table." ) );
    REQUIRE( conv.from<vector<double>>( list{ true } ) ==
             conv.err( "failed to convert value of type boolean "
                       "to double." ) );
  }
}

TEST_CASE( "[cdr/ext-std] array" ) {
  SECTION( "to_canonoical" ) {
    array<int, 0> empty;
    REQUIRE( conv.to( empty ) == list{} );
    array<int, 3> arr{ 3, 4, 5 };
    REQUIRE( conv.to( arr ) == list{ 3, 4, 5 } );
  }
  SECTION( "from_canonoical" ) {
    REQUIRE( conv_from_bt<array<int, 2>>( list{ 5, 7 } ) ==
             array<int, 2>{ 5, 7 } );
    REQUIRE(
        conv.from<array<int, 2>>( list{ 5 } ) ==
        conv.err(
            "expected list of size 2, instead found size 1." ) );
    REQUIRE( conv.from<array<int, 2>>( 5.5 ) ==
             conv.err( "expected type list, instead found type "
                       "floating." ) );
    REQUIRE( conv.from<array<int, 2>>( list{ true, false } ) ==
             conv.err( "failed to convert value of type boolean "
                       "to int." ) );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_map (list)" ) {
  SECTION( "to_canonoical" ) {
    unordered_map<int, double> empty;
    REQUIRE( conv.to( empty ) == list{} );
    unordered_map<int, double> m1{ { 3, 5.5 }, { 4, 7.7 } };
    value                      v1 = conv.to( m1 );
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
    REQUIRE( conv_from_bt<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_map (table)" ) {
  SECTION( "to_canonoical" ) {
    // Here we have string keys, so it should convert to a Cdr
    // table.
    unordered_map<string, double> empty;
    REQUIRE( conv.to( empty ) == table{} );
    unordered_map<string, double> m1{ { "3", 5.5 },
                                      { "4", 7.7 } };

    value v1 = conv.to( m1 );
    REQUIRE( v1 == table{ { "3", 5.5 }, { "4", 7.7 } } );
    unordered_map<string_view, double> m2{ { "3", 5.5 },
                                           { "4", 7.7 } };

    value v2 = conv.to( m2 );
    REQUIRE( v2 == table{ { "3", 5.5 }, { "4", 7.7 } } );
  }
  SECTION( "from_canonoical" ) {
    // Here we can convert from either a table or a list, but
    // here we are testing the table version.
    using M = unordered_map<string, int>;
    M     expected{ { "one", 1 }, { "two", 2 } };
    value v = table{ { "one", 1 }, { "two", 2 } };
    REQUIRE( conv_from_bt<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] map (list)" ) {
  SECTION( "to_canonoical" ) {
    map<int, double> empty;
    REQUIRE( conv.to( empty ) == list{} );
    map<int, double> m1{ { 3, 5.5 }, { 4, 7.7 } };
    value            v1 = conv.to( m1 );
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
    using M = map<string, int>;
    M     expected{ { "one", 1 }, { "two", 2 } };
    value v = list{ table{ { "key", "one" }, { "val", 1 } },
                    table{ { "key", "two" }, { "val", 2 } } };
    REQUIRE( conv_from_bt<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] map (table)" ) {
  SECTION( "to_canonoical" ) {
    // Here we have string keys, so it should convert to a Cdr
    // table.
    map<string, double> empty;
    REQUIRE( conv.to( empty ) == table{} );
    map<string, double> m1{ { "3", 5.5 }, { "4", 7.7 } };

    value v1 = conv.to( m1 );
    REQUIRE( v1 == table{ { "3", 5.5 }, { "4", 7.7 } } );
    map<string_view, double> m2{ { "3", 5.5 }, { "4", 7.7 } };

    value v2 = conv.to( m2 );
    REQUIRE( v2 == table{ { "3", 5.5 }, { "4", 7.7 } } );
  }
  SECTION( "from_canonoical" ) {
    // Here we can convert from either a table or a list, but
    // here we are testing the table version.
    using M = map<string, int>;
    M     expected{ { "one", 1 }, { "two", 2 } };
    value v = table{ { "one", 1 }, { "two", 2 } };
    REQUIRE( conv_from_bt<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_set" ) {
  SECTION( "to_canonoical" ) {
    unordered_set<string> empty;
    REQUIRE( conv.to( empty ) == list{} );
    unordered_set<string> m1{ "hello", "world" };
    value                 v1 = conv.to( m1 );
    REQUIRE( ( ( v1 == list{ "hello", "world" } ) ||
               ( v1 == list{ "world", "hello" } ) ) );
  }
  SECTION( "from_canonoical" ) {
    using M = unordered_set<int>;
    M     expected{ 1, 2, 3 };
    value v = list{ 2, 3, 3, 1, 2, 1, 1 };
    REQUIRE( conv_from_bt<M>( v ) == expected );
  }
}

TEST_CASE( "[cdr/ext-std] unordered_set invalid element" ) {
  using M        = unordered_set<int>;
  value v        = list{ 2, 3, 3, 1, "2", 1, 1 };
  auto  expected = conv.err(
       "message: failed to convert value of type string to int.\n"
        "frame trace (most recent frame last):\n"
        "---------------------------------------------------\n"
        "std::unordered_set<int, std::hash<int>, "
        "std::equal_to<int>, st...\n"
        " \\-index 4\n"
        "    \\-int\n"
        "---------------------------------------------------" );
  REQUIRE( conv_from_bt<M>( v ) == expected );
}

TEST_CASE( "[cdr/ext-std] unique_ptr" ) {
  SECTION( "to_canonoical" ) {
    unique_ptr<string> empty;
    REQUIRE( conv.to( empty ) == null );
    auto  u = make_unique<string>( "hello" );
    value v = conv.to( u );
    REQUIRE( v == "hello" );
  }
  SECTION( "from_canonoical" ) {
    REQUIRE( conv_from_bt<unique_ptr<string>>( null ) ==
             unique_ptr<string>( nullptr ) );
    value                      v = "hello";
    result<unique_ptr<string>> res =
        conv_from_bt<unique_ptr<string>>( v );
    REQUIRE( res.has_value() );
    REQUIRE( *res != nullptr );
    REQUIRE( **res == "hello" );
  }
}

} // namespace
} // namespace cdr
