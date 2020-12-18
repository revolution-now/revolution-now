/****************************************************************
**ranges-heavy.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-12-18.
*
* Description: Unit tests for the ranges-v3 module.
*
*****************************************************************/
#if 0 // Disabled because compile time is 19 sec by itself.
#  include "testing.hpp"

// Under test.
#  include "range/v3/numeric/accumulate.hpp"
#  include "range/v3/range/conversion.hpp"
#  include "range/v3/view/concat.hpp"
#  include "range/v3/view/cycle.hpp"
#  include "range/v3/view/drop.hpp"
#  include "range/v3/view/drop_while.hpp"
#  include "range/v3/view/enumerate.hpp"
#  include "range/v3/view/filter.hpp"
#  include "range/v3/view/group_by.hpp"
#  include "range/v3/view/indirect.hpp"
#  include "range/v3/view/intersperse.hpp"
#  include "range/v3/view/iota.hpp"
#  include "range/v3/view/map.hpp"
#  include "range/v3/view/reverse.hpp"
#  include "range/v3/view/tail.hpp"
#  include "range/v3/view/take.hpp"
#  include "range/v3/view/take_while.hpp"
#  include "range/v3/view/transform.hpp"
#  include "range/v3/view/zip.hpp"

#  include "src/base/lambda.hpp"

#  include "src/base/lambda.hpp"

// Must be last.
#  include "catch-common.hpp"

