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
#include "test/testing.hpp"

// Under test.
#include "src/base/range-lite.hpp"

// base
#include "base/conv.hpp"
#include "base/lambda.hpp"
#include "base/string.hpp"
#include "base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp"

// C++ standard library
#include <unordered_map>

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

TEST_CASE( "[range-lite] rall" ) {
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

TEST_CASE( "[range-lite] cache1" ) {
  vector<int> input{ 1, 2, 3, 4 };

  int count = 0;

  auto f = [&]( int m ) {
    ++count;
    return m;
  };

  auto view = rl::all( input ).map( f );

  // First without cache1 as sanity check.
  for( auto it = view.begin(); it != view.end(); ++it ) {
    *it;
    *it;
    *it;
  }
  REQUIRE( count == 3 * 4 );

  count = 0;

  // Now with cache.
  auto cached = std::move( view ).cache1().map_L( _ );
  for( auto it = cached.begin(); it != cached.end(); ++it ) {
    *it;
    *it;
    *it;
  }
  REQUIRE( count == 4 );
}

TEST_CASE( "[range-lite] cache1 no ref" ) {
  vector<int> input{ 1, 2, 3, 4 };

  auto non_cached = rl::all( input ).take( 4 );
  static_assert(
      is_same_v<decltype( *non_cached.begin() ), int&> );

  auto cached1 = rl::all( input ).cache1().take( 4 );
  static_assert( is_same_v<decltype( *cached1.begin() ), int> );

  auto cached2 = rl::all( input ).take( 4 ).cache1();
  static_assert( is_same_v<decltype( *cached2.begin() ), int> );

  auto cached3 = rl::all( input ).cache1().map(
      []<typename T>( T&& _ ) { return std::forward<T>( _ ); } );
  static_assert( is_same_v<decltype( *cached3.begin() ), int> );

  auto cached4 = rl::all( input )
                     .map( []( auto&& _ ) -> decltype( auto ) {
                       return _;
                     } )
                     .cache1();
  static_assert( is_same_v<decltype( *cached4.begin() ), int> );
}

TEST_CASE( "[range-lite] string_view" ) {
  string_view sv = "hello world";

  auto view = rl::rall( sv ).map_L( _ == ' ' ? '-' : _ );

  REQUIRE( view.to_string() == "dlrow-olleh" );
}

TEST_CASE( "[range-lite] to_string" ) {
  string_view sv = "hello world";

  auto view = rl::rall( sv ).map_L( _ == ' ' ? '-' : _ );

  REQUIRE( view.to_string() == "dlrow-olleh" );
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

  auto res = rl::all( msg ).remove_if( is_num ).to_string();

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

TEST_CASE( "[range-lite] accumulate_monoid" ) {
  SECTION( "empty" ) {
    vector<int> input{};
    auto        res = rl::all( input ).accumulate_monoid();
    REQUIRE( res == nothing );
  }
  SECTION( "single" ) {
    vector<int> input{ 10 };
    auto        res = rl::all( input ).accumulate_monoid();
    REQUIRE( res == 10 );
  }
  SECTION( "multiple" ) {
    vector<int> input{ 1, 2, 3, 4, 5 };
    auto        res =
        rl::all( input ).accumulate_monoid( std::multiplies{} );
    REQUIRE( res == 1 * 2 * 3 * 4 * 5 );
  }
  SECTION( "string" ) {
    vector<int> input{ 1, 2, 3, 4, 5 };
    auto        res = rl::all( input )
                   .map_L( to_string( _ ) )
                   .accumulate_monoid();
    REQUIRE( res == "12345" );
  }
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

TEST_CASE( "[range-lite] min_by" ) {
  struct A {
    bool operator==( A const& ) const = default;
    int  n;
  };
  vector<A> input{ { 5 }, { 4 }, { 3 }, { 10 } };

  auto res = rl::all( input ).min_by_L( _.n );
  static_assert( is_same_v<decltype( res ), maybe<A>> );

  REQUIRE( res == A{ 3 } );
}

TEST_CASE( "[range-lite] max_by" ) {
  struct A {
    bool operator==( A const& ) const = default;
    int  n;
  };
  vector<A> input{ { 5 }, { 4 }, { 3 }, { 10 }, { 8 } };

  auto res = rl::all( input ).max_by_L( _.n );
  static_assert( is_same_v<decltype( res ), maybe<A>> );

  REQUIRE( res == A{ 10 } );
}

TEST_CASE( "[range-lite] min" ) {
  vector<int> input{ 5, 4, 3, 10 };
  auto        res = rl::all( input ).min();
  REQUIRE( res == 3 );
}

TEST_CASE( "[range-lite] max" ) {
  vector<int> input{ 5, 4, 3, 10, 8 };
  auto        res = rl::all( input ).max();
  REQUIRE( res == 10 );
}

TEST_CASE( "[range-lite] map2val" ) {
  SECTION( "int" ) {
    vector<int> input{ 1, 2, 3 };

    auto res = rl::all( input ).map2val_L( _ * _ ).to_vector();
    static_assert(
        is_same_v<decltype( res ), vector<pair<int&, int>>> );

    int n1 = 1, n2 = 2, n3 = 3;
    REQUIRE_THAT( res, Equals( vector<pair<int&, int>>{
                           { n1, 1 }, { n2, 4 }, { n3, 9 } } ) );
  }
  SECTION( "string" ) {
    vector<int> input{ 1, 2, 3 };

    auto res =
        rl::all( input ).map2val_L( to_string( _ ) ).to_vector();

    int n1 = 1, n2 = 2, n3 = 3;
    REQUIRE_THAT(
        res, Equals( vector<pair<int&, string>>{
                 { n1, "1" }, { n2, "2" }, { n3, "3" } } ) );
  }
}

TEST_CASE( "[range-lite] as_const" ) {
  SECTION( "reference" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto m1 = rl::all( input ) //
                  .keep_if( L( _ % 2 == 0 ) )
                  .head();
    static_assert( is_same_v<decltype( m1 ), maybe<int&>> );
    REQUIRE( m1 == 2 );

    // Now add an as_const.
    auto m2 = rl::all( input )
                  .keep_if( L( _ % 2 == 0 ) )
                  .as_const()
                  .head();
    static_assert(
        is_same_v<decltype( m2 ), maybe<int const&>> );
    REQUIRE( m2 == 2 );
  }
  SECTION( "value" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    // We can't call as_const() after the map because it can only
    // be called on references, but we can call it before and it
    // should basically have no effect.
    auto m = rl::all( input )
                 .keep_if( L( _ % 2 == 0 ) )
                 .as_const()
                 .map_L( _ )
                 .head();

    static_assert( is_same_v<decltype( m ), maybe<int>> );

    REQUIRE( m == 2 );
  }
}

TEST_CASE( "[range-lite] zip" ) {
  SECTION( "zip: int, int" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_odd =
        rl::all( input ).keep_if( L( _ % 2 == 1 ) ).as_const();

    auto vec = rl::all( input )
                   .keep_if( L( _ % 2 == 0 ) )
                   .zip( view_odd )
                   .take_while( L( _.second < 8 ) )
                   .to_vector();
    static_assert( is_same_v<decltype( vec ),
                             vector<pair<int&, int const&>>> );

    REQUIRE( vec.size() == 4 );
    int n, m;

    // `vec` is a vector of pairs of references, which are a pain
    // to compare.
    n = 2;
    m = 1;
    REQUIRE( vec[0].first == n );
    REQUIRE( vec[0].second == m );
    n = 4;
    m = 3;
    REQUIRE( vec[1].first == n );
    REQUIRE( vec[1].second == m );
    n = 6;
    m = 5;
    REQUIRE( vec[2].first == n );
    REQUIRE( vec[2].second == m );
    n = 8;
    m = 7;
    REQUIRE( vec[3].first == n );
    REQUIRE( vec[3].second == m );
  }
  SECTION( "zip: int, string" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = rl::all( input ).map( L( to_string( _ ) ) );

    auto vec = rl::all( input )
                   .keep_if( L( _ % 2 == 0 ) )
                   .zip( view_str )
                   .to_vector();

    REQUIRE( vec.size() == 4 );
    int    n;
    string m;

    // `vec` is a vector of pairs of references, which are a pain
    // to compare.
    n = 2;
    m = "1";
    REQUIRE( vec[0].first == n );
    REQUIRE( vec[0].second == m );
    n = 4;
    m = "2";
    REQUIRE( vec[1].first == n );
    REQUIRE( vec[1].second == m );
    n = 6;
    m = "3";
    REQUIRE( vec[2].first == n );
    REQUIRE( vec[2].second == m );
    n = 8;
    m = "4";
    REQUIRE( vec[3].first == n );
    REQUIRE( vec[3].second == m );
  }
  SECTION( "zip: empty" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = rl::all( input ).map( L( to_string( _ ) ) );

    auto vec = rl::all( input )
                   .keep_if( L( _ > 200 ) )
                   .zip( view_str )
                   .to_vector();

    REQUIRE( vec.empty() );
  }
}

TEST_CASE( "[range-lite] zip write-through" ) {
  SECTION( "both refs" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view = rl::zip( input, rl::rall( input ) );

    auto it        = view.begin();
    ( *it ).first  = 100;
    ( *it ).second = 200;

    ++it;
    ( *it ).first  = 300;
    ( *it ).second = 400;

    auto expected =
        vector<int>{ 100, 300, 3, 4, 5, 6, 7, 400, 200 };
    REQUIRE_THAT( input, Equals( expected ) );
  }
  SECTION( "one ref" ) {
    vector<int> input1{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    vector<int> input2{ 1, 2, 3, 4, 5 };

    auto view = rl::zip( rl::all( input1 ).map_L( _ ), input2 )
                    .to_vector();
    static_assert(
        is_same_v<decltype( view ), vector<pair<int, int&>>> );

    auto it        = view.begin();
    ( *it ).first  = 100;
    ( *it ).second = 200;

    ++it;
    ( *it ).first  = 300;
    ( *it ).second = 400;

    auto expected = vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    REQUIRE_THAT( input1, Equals( expected ) );
    expected = vector<int>{ 200, 400, 3, 4, 5 };
    REQUIRE_THAT( input2, Equals( expected ) );
  }
}

TEST_CASE( "[range-lite] zip3" ) {
  vector<int> input1{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  vector<int> input2{ 4, 5, 6, 7 };
  vector<int> input3{ 8, 9, 0 };

  auto vec = rl::zip( input1, input2, input3 ).to_vector();

  auto expected = vector<tuple<int, int, int>>{
      { 1, 4, 8 }, { 2, 5, 9 }, { 3, 6, 0 } };
  REQUIRE_THAT( vec, Equals( expected ) );
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

TEST_CASE( "[range-lite] generate_n" ) {
  int  n = 9;
  auto f = [&] { return n--; };

  REQUIRE_THAT( rl::generate_n( f, 0 ).to_vector(),
                Equals( vector<int>{} ) );

  REQUIRE_THAT( rl::generate_n( f, 1 ).to_vector(),
                Equals( vector<int>{ 9 } ) );

  auto vec = rl::generate_n( f, 5 ).to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 8, 7, 6, 5, 4 } ) );
}

TEST_CASE( "[range-lite] enumerate" ) {
  vector<string> input{ "hello", "world", "one", "two" };

  auto vec =
      rl::all( input ).cycle().enumerate().take( 8 ).to_vector();

  REQUIRE( vec.size() == 8 );
  REQUIRE( vec[0].first == 0 );
  REQUIRE( vec[0].second == "hello" );
  REQUIRE( vec[1].first == 1 );
  REQUIRE( vec[1].second == "world" );
  REQUIRE( vec[2].first == 2 );
  REQUIRE( vec[2].second == "one" );
  REQUIRE( vec[3].first == 3 );
  REQUIRE( vec[3].second == "two" );
  REQUIRE( vec[4].first == 4 );
  REQUIRE( vec[4].second == "hello" );
  REQUIRE( vec[5].first == 5 );
  REQUIRE( vec[5].second == "world" );
  REQUIRE( vec[6].first == 6 );
  REQUIRE( vec[6].second == "one" );
  REQUIRE( vec[7].first == 7 );
  REQUIRE( vec[7].second == "two" );
}

string str_tolower( string const& s ) {
  return rl::all( s ).map_L( tolower( _ ) ).to_string();
};

constexpr string_view knuth_mcelrow_text = R"(
  statement AS in form MERCHANTABILITY solely the AND WARRANTIES
  of following the accompanying and this CONTRACT in OF IN NO OF
  WITHOUT software to SOFTWARE HOLDERS of THE EXPRESS Software
  third including OR restriction IS so to IMPLIED works works the
  copy and BUT The part derivative this the PARTICULAR processor
  permit ANY THE to distribute OTHER the this WHETHER copyright
  SOFTWARE THE and a BE code of or granted covered TITLE such NOT
  OR to unless do must and DISTRIBUTING OR OUT PROVIDED ANY
  CONNECTION IN of following copies in NON is use Software works
  above FOR are the disclaimer LIABLE license INCLUDING the
  entire any IN hereby free license is DEALINGS to WITH the OR
  generated or LIMITED TORT furnished COPYRIGHT charge USE
  transmit whole ANYONE FITNESS grant copies all documentation
  SOFTWARE Software ARISING and to OR FROM OR THE TO EVENT
  Permission all the parties machine derivative included OTHER be
  of source of by PURPOSE organization subject the LIABILITY IN
  WARRANTY prepare reproduce IS derivative Software Software and
  SHALL language Software object DAMAGES THE execute SOFTWARE a A
  by THE the OTHERWISE or all and notices in display the person
  Software INFRINGEMENT OR THE OF in to FOR KIND executable
  obtaining whom
)";

// Compute top 7 most frequent words.
TEST_CASE( "[range-lite] knuth-mcelroy with sort" ) {
  vector<string> words =
      rl::all( knuth_mcelrow_text )
          .group_on_L( isalnum( _ ) )
          .map_L( str_tolower( _.to_string() ) )
          .remove_if_L( !isalnum( _[0] ) )
          .to_vector();

  sort( words.begin(), words.end() );

  auto freqs = rl::all( words )
                   .group()
                   .map_L( pair{ *_.begin(), _.distance() } )
                   .to_vector();

  sort( freqs.begin(), freqs.end(), []( auto& l, auto& r ) {
    // Use > because we will subsequently use the result of this
    // sort in reverse because of rl::rall. That will have the
    // effect of ranking the words in order of decreasing count,
    // but sorting words with equal count in ascending order.
    if( l.second == r.second ) return l.first > r.first;
    return l.second < r.second;
  } );

  auto res = rl::rall( freqs ).take( 7 ).to_vector();

  REQUIRE_THAT(
      res, Equals( vector<pair<string, int>>{ { "the", 20 },
                                              { "software", 12 },
                                              { "or", 10 },
                                              { "in", 9 },
                                              { "of", 9 },
                                              { "and", 8 },
                                              { "to", 8 } } ) );
  REQUIRE( rl::all( freqs ).distance() == 111 );
}

TEST_CASE( "[range-lite] knuth-mcelroy with map" ) {
  using M    = unordered_map<string, int>;
  auto freqs = rl::all( knuth_mcelrow_text )
                   .group_on_L( isalnum( _ ) )
                   .map_L( str_tolower( _.to_string() ) )
                   .remove_if_L( !isalnum( _[0] ) )
                   .accumulate(
                       []( M& m, string const& s ) {
                         m[s]++;
                         return std::move( m );
                       },
                       M{} );

  vector<pair<string, int>> v( freqs.begin(), freqs.end() );
  sort( v.begin(), v.end(), []( auto& l, auto& r ) {
    // Use > because we will subsequently use the result of this
    // sort in reverse because of rl::rall. That will have the
    // effect of ranking the words in order of decreasing count,
    // but sorting words with equal count in ascending order.
    if( l.second == r.second ) return l.first > r.first;
    return l.second < r.second;
  } );

  auto res = rl::rall( v ).take( 7 ).to_vector();

  REQUIRE_THAT(
      res, Equals( vector<pair<string, int>>{ { "the", 20 },
                                              { "software", 12 },
                                              { "or", 10 },
                                              { "in", 9 },
                                              { "of", 9 },
                                              { "and", 8 },
                                              { "to", 8 } } ) );
  REQUIRE( rl::all( freqs ).distance() == 111 );
}

TEST_CASE( "[range-lite] free-standing zip" ) {
  SECTION( "free-zip: vectors" ) {
    vector<string> input1{ "hello", "world", "one", "two" };
    vector<int>    input2{ 4, 6, 2, 7, 3 };

    auto vec = rl::zip( input1, input2 ).take( 3 ).to_vector();

    REQUIRE( vec.size() == 3 );
    REQUIRE( vec[0].first == "hello" );
    REQUIRE( vec[0].second == 4 );
    REQUIRE( vec[1].first == "world" );
    REQUIRE( vec[1].second == 6 );
    REQUIRE( vec[2].first == "one" );
    REQUIRE( vec[2].second == 2 );
  }
  SECTION( "free-zip: Views" ) {
    auto vec = rl::zip( rl::ints( 5 ), rl::ints( 7 ) )
                   .take( 3 )
                   .to_vector();
    static_assert(
        is_same_v<decltype( vec ), vector<pair<int, int>>> );

    auto expected =
        vector<pair<int, int>>{ { 5, 7 }, { 6, 8 }, { 7, 9 } };
    REQUIRE_THAT( vec, Equals( expected ) );
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

TEST_CASE( "[range-lite] intersperse" ) {
  SECTION( "empty" ) {
    vector<int> input{};

    auto vec = rl::all( input ).intersperse( 3 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{} ) );
  }
  SECTION( "one" ) {
    vector<int> input{ 1 };

    auto vec = rl::all( input ).intersperse( 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1 } ) );
  }
  SECTION( "two" ) {
    vector<int> input{ 1, 2 };

    auto vec = rl::all( input ).intersperse( 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 5, 2 } ) );
  }
  SECTION( "three" ) {
    vector<int> input{ 1, 2, 3 };

    auto vec = rl::all( input ).intersperse( 5 ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 5, 2, 5, 3 } ) );
  }
  SECTION( "many" ) {
    vector<string> input{ "1", "2", "3" };

    auto vec = rl::all( input ).intersperse( "5" ).to_vector();

    REQUIRE_THAT( vec, Equals( vector<string>{ "1", "5", "2",
                                               "5", "3" } ) );
  }
}

