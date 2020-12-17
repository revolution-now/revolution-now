/****************************************************************
**ranges-lite.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-16.
*
* Description: Unit tests for the src/base/ranges-lite.* module.
*
*****************************************************************/
#include "testing.hpp"

// Under test.
#include "src/base/ranges-lite.hpp"

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
  return rl::view( vector<int>{} )
      .keep( L( _ % 2 == 1 ) )
      .map( L( _ * _ ) )
      .map( L( _ + 1 ) )
      .take_while( L( _ < 27 ) );
}

namespace {

using ::Catch::Equals;

TEST_CASE( "[ranges-lite] non-materialized" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )
                 .keep( L( _ % 2 == 1 ) )
                 .map( L( _ * _ ) )
                 .map( L( _ + 1 ) )
                 .take_while( L( _ < 27 ) );

  static_assert( sizeof( decltype( vec ) ) == 88 );
}

TEST_CASE( "[ranges-lite] materialized" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )
                 .keep( L( _ % 2 == 1 ) )   // 1,3,5,7,9
                 .map( L( _ * _ ) )         // 1,9,25,49,81
                 .map( L( _ + 1 ) )         // 2,10,26,50,82
                 .take_while( L( _ < 27 ) ) // 2,10,26
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-lite] lambdas with capture" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  int n = 2;

  auto vec = rl::view( input )
                 .keep( LC( _ % n == 1 ) ) // 1,3,5,7,9
                 .map( LC( _ * n ) )       // 2,6,10,14,18
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 6, 10, 14, 18 } ) );
}

#ifdef __clang__ // C++20
TEST_CASE( "[ranges-lite] static create" ) {
  using type = decltype( GetCompoundViewType() );
  static_assert( sizeof( type ) == 88 );

  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };
  auto        view = type::create( input );
  static_assert( sizeof( decltype( view ) ) == 88 );

  REQUIRE_THAT( view.to_vector(),
                Equals( vector<int>{ 2, 10, 26 } ) );
}
#endif

TEST_CASE( "[ranges-lite] rview" ) {
  vector<int> input{ 9, 8, 7, 6, 5, 4, 3, 2, 1 };

  auto vec = rl::rview( input )
                 .keep( L( _ % 2 == 1 ) )   // 1,3,5,7,9
                 .map( L( _ * _ ) )         // 1,9,25,49,81
                 .map( L( _ + 1 ) )         // 2,10,26,50,82
                 .take_while( L( _ < 27 ) ) // 2,10,26
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-lite] macros" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )
                 .rl_keep( _ % 2 == 1 )   // 1,3,5,7,9
                 .rl_map( _ * _ )         // 1,9,25,49,81
                 .rl_map( _ + 1 )         // 2,10,26,50,82
                 .rl_take_while( _ < 27 ) // 2,10,26
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-lite] head" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto res = rl::view( input )
                 .keep( L( _ % 2 == 1 ) )   // 1,3,5,7,9
                 .map( L( _ * _ ) )         // 1,9,25,49,81
                 .map( L( _ + 1 ) )         // 2,10,26,50,82
                 .take_while( L( _ < 27 ) ) // 2,10,26
                 .head();

  REQUIRE( res == 2 );

  auto res2 = rl::view( input ).rl_take_while( _ > 100 ).head();

  REQUIRE( res2 == nothing );
}

TEST_CASE( "[ranges-lite] remove" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto odd = L( _ % 2 == 1 );

  auto res = rl::view( input ).remove( odd ).to_vector();

  REQUIRE_THAT( res, Equals( vector<int>{ 2, 4, 6, 8 } ) );
}

TEST_CASE( "[ranges-lite] to" ) {
  string msg = "1hello 123 with 345 numbers44";

  auto is_num = L( _ >= '0' && _ <= '9' );

  auto res = rl::view( msg ).remove( is_num ).to<string>();

  REQUIRE( res == "hello  with  numbers" );
}

TEST_CASE( "[ranges-lite] accumulate" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto res = rl::view( input )
                 .keep( L( _ % 2 == 1 ) ) // 1,3,5,7,9
                 .map( L( _ * _ ) )       // 1,9,25,49,81
                 .map( L( _ + 1 ) )       // 2,10,26,50,82
                 .accumulate();

  REQUIRE( res == 2 + 10 + 26 + 50 + 82 );

  auto res2 = rl::view( input )
                  .keep( L( _ % 2 == 1 ) ) // 1,3,5,7,9
                  .map( L( _ * _ ) )       // 1,9,25,49,81
                  .map( L( _ + 1 ) )       // 2,10,26,50,82
                  .accumulate( std::multiplies{}, 1 );

  REQUIRE( res2 == 2 * 10 * 26 * 50 * 82 );

  auto res3 =
      rl::view( input ).rl_map( to_string( _ ) ).accumulate();

  REQUIRE( res2 == 2 * 10 * 26 * 50 * 82 );
}