namespace testing {

namespace {

namespace rv = ::ranges::views;
namespace rg = ::ranges;

using namespace std;

using ::Catch::Equals;

TEST_CASE( "[ranges-heavy] double traverse" ) {
  vector<int> input{ 1, 2, 3 };

  auto view = input | rv::cycle | rv::take( 4 );

  auto vec1 = rg::to<vector<int>>( view );
  REQUIRE_THAT( vec1, Equals( vector<int>{ 1, 2, 3, 1 } ) );

  auto vec2 = rg::to<vector<int>>( view );
  REQUIRE_THAT( rg::to<vector<int>>( vec2 ),
                Equals( vector<int>{ 1, 2, 3, 1 } ) );
}

TEST_CASE( "[ranges-heavy] non-materialized" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = input | rv::filter( L( _ % 2 == 1 ) ) |
             rv::transform( L( _ * _ ) ) |
             rv::transform( L( _ + 1 ) ) |
             rv::take_while( L( _ < 27 ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-heavy] materialized" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = input | rv::filter( L( _ % 2 == 1 ) ) |
             rv::transform( L( _ * _ ) ) |
             rv::transform( L( _ + 1 ) ) |
             rv::take_while( L( _ < 27 ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-heavy] lambdas with capture" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  int n = 2;

  auto vec = input | rv::filter( LC( _ % n == 1 ) ) |
             rv::transform( LC( _ * n ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 2, 6, 10, 14, 18 } ) );
}

TEST_CASE( "[ranges-heavy] rview" ) {
  vector<int> input{ 9, 8, 7, 6, 5, 4, 3, 2, 1 };

  auto vec =
      input | rv::reverse | rv::filter( L( _ % 2 == 1 ) ) |
      rv::transform( L( _ * _ ) ) | rv::transform( L( _ + 1 ) ) |
      rv::take_while( L( _ < 27 ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-heavy] macros" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = input | rv::filter( L( _ % 2 == 1 ) ) |
             rv::transform( L( _ * _ ) ) |
             rv::transform( L( _ + 1 ) ) |
             rv::take_while( L( _ < 27 ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 2, 10, 26 } ) );
}

TEST_CASE( "[ranges-heavy] head" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto res = input | rv::filter( L( _ % 2 == 1 ) ) |
             rv::transform( L( _ * _ ) ) |
             rv::transform( L( _ + 1 ) ) |
             rv::take_while( L( _ < 27 ) ) | rv::take( 1 );

  REQUIRE( *res.begin() == 2 );
}

TEST_CASE( "[ranges-heavy] remove" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto odd = L( _ % 2 == 1 );

  auto res = input | rv::filter( std::not_fn( odd ) );

  REQUIRE_THAT( rg::to<vector<int>>( res ),
                Equals( vector<int>{ 2, 4, 6, 8 } ) );
}

TEST_CASE( "[ranges-heavy] accumulate" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto res =
      rg::accumulate( input | rv::filter( L( _ % 2 == 1 ) ) |
                          rv::transform( L( _ * _ ) ) |
                          rv::transform( L( _ + 1 ) ),
                      0 );

  REQUIRE( res == 2 + 10 + 26 + 50 + 82 );

  auto res2 =
      rg::accumulate( input | rv::filter( L( _ % 2 == 1 ) ) |
                          rv::transform( L( _ * _ ) ) |
                          rv::transform( L( _ + 1 ) ),
                      1, std::multiplies{} );

  REQUIRE( res2 == 2 * 10 * 26 * 50 * 82 );

  auto res3 = rg::accumulate(
      input | rv::transform( L( to_string( _ ) ) ),
      string( "" ) );

  REQUIRE( res3 == "123456789" );
}

TEST_CASE( "[ranges-heavy] mixing" ) {
  vector<int> input{ 1, 22, 333, 4444, 55555, 666666, 7777777 };

  auto res = input | rv::filter( L( _ % 2 == 1 ) ) |
             rv::transform( L( to_string( _ ) ) ) |
             rv::transform( L( _.size() ) );

  REQUIRE_THAT( rg::to<vector<size_t>>( res ),
                Equals( vector<size_t>{ 1, 3, 5, 7 } ) );
}

TEST_CASE( "[ranges-heavy] zip" ) {
  SECTION( "int, int" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_odd = input | rv::filter( L( _ % 2 == 1 ) );

    auto vec = rv::zip( input | rv::filter( L( _ % 2 == 0 ) ),
                        view_odd ) |
               rv::take_while( L( _.second < 8 ) );

    auto expected = vector<pair<int, int>>{
        { 2, 1 },
        { 4, 3 },
        { 6, 5 },
        { 8, 7 },
    };
    REQUIRE_THAT( ( rg::to<vector<pair<int, int>>>( vec ) ),
                  Equals( expected ) );
  }
  SECTION( "int, string" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = input | rv::transform( L( to_string( _ ) ) );

    auto vec = rv::zip( input | rv::filter( L( _ % 2 == 0 ) ),
                        view_str );

    auto expected = vector<pair<int, string>>{
        { 2, "1" },
        { 4, "2" },
        { 6, "3" },
        { 8, "4" },
    };
    REQUIRE_THAT( ( rg::to<vector<pair<int, string>>>( vec ) ),
                  Equals( expected ) );
  }
  SECTION( "empty" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = input | rv::transform( L( to_string( _ ) ) );

    auto vec =
        rv::zip( input | rv::filter( L( _ > 200 ) ), view_str );

    auto expected = vector<pair<int, string>>{};
    REQUIRE_THAT( ( rg::to<vector<pair<int, string>>>( vec ) ),
                  Equals( expected ) );
  }
}

TEST_CASE( "[ranges-heavy] zip2" ) {
  SECTION( "int, int" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_odd = input | rv::filter( L( _ % 2 == 1 ) );

    auto vec = rv::zip( input | rv::filter( L( _ % 2 == 0 ) ),
                        view_odd ) |
               rv::take_while( L( _.second < 8 ) );

    auto expected = vector<pair<int, int>>{
        { 2, 1 },
        { 4, 3 },
        { 6, 5 },
        { 8, 7 },
    };
    REQUIRE_THAT( ( rg::to<vector<pair<int, int>>>( vec ) ),
                  Equals( expected ) );
  }
  SECTION( "int, string" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto view_str = input | rv::transform( L( to_string( _ ) ) );

    auto vec = rv::zip( input | rv::filter( L( _ % 2 == 0 ) ),
                        view_str );

    auto expected = vector<pair<int, string>>{
        { 2, "1" },
        { 4, "2" },
        { 6, "3" },
        { 8, "4" },
    };
    REQUIRE_THAT( ( rg::to<vector<pair<int, string>>>( vec ) ),
                  Equals( expected ) );
  }
}

TEST_CASE( "[ranges-heavy] take" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = input | rv::take_while( L( _ < 5 ) ) |
             rv::take( 2 ) | rv::transform( L( _ + 1 ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 2, 3 } ) );

  auto vec2 = input | rv::take_while( L( _ < 50 ) ) |
              rv::take( 0 ) | rv::transform( L( _ + 1 ) );

  REQUIRE_THAT( rg::to<vector<int>>( vec2 ),
                Equals( vector<int>{} ) );

  auto vec3 =
      input | rv::take_while( L( _ < 0 ) ) | rv::take( 5 );

  REQUIRE_THAT( rg::to<vector<int>>( vec3 ),
                Equals( vector<int>{} ) );
}

TEST_CASE( "[ranges-heavy] drop" ) {
  vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  auto vec = input | rv::take( 7 ) // 1,2,3,4,5,6,7
             | rv::drop( 2 )       // 3,4,5,6,7
             | rv::take( 3 );

  REQUIRE_THAT( rg::to<vector<int>>( vec ),
                Equals( vector<int>{ 3, 4, 5 } ) );

  auto vec2 = input | rv::take( 7 ) // 1,2,3,4,5,6,7
              | rv::drop( 0 )       // 1,2,3,4,5,6,7
              | rv::take( 3 )       // 1,2,3
      ;

  REQUIRE_THAT( rg::to<vector<int>>( vec2 ),
                Equals( vector<int>{ 1, 2, 3 } ) );

  auto vec3 = input | rv::take( 7 ) // 1,2,3,4,5,6,7
              | rv::drop( 7 )       // none
              | rv::take( 3 )       // none
      ;

  REQUIRE_THAT( rg::to<vector<int>>( vec3 ),
                Equals( vector<int>{} ) );

  auto vec4 = input | rv::take( 7 ) // 1,2,3,4,5,6,7
              | rv::drop( 10 )      // none
              | rv::take( 3 )       // none
      ;

  REQUIRE_THAT( rg::to<vector<int>>( vec4 ),
                Equals( vector<int>{} ) );

  auto vec5 = input | rv::take( 0 ) // 1,2,3,4,5,6,7
              | rv::drop( 2 )       // none
      ;

  REQUIRE_THAT( rg::to<vector<int>>( vec5 ),
                Equals( vector<int>{} ) );
}

TEST_CASE( "[ranges-heavy] cycle" ) {
  SECTION( "basic" ) {
    vector<int> input{ 1, 2, 3 };

    auto vec = input | rv::cycle | rv::take( 10 );

    REQUIRE_THAT(
        rg::to<vector<int>>( vec ),
        Equals( vector<int>{ 1, 2, 3, 1, 2, 3, 1, 2, 3, 1 } ) );
  }
  SECTION( "single" ) {
    vector<int> input{ 1 };

    auto vec = input | rv::cycle | rv::take( 5 ) // 1,1,1,1,1
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 1, 1, 1, 1, 1 } ) );
  }
  SECTION( "double" ) {
    vector<int> input{ 1, 2 };

    auto vec = input | rv::cycle | rv::take( 5 ) // 1,2,1,2,1
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 1, 2, 1, 2, 1 } ) );
  }
  SECTION( "with stateful function" ) {
    vector<int> input{ 1, 2 };

    int                  n = 3;
    function<int( int )> f = LC( _ + n );

    auto vec = input | rv::cycle // 1, 2, 1, 2...
               | rv::take( 3 )   // 1, 2, 1
               | rv::transform( std::move( f ) ) | rv::cycle |
               rv::take( 7 ) // 4,
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 4, 5, 4, 4, 5, 4, 4 } ) );
  }
  SECTION( "with decayed lambda" ) {
    vector<int> input{ 1, 2 };

    auto* f = +[]( int n ) { return n + 3; };

    auto vec = input | rv::cycle    // 1, 2, 1, 2...
               | rv::take( 3 )      // 1, 2, 1
               | rv::transform( f ) // 4, 5, 4
               | rv::cycle          // 4, 5, 4, 4, 5, 4...
               | rv::take( 7 )      // 4, 5, 4, 4, 5, 4, 4
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 4, 5, 4, 4, 5, 4, 4 } ) );
  }
}

