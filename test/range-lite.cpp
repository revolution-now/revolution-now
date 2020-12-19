/****************************************************************
**range-lite.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-16.
*
* Description: Unit tests for the src/base/range-lite.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/range-lite.hpp"

// base
#include "base/lambda.hpp"

// Must be last.
#include "catch-common.hpp"

namespace base {

using namespace std;

// This function exists just to get the return type; we can't de-
// cltype it since we can't have lambdas in unevaluated contexts.
//
// FIXME: can get rid of this once clang gets support for the
// C++20 feature called "lambdas in unevaluated contexts".
auto GetCompoundViewType() {
  return rl::all( vector<int>{} )
      .keep_if( L( _ % 2 == 1 ) )
      .map( L( _ * _ ) )
      .map( L( _ + 1 ) )
      .take_while( L( _ < 27 ) );
}

auto GetCompoundViewOfRefs() {
  return rl::all( vector<int>{} ).keep_if( L( _ % 2 == 1 ) );
}

namespace {

using ::Catch::Equals;

TEST_CASE( "[range-lite] double traverse" ) {
  vector<int> input{ 1, 2, 3 };

  auto view = rl::all( input ).cycle().take( 4 );

  auto vec1 = view.to_vector();
  REQUIRE_THAT( vec1, Equals( vector<int>{ 1, 2, 3, 1 } ) );

  auto vec2 = view.to_vector();
  REQUIRE_THAT( vec2, Equals( vector<int>{ 1, 2, 3, 1 } ) );
}

TEST_CASE( "[range-lite] non-materialized" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::all( input )
                 .keep_if( L( _ % 2 == 1 ) )
                 .map( L( _ * _ ) )
                 .map( L( _ + 1 ) )
                 .take_while( L( _ < 27 ) );

  static_assert( sizeof( decltype( vec ) ) == 48 );
}

TEST_CASE( "[range-lite] long-range" ) {
  vector<int> input;
  input.reserve( 10000 );
  for( int s = 0; s < 10000; ++s ) input.push_back( s );
  auto vec = rl::all( input )
                 .keep_if( L( _ < 100000 ) )
                 .map( L( _ * 2 ) )
                 .map( L( _ / 2 ) )
                 .take_while( L( _ >= 0 ) );

  REQUIRE_THAT( vec.to_vector(), Equals( input ) );
}

TEST_CASE( "[range-lite] move-only" ) {
  struct A {
    A( int m ) : n( m ) {}
    A( A const& a ) = delete;
    A& operator=( A const& a ) = delete;
    A( A&& a ) { n = a.n; }
    A& operator=( A&& a ) {
      n = a.n;
      return *this;
    }
    bool operator==( A const& ) const = default;
    int  n;
  };
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto view = rl::all( input ).map_L( A{ _ } ).drop( 3 ).map_L(
      A{ _.n } );

  vector<A> expected;
  expected.emplace_back( 4 );
  expected.emplace_back( 5 );
  expected.emplace_back( 6 );
  expected.emplace_back( 7 );
  expected.emplace_back( 8 );
  expected.emplace_back( 9 );

  REQUIRE_THAT( view.to_vector(), Equals( expected ) );
}

TEST_CASE( "[range-lite] move-over-copy" ) {
  static bool copied = false;
  static bool moved  = false;
  struct A {
    A( int m ) : n( m ) {}
    A( A const& a ) : n( a.n ) { copied = true; }
    A& operator=( A const& a ) {
      n      = a.n;
      copied = true;
      return *this;
    }
    A( A&& a ) {
      n     = a.n;
      moved = true;
    }
    A& operator=( A&& a ) {
      n     = a.n;
      moved = true;
      return *this;
    }
    bool operator==( A const& ) const = default;
    int  n;
  };
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto view = rl::all( input ).map_L( A{ _ } ).map_L( A{ _.n } );

  vector<A> expected{ { 1 }, { 2 }, { 3 }, { 4 }, { 5 },
                      { 6 }, { 7 }, { 8 }, { 9 } };
  copied = moved = false;

  REQUIRE_THAT( view.to_vector(), Equals( expected ) );
  REQUIRE( moved );
  REQUIRE_FALSE( copied );
}

TEST_CASE( "[range-lite] materialized" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::all( input )
                 .keep_if( L( _ % 2 == 1 ) )
                 .map( L( _ * _ ) )
                 .map( L( _ + 1 ) )
                 .take_while( L( _ < 27 ) )
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[range-lite] lambdas with capture" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  int n = 2;

  auto vec = rl::all( input )
                 .keep_if( LC( _ % n == 1 ) )
                 .map( LC( _ * n ) )
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 6, 10, 14, 18 } ) );
}

TEST_CASE( "[range-lite] static create" ) {
  SECTION( "one" ) {
    using type = decltype( GetCompoundViewType() );
    static_assert( sizeof( type ) == 48 );

    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    auto        view = type::create( input );
    static_assert( sizeof( decltype( view ) ) == 48 );

    REQUIRE_THAT( view.to_vector(),
                  Equals( vector<int>{ 2, 10, 26 } ) );
  }
  SECTION( "two" ) {
    using type = decltype( GetCompoundViewOfRefs() );
    static_assert( sizeof( type ) == 24 );

    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    auto        view = type::create( input );
    static_assert( sizeof( decltype( view ) ) == 24 );

    for( int& i : view ) i *= 10;
    REQUIRE_THAT( input, Equals( vector<int>{ 10, 2, 30, 4, 50,
                                              6, 70, 8, 90 } ) );
  }
}

TEST_CASE( "[range-lite] input/attach" ) {
  auto view = rl::input<vector<int>>()
                  .drop( 4 )
                  .cycle()
                  .take( 5 )
                  .map_L( to_string( _ ) )
                  .remove_if_L( _ == "2" );

  vector<int> input{ 9, 8, 7, 6, 5, 4, 3, 2, 1 };
  view.attach( input );

  REQUIRE_THAT( view.to_vector(),
                Equals( vector<string>{ "5", "4", "3", "1" } ) );
}

TEST_CASE( "[range-lite] rview" ) {
  vector<int> input{ 9, 8, 7, 6, 5, 4, 3, 2, 1 };

  auto vec = rl::rall( input )
                 .keep_if( L( _ % 2 == 1 ) )
                 .map( L( _ * _ ) )
                 .map( L( _ + 1 ) )
                 .take_while( L( _ < 27 ) )
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[range-lite] macros" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::all( input )
                 .keep_if_L( _ % 2 == 1 )
                 .map_L( _ * _ )
                 .map_L( _ + 1 )
                 .take_while_L( _ < 27 )
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[range-lite] head" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto res = rl::all( input )
                 .keep_if( L( _ % 2 == 1 ) )
                 .map( L( _ * _ ) )
                 .map( L( _ + 1 ) )
                 .take_while( L( _ < 27 ) )
                 .head();

  REQUIRE( res == 2 );

  auto res2 = rl::all( input ).take_while_L( _ > 100 ).head();

  REQUIRE( res2 == nothing );
}

TEST_CASE( "[range-lite] write-through head" ) {
  SECTION( "non-const ref" ) {
    vector<int> input{ 1, 2, 3 };

    auto res = rl::all( input ).head();
    static_assert( is_same_v<decltype( res ), maybe<int&>> );
    REQUIRE( res.has_value() );
    REQUIRE( res == 1 );

    *res += 1; // write through into `input`.

    REQUIRE_THAT( input, Equals( vector<int>{ 2, 2, 3 } ) );
  }
  SECTION( "const ref" ) {
    vector<int> const input{ 1, 2, 3 };

    auto res = rl::all( input ).head();
    REQUIRE( res.has_value() );
    REQUIRE( res == 1 );
    static_assert(
        is_same_v<decltype( res ), maybe<int const&>> );
  }
}

TEST_CASE( "[range-lite] remove" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto odd = L( _ % 2 == 1 );

  auto res = rl::all( input ).remove_if( odd ).to_vector();

  REQUIRE_THAT( res, Equals( vector<int>{ 2, 4, 6, 8 } ) );
}

TEST_CASE( "[range-lite] to" ) {
  string msg = "1hello 123 with 345 numbers44";

  auto is_num = L( _ >= '0' && _ <= '9' );

  auto res = rl::all( msg ).remove_if( is_num ).to<string>();

  REQUIRE( res == "hello  with  numbers" );
}

TEST_CASE( "[range-lite] accumulate" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto res = rl::all( input )
                 .keep_if( L( _ % 2 == 1 ) )
                 .map( L( _ * _ ) )
                 .map( L( _ + 1 ) )
                 .accumulate();

  REQUIRE( res == 2 + 10 + 26 + 50 + 82 );

  auto res2 = rl::all( input )
                  .keep_if( L( _ % 2 == 1 ) )
                  .map( L( _ * _ ) )
                  .map( L( _ + 1 ) )
                  .accumulate( std::multiplies{}, 1 );

  REQUIRE( res2 == 2 * 10 * 26 * 50 * 82 );

  auto res3 =
      rl::all( input ).map_L( to_string( _ ) ).accumulate();

  REQUIRE( res3 == "123456789" );
}

TEST_CASE( "[range-lite] mixing" ) {
  vector<int> input{ 1, 22, 333, 4444, 55555, 666666, 7777777 };

  auto res = rl::all( input )
                 .keep_if( L( _ % 2 == 1 ) )
                 .map_L( to_string( _ ) )
                 .map_L( _.size() )
                 .to_vector();

  REQUIRE_THAT( res, Equals( vector<size_t>{ 1, 3, 5, 7 } ) );
}

TEST_CASE( "[range-lite] zip" ) {
  SECTION( "zip: int, int" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_odd = rl::all( input ).keep_if( L( _ % 2 == 1 ) );

    auto vec = rl::all( input )
                   .keep_if( L( _ % 2 == 0 ) )
                   .zip( view_odd )
                   .take_while( L( _.second < 8 ) )
                   .to_vector();

    auto expected = vector<pair<int, int>>{
        { 2, 1 },
        { 4, 3 },
        { 6, 5 },
        { 8, 7 },
    };
    REQUIRE_THAT( vec, Equals( expected ) );
  }
  SECTION( "zip: int, string" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = rl::all( input ).map( L( to_string( _ ) ) );

    auto vec = rl::all( input )
                   .keep_if( L( _ % 2 == 0 ) )
                   .zip( view_str )
                   .to_vector();

    auto expected = vector<pair<int, string>>{
        { 2, "1" },
        { 4, "2" },
        { 6, "3" },
        { 8, "4" },
    };
    REQUIRE_THAT( vec, Equals( expected ) );
  }
  SECTION( "zip: empty" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = rl::all( input ).map( L( to_string( _ ) ) );

    auto vec = rl::all( input )
                   .keep_if( L( _ > 200 ) )
                   .zip( view_str )
                   .to_vector();

    auto expected = vector<pair<int, string>>{};
    REQUIRE_THAT( vec, Equals( expected ) );
  }
}

TEST_CASE( "[range-lite] take_while_incl" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::all( input )
                 .take_while_incl_L( _ < 5 )
                 .map_L( _ + 1 )
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 3, 4, 5, 6 } ) );

  auto vec2 = rl::all( input )
                  .take_while_incl_L( _ < 50 )
                  .map_L( _ + 1 )
                  .to_vector();

  REQUIRE_THAT( vec2, Equals( vector<int>{ 2, 3, 4, 5, 6, 7, 8,
                                           9, 10 } ) );

  auto vec3 =
      rl::all( input ).take_while_incl_L( _ < 0 ).to_vector();

  REQUIRE_THAT( vec3, Equals( vector<int>{ 1 } ) );
}

TEST_CASE( "[range-lite] take" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::all( input )
                 .take_while_incl_L( _ < 5 )
                 .take( 2 )
                 .map_L( _ + 1 )
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 3 } ) );

  auto vec2 = rl::all( input )
                  .take_while_incl_L( _ < 50 )
                  .take( 0 )
                  .map_L( _ + 1 )
                  .to_vector();

  REQUIRE_THAT( vec2, Equals( vector<int>{} ) );

  auto vec3 = rl::all( input )
                  .take_while_L( _ < 0 )
                  .take( 5 )
                  .to_vector();

  REQUIRE_THAT( vec3, Equals( vector<int>{} ) );
}

TEST_CASE( "[range-lite] drop" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec =
      rl::all( input ).take( 7 ).drop( 2 ).take( 3 ).to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 3, 4, 5 } ) );

  auto vec2 =
      rl::all( input ).take( 7 ).drop( 0 ).take( 3 ).to_vector();

  REQUIRE_THAT( vec2, Equals( vector<int>{ 1, 2, 3 } ) );

  auto vec3 =
      rl::all( input ).take( 7 ).drop( 7 ).take( 3 ).to_vector();

  REQUIRE_THAT( vec3, Equals( vector<int>{} ) );

  auto vec4 = rl::all( input )
                  .take( 7 )
                  .drop( 10 )
                  .take( 3 )
                  .to_vector();

  REQUIRE_THAT( vec4, Equals( vector<int>{} ) );

  auto vec5 = rl::all( input ).take( 0 ).drop( 2 ).to_vector();

  REQUIRE_THAT( vec5, Equals( vector<int>{} ) );
}

TEST_CASE( "[range-lite] cycle" ) {
  SECTION( "cycle: basic" ) {
    vector<int> input{ 1, 2, 3 };

    auto vec = rl::all( input ).cycle().take( 10 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 3, 1, 2, 3, 1,
                                            2, 3, 1 } ) );
  }
  SECTION( "cycle: single" ) {
    vector<int> input{ 1 };

    auto vec = rl::all( input ).cycle().take( 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 1, 1, 1, 1 } ) );
  }
  SECTION( "cycle: double" ) {
    vector<int> input{ 1, 2 };

    auto vec = rl::all( input ).cycle().take( 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 1, 2, 1 } ) );
  }
  SECTION( "cycle: with stateful function" ) {
    vector<int> input{ 1, 2 };

    int n = 3;

    function<int( int )> f = LC( _ + n );

    auto vec = rl::all( input )
                   .cycle()
                   .take( 3 )
                   .map( std::move( f ) )
                   .cycle()
                   .take( 7 )
                   .to_vector();

    REQUIRE_THAT( vec,
                  Equals( vector<int>{ 4, 5, 4, 4, 5, 4, 4 } ) );
  }
  SECTION( "cycle: with decayed lambda" ) {
    vector<int> input{ 1, 2 };

    auto* f = +[]( int n ) { return n + 3; };

    auto vec = rl::all( input )
                   .cycle()
                   .take( 3 )
                   .map( f )
                   .cycle()
                   .take( 7 )
                   .to_vector();

    REQUIRE_THAT( vec,
                  Equals( vector<int>{ 4, 5, 4, 4, 5, 4, 4 } ) );
  }
}

TEST_CASE( "[range-lite] ints" ) {
  SECTION( "ints: 0, ..." ) {
    auto vec = rl::ints().take( 10 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 0, 1, 2, 3, 4, 5, 6,
                                            7, 8, 9 } ) );
  }
  SECTION( "ints: -5, ..." ) {
    auto vec = rl::ints( -5 ).take( 10 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ -5, -4, -3, -2, -1,
                                            0, 1, 2, 3, 4 } ) );
  }
  SECTION( "ints: 8, 20" ) {
    auto vec = rl::ints( 8, 20 ).to_vector();

    REQUIRE_THAT(
        vec, Equals( vector<int>{ 8, 9, 10, 11, 12, 13, 14, 15,
                                  16, 17, 18, 19 } ) );
  }
}

TEST_CASE( "[range-lite] enumerate" ) {
  vector<string> input{ "hello", "world", "one", "two" };

  auto vec =
      rl::all( input ).cycle().enumerate().take( 8 ).to_vector();

  auto expected = vector<pair<int, string>>{
      { 0, "hello" }, { 1, "world" }, { 2, "one" }, { 3, "two" },
      { 4, "hello" }, { 5, "world" }, { 6, "one" }, { 7, "two" },
  };
  REQUIRE_THAT( vec, Equals( expected ) );
}

TEST_CASE( "[range-lite] free-standing zip" ) {
  SECTION( "free-zip: vectors" ) {
    vector<string> input1{ "hello", "world", "one", "two" };
    vector<int>    input2{ 4, 6, 2, 7, 3 };

    auto view = rl::zip( input1, input2 ).take( 3 );

    auto expected = vector<pair<string, int>>{
        { "hello", 4 }, { "world", 6 }, { "one", 2 } };
    REQUIRE_THAT( view.to_vector(), Equals( expected ) );
  }
  SECTION( "free-zip: Views" ) {
    auto view =
        rl::zip( rl::ints( 5 ), rl::ints( 7 ) ).take( 3 );

    auto expected =
        vector<pair<int, int>>{ { 5, 7 }, { 6, 8 }, { 7, 9 } };
    REQUIRE_THAT( view.to_vector(), Equals( expected ) );
  }
}

TEST_CASE( "[range-lite] mutation" ) {
  vector<int> input{ 1, 2, 3, 4, 5 };

  auto view = rl::all( input ).cycle().drop( 2 ).take( 4 );

  for( int& i : view ) i *= 10;

  auto expected = vector<int>{ 10, 2, 30, 40, 50 };
  REQUIRE_THAT( input, Equals( expected ) );
}

TEST_CASE( "[range-lite] keys" ) {
  vector<pair<string, int>> input{
      { "hello", 3 },
      { "world", 2 },
      { "again", 1 },
  };

  auto vec =
      rl::all( input ).cycle().keys().take( 6 ).to_vector();

  vector<string> expected{
      { "hello" }, { "world" }, { "again" },
      { "hello" }, { "world" }, { "again" },
  };

  REQUIRE_THAT( vec, Equals( expected ) );
}

TEST_CASE( "[range-lite] mutable through map" ) {
  vector<string> input{ "dropped", "hello", "world", "again" };

  auto view = rl::all( input )
                  // Get reference to second char.
                  .map( []( string& s ) -> decltype( auto ) {
                    return s.data()[1];
                  } )
                  .drop( 1 )
                  // Forward the ref to second char.
                  .map( []( char& c ) -> decltype( auto ) {
                    return c;
                  } );

  for( auto& c : view ) ++c;

  vector<string> expected{ "dropped", "hfllo", "wprld",
                           "ahain" };

  REQUIRE_THAT( input, Equals( expected ) );
}

TEST_CASE( "[range-lite] keys mutation" ) {
  vector<pair<string, int>> input{
      { "hello", 3 },
      { "world", 2 },
      { "again", 1 },
  };

  auto view =
      rl::all( input ).cycle().keys().drop( 1 ).take( 2 );

  for( auto& key : view ) key = key + key;

  vector<pair<string, int>> expected{
      { "hello", 3 },
      { "worldworld", 2 },
      { "againagain", 1 },
  };

  REQUIRE_THAT( input, Equals( expected ) );
}

TEST_CASE( "[range-lite] take_while" ) {
  SECTION( "take_while: empty" ) {
    vector<int> input{};

    auto vec =
        rl::all( input ).take_while_L( _ < 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{} ) );
  }
  SECTION( "take_while: take all" ) {
    vector<int> input{ 1, 2, 3, 4 };

    auto vec =
        rl::all( input ).take_while_L( _ < 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 3, 4 } ) );
  }
  SECTION( "take_while: take none" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec =
        rl::all( input ).take_while_L( _ < 0 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{} ) );
  }
  SECTION( "take_while: take some" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec =
        rl::all( input ).take_while_L( _ < 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 3, 4 } ) );
  }
}

TEST_CASE( "[range-lite] drop_while" ) {
  SECTION( "drop_while: empty" ) {
    vector<int> input{};

    auto vec =
        rl::all( input ).drop_while_L( _ < 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{} ) );
  }
  SECTION( "drop_while: drop all" ) {
    vector<int> input{ 1, 2, 3, 4 };

    auto vec =
        rl::all( input ).drop_while_L( _ < 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{} ) );
  }
  SECTION( "drop_while: drop none" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec =
        rl::all( input ).drop_while_L( _ < 0 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 3, 4, 5, 6, 7,
                                            8, 9 } ) );
  }
  SECTION( "drop_while: drop some" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec =
        rl::all( input ).drop_while_L( _ < 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 5, 6, 7, 8, 9 } ) );
  }
}

TEST_CASE( "[range-lite] dereference" ) {
  vector<maybe<int>> input{ { 1 }, { 2 }, { 3 }, { 4 }, { 5 } };

  auto view = rl::all( input ).dereference();

  for( int& i : view ) i *= 10;

  auto expected = vector<maybe<int>>{
      { 10 }, { 20 }, { 30 }, { 40 }, { 50 } };
  REQUIRE_THAT( input, Equals( expected ) );

  REQUIRE_THAT( view.to_vector(),
                Equals( vector<int>{ 10, 20, 30, 40, 50 } ) );
}

TEST_CASE( "[range-lite] cat_maybes" ) {
  vector<maybe<int>> input{
      { 1 }, nothing, { 3 }, nothing, { 5 } };

  auto view = rl::all( input ).cat_maybes();

  for( int& i : view ) i *= 10;

  auto expected = vector<maybe<int>>{
      { 10 }, nothing, { 30 }, nothing, { 50 } };
  REQUIRE_THAT( input, Equals( expected ) );

  REQUIRE_THAT( view.to_vector(),
                Equals( vector<int>{ 10, 30, 50 } ) );
}

TEST_CASE( "[range-lite] tail" ) {
  SECTION( "tail: some" ) {
    vector<int> input{ 1, 2, 3, 4 };
    auto        vec = rl::all( input ).tail().to_vector();
    REQUIRE_THAT( vec, Equals( vector<int>{ 2, 3, 4 } ) );
  }
  SECTION( "tail: empty" ) {
    vector<int> input{};
    auto        vec = rl::all( input ).tail().to_vector();
    REQUIRE_THAT( vec, Equals( vector<int>{} ) );
  }
}

TEST_CASE( "[range-lite] group_by" ) {
  SECTION( "group_by: empty" ) {
    vector<int> input{};

    auto view = rl::all( input ).group_by( L2( _1 == _2 ) );

    auto it = view.begin();
    REQUIRE( it == view.end() );

    vector<vector<int>> expected{};
    REQUIRE_THAT(
        std::move( view ).map_L( _.to_vector() ).to_vector(),
        Equals( expected ) );
  }
  SECTION( "group_by: single" ) {
    vector<int> input{ 1 };

    auto view = rl::all( input ).group_by( L2( _1 == _2 ) );

    auto it = view.begin();
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 1 } ) );

    ++it;
    REQUIRE( it == view.end() );

    vector<vector<int>> expected{ { 1 } };
    REQUIRE_THAT(
        std::move( view ).map_L( _.to_vector() ).to_vector(),
        Equals( expected ) );
  }
  SECTION( "group_by: many" ) {
    vector<int> input{ 1, 2, 2, 2, 3, 3, 4, 5, 5, 5, 5, 6 };

    auto view = rl::all( input ).group_by( L2( _1 == _2 ) );

    auto it = view.begin();
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 1 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 2, 2, 2 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 3, 3 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 4 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 5, 5, 5, 5 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( ( *it ).to_vector(),
                  Equals( vector<int>{ 6 } ) );

    ++it;
    REQUIRE( it == view.end() );

    vector<vector<int>> expected{ { 1 },          { 2, 2, 2 },
                                  { 3, 3 },       { 4 },
                                  { 5, 5, 5, 5 }, { 6 } };
    REQUIRE_THAT(
        std::move( view ).map_L( _.to_vector() ).to_vector(),
        Equals( expected ) );
  }
}

TEST_CASE( "[range-lite] group_by complicated" ) {
  vector<string> input{ "hello", "world", "four", "seven",
                        "six",   "two",   "three" };

  auto view = rl::all( input )
                  .cycle()
                  .group_by_L( _1.size() == _2.size() )
                  .take( 6 );

  vector<vector<string>> expected{ { "hello", "world" },
                                   { "four" },
                                   { "seven" },
                                   { "six", "two" },
                                   { "three", "hello", "world" },
                                   { "four" } };
  auto                   view2 = view;
  REQUIRE_THAT(
      std::move( view2 ).map_L( _.to_vector() ).to_vector(),
      Equals( expected ) );

  for( auto [l, r] : rl::zip( view, expected ) ) {
    REQUIRE_THAT( l.to_vector(), Equals( r ) );
  }
}

TEST_CASE( "[range-lite] filter alias" ) {
  vector<string> input{ "hello", "world", "four", "seven",
                        "six",   "two",   "three" };

  auto view = rl::all( input ).filter_L( _.size() == 3 );

  vector<string> expected{ "six", "two" };
  REQUIRE_THAT( view.to_vector(), Equals( expected ) );
}

} // namespace
} // namespace base