TEST_CASE( "[ranges-lite] mixing" ) {
  vector<int> input{ 1, 22, 333, 4444, 55555, 666666, 7777777 };

  auto res = rl::view( input )
                 .keep( L( _ % 2 == 1 ) )
                 .rl_map( to_string( _ ) )
                 .rl_map( _.size() )
                 .to_vector();

  REQUIRE_THAT( res, Equals( vector<size_t>{ 1, 3, 5, 7 } ) );
}

TEST_CASE( "[ranges-lite] zip" ) {
  SECTION( "int, int" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_odd = rl::view( input ).keep( L( _ % 2 == 1 ) );

    auto vec = rl::view( input )
                   .keep( L( _ % 2 == 0 ) )
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
  SECTION( "int, string" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = rl::view( input ).map( L( to_string( _ ) ) );

    auto vec = rl::view( input )
                   .keep( L( _ % 2 == 0 ) )
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
  SECTION( "empty" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = rl::view( input ).map( L( to_string( _ ) ) );

    auto vec = rl::view( input )
                   .keep( L( _ > 200 ) )
                   .zip( view_str )
                   .to_vector();

    auto expected = vector<pair<int, string>>{};
    REQUIRE_THAT( vec, Equals( expected ) );
  }
}

TEST_CASE( "[ranges-lite] take_while_incl" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )
                 .rl_take_while_incl( _ < 5 ) // 1,2,3,4,5
                 .rl_map( _ + 1 )             // 2,3,4,5,6
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 3, 4, 5, 6 } ) );

  auto vec2 = rl::view( input )
                  .rl_take_while_incl( _ < 50 ) // all nums
                  .rl_map( _ + 1 )
                  .to_vector();

  REQUIRE_THAT( vec2, Equals( vector<int>{ 2, 3, 4, 5, 6, 7, 8,
                                           9, 10 } ) );

  auto vec3 = rl::view( input )
                  .rl_take_while_incl( _ < 0 ) // 1
                  .to_vector();

  REQUIRE_THAT( vec3, Equals( vector<int>{ 1 } ) );
}

TEST_CASE( "[ranges-lite] take" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )
                 .rl_take_while_incl( _ < 5 ) // 1,2,3,4,5
                 .take( 2 )                   // 1,2
                 .rl_map( _ + 1 )             // 2,3
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 2, 3 } ) );

  auto vec2 = rl::view( input )
                  .rl_take_while_incl( _ < 50 ) // all nums
                  .take( 0 )
                  .rl_map( _ + 1 )
                  .to_vector();

  REQUIRE_THAT( vec2, Equals( vector<int>{} ) );

  auto vec3 = rl::view( input )
                  .rl_take_while( _ < 0 ) // none
                  .take( 5 )
                  .to_vector();

  REQUIRE_THAT( vec3, Equals( vector<int>{} ) );
}

TEST_CASE( "[ranges-lite] drop" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = rl::view( input )
                 .take( 7 ) // 1,2,3,4,5,6,7
                 .drop( 2 ) // 3,4,5,6,7
                 .take( 3 ) // 3,4,5
                 .to_vector();

  REQUIRE_THAT( vec, Equals( vector<int>{ 3, 4, 5 } ) );

  auto vec2 = rl::view( input )
                  .take( 7 ) // 1,2,3,4,5,6,7
                  .drop( 0 ) // 1,2,3,4,5,6,7
                  .take( 3 ) // 1,2,3
                  .to_vector();

  REQUIRE_THAT( vec2, Equals( vector<int>{ 1, 2, 3 } ) );

  auto vec3 = rl::view( input )
                  .take( 7 ) // 1,2,3,4,5,6,7
                  .drop( 7 ) // none
                  .take( 3 ) // none
                  .to_vector();

  REQUIRE_THAT( vec3, Equals( vector<int>{} ) );

  auto vec4 = rl::view( input )
                  .take( 7 )  // 1,2,3,4,5,6,7
                  .drop( 10 ) // none
                  .take( 3 )  // none
                  .to_vector();

  REQUIRE_THAT( vec4, Equals( vector<int>{} ) );

  auto vec5 = rl::view( input )
                  .take( 0 ) // 1,2,3,4,5,6,7
                  .drop( 2 ) // none
                  .to_vector();

  REQUIRE_THAT( vec5, Equals( vector<int>{} ) );
}