TEST_CASE( "[ranges-heavy] ints" ) {
  SECTION( "0, ..." ) {
    auto vec = rv::ints // 0, 1, 2, 3, 4...
               | rv::take( 10 );

    REQUIRE_THAT(
        rg::to<vector<int>>( vec ),
        Equals( vector<int>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 } ) );
  }
  SECTION( "-5, ..." ) {
    auto vec = rv::ints( -5, rg::unreachable ) | rv::take( 10 );

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ -5, -4, -3, -2, -1, 0, 1,
                                       2, 3, 4 } ) );
  }
  SECTION( "8, 20" ) {
    auto vec = rv::ints( 8, 20 );

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 8, 9, 10, 11, 12, 13, 14,
                                       15, 16, 17, 18, 19 } ) );
  }
}

TEST_CASE( "[ranges-heavy] enumerate" ) {
  vector<string> input{ "hello", "world", "one", "two" };

  auto vec = input | rv::cycle | rv::enumerate | rv::take( 8 );

  auto expected = vector<pair<int, string>>{
      { 0, "hello" }, { 1, "world" }, { 2, "one" }, { 3, "two" },
      { 4, "hello" }, { 5, "world" }, { 6, "one" }, { 7, "two" },
  };
  REQUIRE_THAT( ( rg::to<vector<pair<int, string>>>( vec ) ),
                Equals( expected ) );
}

