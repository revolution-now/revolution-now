/****************************************************************
**coord.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2019-08-23.
*
* Description: Unit tests for the coord module.
*
*****************************************************************/
#include "testing.hpp"

// Revolution Now
#include "coord.hpp"

// Rcl
#include "rcl/model.hpp"

// Must be last.
#include "catch-common.hpp"

FMT_TO_CATCH( ::rn::Coord );
FMT_TO_CATCH( ::rn::Rect );
FMT_TO_CATCH( ::rn::Delta );
FMT_TO_CATCH( ::rn::Scale );
FMT_TO_CATCH( ::rcl::error );

namespace {

using namespace std;
using namespace rn;

TEST_CASE( "[coord] centered*" ) {
  Rect  rect;
  Delta delta;
  Coord expect;

  rect   = Rect{ 1_x, 1_y, 0_w, 0_h };
  delta  = Delta{ 4_w, 3_h };
  expect = Coord{ -1_x, 0_y };
  REQUIRE( centered( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 3_y };
  REQUIRE( centered( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 4_y };
  REQUIRE( centered_bottom( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 0_y };
  REQUIRE( centered_top( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 5_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 0_x, 3_y };
  REQUIRE( centered_left( delta, rect ) == expect );

  rect   = Rect{ 1_x, 2_y, 4_w, 6_h };
  delta  = Delta{ 3_w, 4_h };
  expect = Coord{ 2_x, 3_y };
  REQUIRE( centered_right( delta, rect ) == expect );
}

TEST_CASE( "[coord] centered_on" ) {
  Coord coord;
  Rect  rect;
  Rect  expect;

  // Zero size.
  rect   = Rect{ 0_x, 0_y, 0_w, 0_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 10_x, 10_y, 0_w, 0_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ 0_x, 0_y, 0_w, 0_h };
  coord  = Coord{ -1_x, -2_y };
  expect = Rect{ -1_x, -2_y, 0_w, 0_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  // Small size.
  rect   = Rect{ 5_x, 5_y, 1_w, 2_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 10_x, 9_y, 1_w, 2_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ 5_x, 5_y, 2_w, 1_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 9_x, 10_y, 2_w, 1_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  // Large size.
  rect   = Rect{ 5_x, 5_y, 5_w, 5_h };
  coord  = Coord{ 10_x, 10_y };
  expect = Rect{ 8_x, 8_y, 5_w, 5_h };
  REQUIRE( rect.centered_on( coord ) == expect );

  rect   = Rect{ 100_x, 5_y, 300_w, 150_h };
  coord  = Coord{ -8_x, -2_y };
  expect = Rect{ -158_x, -77_y, 300_w, 150_h };
  REQUIRE( rect.centered_on( coord ) == expect );
}

TEST_CASE( "[coord] as_if_origin_were" ) {
  auto rect   = Rect{ 5_x, 5_y, 7_w, 9_h };
  auto coord  = Coord{ 10_x, 10_y };
  auto expect = Rect{ 15_x, 15_y, 7_w, 9_h };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );

  rect   = Rect{ 2_x, 3_y, 7_w, 9_h };
  coord  = Coord{ 1_x, 2_y };
  expect = Rect{ 3_x, 5_y, 7_w, 9_h };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );

  rect   = Rect{ 2_x, 3_y, 1_w, 1_h };
  coord  = Coord{ -1_x, -2_y };
  expect = Rect{ 1_x, 1_y, 1_w, 1_h };
  REQUIRE( rect.as_if_origin_were( coord ) == expect );
}

TEST_CASE( "[coord] Rect::normalized" ) {
  Rect rect, expect;

  rect   = Rect{ 5_x, 5_y, 0_w, 0_h };
  expect = rect;
  REQUIRE( rect.normalized() == expect );

  rect   = Rect{ 5_x, 5_y, 7_w, 9_h };
  expect = rect;
  REQUIRE( rect.normalized() == expect );

  rect   = Rect{ 5_x, 5_y, -2_w, -1_h };
  expect = Rect{ 3_x, 4_y, 2_w, 1_h };
  REQUIRE( rect.normalized() == expect );
}

TEST_CASE( "[coord] rounded_to_multiple_to_minus_inf" ) {
  Delta delta;
  Coord coord;
  Coord expect;

  delta  = Delta{ 0_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 0_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 10_x, 8_y };
  expect = Coord{ 10_x, 8_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ -10_x, -8_y };
  expect = Coord{ -10_x, -8_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 3_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 15_y };
  expect = Coord{ 0_x, 10_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 20_x, 20_y };
  expect = Coord{ 20_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 23_x, 29_y };
  expect = Coord{ 20_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, 29_y };
  expect = Coord{ -10_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, -29_y };
  expect = Coord{ -10_x, -30_y };
  REQUIRE( coord.rounded_to_multiple_to_minus_inf( delta ) ==
           expect );
}

TEST_CASE( "[coord] rounded_to_multiple_to_plus_inf" ) {
  Delta delta;
  Coord coord;
  Coord expect;

  delta  = Delta{ 0_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 0_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ 10_x, 8_y };
  expect = Coord{ 10_x, 8_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 1_w, 1_h };
  coord  = Coord{ -10_x, -8_y };
  expect = Coord{ -10_x, -8_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 0_x, 0_y };
  expect = Coord{ 0_x, 0_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 3_y };
  expect = Coord{ 10_x, 10_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 5_x, 15_y };
  expect = Coord{ 10_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 20_x, 20_y };
  expect = Coord{ 20_x, 20_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ 23_x, 29_y };
  expect = Coord{ 30_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, 29_y };
  expect = Coord{ 0_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -7_x, 29_y };
  expect = Coord{ 0_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -2_x, 29_y };
  expect = Coord{ 0_x, 30_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );

  delta  = Delta{ 10_w, 10_h };
  coord  = Coord{ -5_x, -29_y };
  expect = Coord{ 0_x, -20_y };
  REQUIRE( coord.rounded_to_multiple_to_plus_inf( delta ) ==
           expect );
}

TEST_CASE( "[coord] Coord - rcl" ) {
  using namespace rcl;
  using KV = table::value_type;

  SECTION( "success" ) {
    UNWRAP_CHECK( tbl, run_postprocessing( make_table(
                           KV{ "x", 3 }, KV{ "y", 4 } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE( convert_to<Coord>( v ) == Coord{ 3_x, 4_y } );
  }
  SECTION( "failure 1" ) {
    UNWRAP_CHECK( tbl, run_postprocessing( make_table(
                           KV{ "x", 3 }, KV{ "z", 4 } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE( convert_to<Coord>( v ) ==
             error( "table must have a 'x' and 'y' field for "
                    "conversion to Coord." ) );
  }
  SECTION( "failure 2" ) {
    UNWRAP_CHECK(
        tbl, run_postprocessing( make_table( KV{ "x", "hello" },
                                             KV{ "y", 4 } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE(
        convert_to<Coord>( v ) ==
        error( "cannot produce a ::rn::X from type string." ) );
  }
}

TEST_CASE( "[coord] Delta - rcl" ) {
  using namespace rcl;
  using KV = table::value_type;

  SECTION( "success" ) {
    UNWRAP_CHECK( tbl, run_postprocessing( make_table(
                           KV{ "w", 3 }, KV{ "h", 4 } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE( convert_to<Delta>( v ) == Delta{ 3_w, 4_h } );
  }
  SECTION( "failure 1" ) {
    UNWRAP_CHECK( tbl, run_postprocessing( make_table(
                           KV{ "w", 3 }, KV{ "z", 4 } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE( convert_to<Delta>( v ) ==
             error( "table must have a 'w' and 'h' field for "
                    "conversion to Delta." ) );
  }
  SECTION( "failure 2" ) {
    UNWRAP_CHECK(
        tbl, run_postprocessing( make_table( KV{ "w", "hello" },
                                             KV{ "h", 4 } ) ) );
    value v{ std::make_unique<table>( std::move( tbl ) ) };

    // Test.
    REQUIRE(
        convert_to<Delta>( v ) ==
        error( "cannot produce a ::rn::W from type string." ) );
  }
}

} // namespace