TEST_CASE( "[ranges-lite] cycle" ) {
  SECTION( "basic" ) {
    vector<int> input{ 1, 2, 3 };

    auto vec = rl::view( input )
                   .cycle()
                   .take( 10 ) // 1,2,3,1,2,3,1,2,3,1
                   .to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 3, 1, 2, 3, 1,
                                            2, 3, 1 } ) );
  }
  SECTION( "single" ) {
    vector<int> input{ 1 };

    auto vec = rl::view( input )
                   .cycle()
                   .take( 5 ) // 1,1,1,1,1
                   .to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 1, 1, 1, 1 } ) );
  }
  SECTION( "double" ) {
    vector<int> input{ 1, 2 };

    auto vec = rl::view( input )
                   .cycle()
                   .take( 5 ) // 1,2,1,2,1
                   .to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 1, 2, 1, 2, 1 } ) );
  }
  SECTION( "with stateful function" ) {
    vector<int> input{ 1, 2 };

    int n = 3;
    // Need to use a std::function because lambdas are no copy-
    // able, and the cycle needs to keep a copy.
    function<int( int )> f = LC( _ + n );

    auto vec = rl::view( input )
                   .cycle()               // 1, 2, 1, 2...
                   .take( 3 )             // 1, 2, 1
                   .map( std::move( f ) ) // 4, 5, 4
                   .cycle()               // 4, 5, 4, 4, 5, 4...
                   .take( 7 )             // 4, 5, 4, 4, 5, 4, 4
                   .to_vector();

    REQUIRE_THAT( vec,
                  Equals( vector<int>{ 4, 5, 4, 4, 5, 4, 4 } ) );
  }
  SECTION( "with decayed lambda" ) {
    vector<int> input{ 1, 2 };

    auto* f = +[]( int n ) { return n + 3; };

    auto vec = rl::view( input )
                   .cycle()   // 1, 2, 1, 2...
                   .take( 3 ) // 1, 2, 1
                   .map( f )  // 4, 5, 4
                   .cycle()   // 4, 5, 4, 4, 5, 4...
                   .take( 7 ) // 4, 5, 4, 4, 5, 4, 4
                   .to_vector();

    REQUIRE_THAT( vec,
                  Equals( vector<int>{ 4, 5, 4, 4, 5, 4, 4 } ) );
  }
}

TEST_CASE( "[ranges-lite] ints" ) {
  SECTION( "0, ..." ) {
    auto vec = rl::ints() // 0, 1, 2, 3, 4...
                   .take( 10 )
                   .to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ 0, 1, 2, 3, 4, 5, 6,
                                            7, 8, 9 } ) );
  }
  SECTION( "-5, ..." ) {
    auto vec = rl::ints( -5 ) // -5, -4, -3 ...
                   .take( 10 )
                   .to_vector();

    REQUIRE_THAT( vec, Equals( vector<int>{ -5, -4, -3, -2, -1,
                                            0, 1, 2, 3, 4 } ) );
  }
  SECTION( "8, 20" ) {
    auto vec = rl::ints( 8, 20 ).to_vector();

    REQUIRE_THAT(
        vec, Equals( vector<int>{ 8, 9, 10, 11, 12, 13, 14, 15,
                                  16, 17, 18, 19 } ) );
  }
}

TEST_CASE( "[ranges-lite] enumerate" ) {
  vector<string> input{ "hello", "world", "one", "two" };

  auto vec = rl::view( input )
                 .cycle()
                 .enumerate()
                 .take( 8 )
                 .to_vector();

  auto expected = vector<pair<int, string>>{
      { 0, "hello" }, { 1, "world" }, { 2, "one" }, { 3, "two" },
      { 4, "hello" }, { 5, "world" }, { 6, "one" }, { 7, "two" },
  };
  REQUIRE_THAT( vec, Equals( expected ) );
}

TEST_CASE( "[ranges-lite] free-standing zip" ) {
  SECTION( "vectors" ) {
    vector<string> input1{ "hello", "world", "one", "two" };
    vector<int>    input2{ 4, 6, 2, 7, 3 };

    auto view = rl::zip( input1, input2 ).take( 3 );

    auto expected = vector<pair<string, int>>{
        { "hello", 4 }, { "world", 6 }, { "one", 2 } };
    REQUIRE_THAT( view.to_vector(), Equals( expected ) );
  }
  SECTION( "Views" ) {
    auto view =
        rl::zip( rl::ints( 5 ), rl::ints( 7 ) ).take( 3 );

    auto expected =
        vector<pair<int, int>>{ { 5, 7 }, { 6, 8 }, { 7, 9 } };
    REQUIRE_THAT( view.to_vector(), Equals( expected ) );
  }
}

} // namespace
} // namespace base