TEST_CASE( "[ranges-heavy] free-standing zip" ) {
  SECTION( "vectors" ) {
    vector<string> input1{ "hello", "world", "one", "two" };
    vector<int>    input2{ 4, 6, 2, 7, 3 };

    auto view = rv::zip( input1, input2 ) | rv::take( 3 );

    auto expected = vector<pair<string, int>>{
        { "hello", 4 }, { "world", 6 }, { "one", 2 } };
    REQUIRE_THAT( ( rg::to<vector<pair<string, int>>>( view ) ),
                  Equals( expected ) );
  }
  SECTION( "Views" ) {
    auto view = rv::zip( rv::ints( 5, rg::unreachable ),
                         rv::ints( 7, rg::unreachable ) ) |
                rv::take( 3 );

    auto expected =
        vector<pair<int, int>>{ { 5, 7 }, { 6, 8 }, { 7, 9 } };
    REQUIRE_THAT( ( rg::to<vector<pair<int, int>>>( view ) ),
                  Equals( expected ) );
  }
}

TEST_CASE( "[ranges-heavy] mutation" ) {
  vector<int> input{ 1, 2, 3, 4, 5 };

  auto view = input | rv::cycle | rv::drop( 2 ) | rv::take( 4 );

  for( int& i : view ) i *= 10;

  auto expected = vector<int>{ 10, 2, 30, 40, 50 };
  REQUIRE_THAT( rg::to<vector<int>>( input ),
                Equals( expected ) );
}

TEST_CASE( "[ranges-heavy] keys" ) {
  vector<pair<string, int>> input{
      { "hello", 3 },
      { "world", 2 },
      { "again", 1 },
  };

  auto vec = input | rv::cycle | rv::keys | rv::take( 6 );

  vector<string> expected{
      { "hello" }, { "world" }, { "again" },
      { "hello" }, { "world" }, { "again" },
  };

  REQUIRE_THAT( rg::to<vector<string>>( vec ),
                Equals( expected ) );
}

TEST_CASE( "[ranges-heavy] lense" ) {
  vector<string> input{ "hello", "world", "again" };

  auto view = input | rv::transform(
                          []( string& s ) -> decltype( auto ) {
                            return s.data()[1];
                          } );

  for( auto& c : view ) ++c;

  vector<string> expected{ "hfllo", "wprld", "ahain" };

  REQUIRE_THAT( rg::to<vector<string>>( input ),
                Equals( expected ) );
}

TEST_CASE( "[ranges-heavy] keys mutation" ) {
  vector<pair<string, int>> input{
      { "hello", 3 },
      { "world", 2 },
      { "again", 1 },
  };

  auto view = input | rv::cycle | rv::keys | rv::drop( 1 ) |
              rv::take( 2 );

  for( auto& key : view ) key = key + key;

  vector<pair<string, int>> expected{
      { "hello", 3 },
      { "worldworld", 2 },
      { "againagain", 1 },
  };

  REQUIRE_THAT( ( rg::to<vector<pair<string, int>>>( input ) ),
                Equals( expected ) );
}

TEST_CASE( "[ranges-heavy] take_while" ) {
  SECTION( "empty" ) {
    vector<int> input{};

    auto vec = input | rv::take_while( L( _ < 5 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{} ) );
  }
  SECTION( "take all" ) {
    vector<int> input{ 1, 2, 3, 4 };

    auto vec = input | rv::take_while( L( _ < 5 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 1, 2, 3, 4 } ) );
  }
  SECTION( "take none" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec = input | rv::take_while( L( _ < 0 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{} ) );
  }
  SECTION( "take some" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec = input | rv::take_while( L( _ < 5 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 1, 2, 3, 4 } ) );
  }
}