TEST_CASE( "[range-lite] dereference pointer" ) {
  SECTION( "just dereference" ) {
    vector<int>  v{ 1, 2, 3, 4, 5 };
    vector<int*> v_ptr{ &v[0], &v[1], &v[2], &v[3], &v[4] };

    auto view = rl::all( v_ptr ).dereference();
    static_assert( is_same_v<decltype( *view.begin() ), int&> );

    for( int& i : view ) i += 1;

    REQUIRE_THAT( v, Equals( vector<int>{ 2, 3, 4, 5, 6 } ) );
  }
  SECTION( "dereference ptr after map" ) {
    vector<int>  v{ 1, 2, 3, 4, 5 };
    vector<int*> v_ptr{ &v[0], &v[1], &v[2], &v[3], &v[4] };

    auto view = rl::all( v_ptr ).map_L( _ ).dereference();
    static_assert( is_same_v<decltype( *view.begin() ), int&> );

    for( int& i : view ) i += 1;

    REQUIRE_THAT( v, Equals( vector<int>{ 2, 3, 4, 5, 6 } ) );
  }
}

TEST_CASE( "[range-lite] dereference maybe" ) {
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

TEST_CASE( "[range-lite] distance" ) {
  vector<int> input0{};
  REQUIRE( rl::all( input0 ).distance() == 0 );
  vector<int> input1{ 1 };
  REQUIRE( rl::all( input1 ).distance() == 1 );
  vector<int> input2{ 1, 2, 3, 4 };
  REQUIRE( rl::all( input2 ).distance() == 4 );
}

TEST_CASE( "[range-lite] group_on" ) {
  SECTION( "IP" ) {
    std::string s = "123.212.323.498.hello.321";

    auto vec = rl::all( s )
                   .group_on_L( _ == '.' )
                   .remove_if_L( *_.begin() == '.' )
                   .map_L( _.to_string() )
                   .map( LIFT( base::stoi ) )
                   .cat_maybes()
                   .to_vector();

    REQUIRE_THAT(
        vec, Equals( vector<int>{ 123, 212, 323, 498, 321 } ) );
  }
  SECTION( "remove redundant spaces" ) {
    std::string s = "  too    many spaces      here ";

    auto s_normalized = rl::all( s )
                            .group_on_L( _ == ' ' )
                            .map_L( _.to_string() )
                            .map( base::trim )
                            .remove_if_L( _.empty() )
                            .intersperse( " " )
                            .accumulate();

    REQUIRE( s_normalized == "too many spaces here" );
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
