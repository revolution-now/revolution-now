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
  return rl::view_temporary( vector<int>{} )
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

} // namespace
} // namespace base