TEST_CASE( "[ranges-heavy] drop_while" ) {
  SECTION( "empty" ) {
    vector<int> input{};

    auto vec = input | rv::drop_while( L( _ < 5 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{} ) );
  }
  SECTION( "drop all" ) {
    vector<int> input{ 1, 2, 3, 4 };

    auto vec = input | rv::drop_while( L( _ < 5 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{} ) );
  }
  SECTION( "drop none" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec = input | rv::drop_while( L( _ < 0 ) ) // 2,10,26
        ;

    REQUIRE_THAT(
        rg::to<vector<int>>( vec ),
        Equals( vector<int>{ 1, 2, 3, 4, 5, 6, 7, 8, 9 } ) );
  }
  SECTION( "drop some" ) {
    vector<int> input{ 1, 2, 3, 4, 5, 6, 7, 8, 9 };

    auto vec = input | rv::drop_while( L( _ < 5 ) ) // 2,10,26
        ;

    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 5, 6, 7, 8, 9 } ) );
  }
}

TEST_CASE( "[ranges-heavy] dereference" ) {
  int          n1 = 1;
  int          n2 = 2;
  int          n3 = 3;
  int          n4 = 4;
  int          n5 = 5;
  vector<int*> input{ &n1, &n2, &n3, &n4, &n5 };

  auto view = input | rv::indirect;

  for( int& i : view ) i *= 10;

  REQUIRE_THAT( rg::to<vector<int>>( view ),
                Equals( vector<int>{ 10, 20, 30, 40, 50 } ) );
}

TEST_CASE( "[ranges-heavy] cat_maybes" ) {
  vector<std::optional<int>> input{
      { 1 }, nullopt, { 3 }, nullopt, { 5 } };

  auto view = input | rv::filter( L( _ != nullopt ) );

  bool equal = rg::to<vector<std::optional<int>>>( view ) ==
               vector<std::optional<int>>{ { 1 }, { 3 }, { 5 } };
  REQUIRE( equal );
}

TEST_CASE( "[ranges-heavy] tail" ) {
  SECTION( "some" ) {
    vector<int> input{ 1, 2, 3, 4 };
    auto        vec = input | rv::tail;
    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{ 2, 3, 4 } ) );
  }
  SECTION( "empty" ) {
    vector<int> input{};
    auto        vec = input | rv::tail;
    REQUIRE_THAT( rg::to<vector<int>>( vec ),
                  Equals( vector<int>{} ) );
  }
}

TEST_CASE( "[ranges-heavy] group_by" ) {
  SECTION( "empty" ) {
    vector<int> input{};

    auto view = input | rv::group_by( L2( _1 == _2 ) );

    auto it = view.begin();
    REQUIRE( it == view.end() );
  }
  SECTION( "single" ) {
    vector<int> input{ 1 };

    auto view = input | rv::group_by( L2( _1 == _2 ) );

    auto it = view.begin();
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 1 } ) );

    ++it;
    REQUIRE( it == view.end() );
  }
  SECTION( "many" ) {
    vector<int> input{ 1, 2, 2, 2, 3, 3, 4, 5, 5, 5, 5, 6 };

    auto view = input | rv::group_by( L2( _1 == _2 ) );

    auto it = view.begin();
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 1 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 2, 2, 2 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 3, 3 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 4 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 5, 5, 5, 5 } ) );

    ++it;
    REQUIRE( it != view.end() );
    REQUIRE_THAT( rg::to<vector<int>>( *it ),
                  Equals( vector<int>{ 6 } ) );

    ++it;
    REQUIRE( it == view.end() );
  }
}

TEST_CASE( "[ranges-heavy] group_by complicated" ) {
  vector<string> input{ "hello", "world", "four", "seven",
                        "six",   "two",   "three" };

  auto view = input | rv::cycle |
              rv::group_by( L2( _1.size() == _2.size() ) ) |
              rv::take( 6 );

  vector<vector<string>> expected{ { "hello", "world" },
                                   { "four" },
                                   { "seven" },
                                   { "six", "two" },
                                   { "three", "hello", "world" },
                                   { "four" } };

  for( auto [l, r] : rv::zip( view, expected ) ) {
    REQUIRE_THAT( rg::to<vector<string>>( l ), Equals( r ) );
  }
}

} // namespace
} // namespace testing
#endif
