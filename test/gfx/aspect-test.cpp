/****************************************************************
**aspect-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-09.
*
* Description: Unit tests for the gfx/aspect module.
*
*****************************************************************/
#include "aspect.rds.hpp"
#include "fmt/format.h"
#include "test/testing.hpp"

// Under test.
#include "src/gfx/aspect.hpp"

// refl
#include "src/refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace gfx {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/aspect] find_closest_aspect_ratio" ) {
  using enum e_named_aspect_ratio;
  size const resolution_0 = { .w = 1'024, .h = 700 };
  size const resolution_1 = { .w = 1'024, .h = 807 };
  size const resolution_2 = { .w = 1'024, .h = 806 };
  size const resolution_3 = { .w = 1'024, .h = 1'024 };
  size const resolution_4 = { .w = 1'024, .h = 577 };
  size const resolution_5 = { .w = 1'024, .h = 305 };

  auto const actual_ratio_0 =
      AspectRatio::from_size( resolution_0 );
  auto const actual_ratio_1 =
      AspectRatio::from_size( resolution_1 );
  auto const actual_ratio_2 =
      AspectRatio::from_size( resolution_2 );
  auto const actual_ratio_3 =
      AspectRatio::from_size( resolution_3 );
  auto const actual_ratio_4 =
      AspectRatio::from_size( resolution_4 );
  auto const actual_ratio_5 =
      AspectRatio::from_size( resolution_5 );
  REQUIRE( actual_ratio_0.has_value() );
  REQUIRE( actual_ratio_1.has_value() );
  REQUIRE( actual_ratio_2.has_value() );
  REQUIRE( actual_ratio_3.has_value() );
  REQUIRE( actual_ratio_4.has_value() );
  REQUIRE( actual_ratio_5.has_value() );

  auto const all_aspect_ratios = {
    AspectRatio::from_named( w_h_16_9 ),
    AspectRatio::from_named( w_h_16_10 ),
    AspectRatio::from_named( w_h_4_3 ),
    AspectRatio::from_named( w_h_1_1 ),
  };

  auto const snapped_ratio_0 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_0 );
  auto const snapped_ratio_1 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_1 );
  auto const snapped_ratio_2 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_2 );
  auto const snapped_ratio_3 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_3 );
  auto const snapped_ratio_4 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_4 );
  auto const snapped_ratio_5 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_5 );

  REQUIRE( snapped_ratio_0 == nothing );
  REQUIRE( snapped_ratio_1 ==
           AspectRatio::from_named( w_h_4_3 ) );
  REQUIRE( snapped_ratio_2 ==
           AspectRatio::from_named( w_h_4_3 ) );
  REQUIRE( snapped_ratio_3 ==
           AspectRatio::from_named( w_h_1_1 ) );
  REQUIRE( snapped_ratio_4 ==
           AspectRatio::from_named( w_h_16_9 ) );
  REQUIRE( snapped_ratio_5 == nothing );
}

TEST_CASE( "[gfx/aspect] find_closest_named_aspect_ratio" ) {
  using enum e_named_aspect_ratio;
  size const resolution_0 = { .w = 1'024, .h = 700 };
  size const resolution_1 = { .w = 1'024, .h = 807 };
  size const resolution_2 = { .w = 1'024, .h = 806 };
  size const resolution_3 = { .w = 1'024, .h = 1'024 };
  size const resolution_4 = { .w = 1'024, .h = 577 };
  size const resolution_5 = { .w = 1'024, .h = 305 };

  auto const actual_ratio_0 =
      AspectRatio::from_size( resolution_0 );
  auto const actual_ratio_1 =
      AspectRatio::from_size( resolution_1 );
  auto const actual_ratio_2 =
      AspectRatio::from_size( resolution_2 );
  auto const actual_ratio_3 =
      AspectRatio::from_size( resolution_3 );
  auto const actual_ratio_4 =
      AspectRatio::from_size( resolution_4 );
  auto const actual_ratio_5 =
      AspectRatio::from_size( resolution_5 );
  REQUIRE( actual_ratio_0.has_value() );
  REQUIRE( actual_ratio_1.has_value() );
  REQUIRE( actual_ratio_2.has_value() );
  REQUIRE( actual_ratio_3.has_value() );
  REQUIRE( actual_ratio_4.has_value() );
  REQUIRE( actual_ratio_5.has_value() );

  auto const snapped_ratio_0 =
      find_closest_named_aspect_ratio( *actual_ratio_0 );
  auto const snapped_ratio_1 =
      find_closest_named_aspect_ratio( *actual_ratio_1 );
  auto const snapped_ratio_2 =
      find_closest_named_aspect_ratio( *actual_ratio_2 );
  auto const snapped_ratio_3 =
      find_closest_named_aspect_ratio( *actual_ratio_3 );
  auto const snapped_ratio_4 =
      find_closest_named_aspect_ratio( *actual_ratio_4 );
  auto const snapped_ratio_5 =
      find_closest_named_aspect_ratio( *actual_ratio_5 );

  REQUIRE( snapped_ratio_0 == nothing );
  REQUIRE( snapped_ratio_1 == w_h_4_3 );
  REQUIRE( snapped_ratio_2 == w_h_4_3 );
  REQUIRE( snapped_ratio_3 == w_h_1_1 );
  REQUIRE( snapped_ratio_4 == w_h_16_9 );
  REQUIRE( snapped_ratio_5 == nothing );
}

TEST_CASE( "[gfx/aspect] AspectRatio reduction" ) {
  size const resolution_0 = { .w = 1'024, .h = 700 };
  size const resolution_1 = { .w = 1'024, .h = 807 };
  size const resolution_2 = { .w = 1'024, .h = 806 };
  size const resolution_3 = { .w = 1'024, .h = 1'024 };
  size const resolution_4 = { .w = 1'024, .h = 577 };
  size const resolution_5 = { .w = 1'024, .h = 605 };
  size const resolution_6 = { .w = 640, .h = 360 };
  size const resolution_7 = { .w = 320, .h = 200 };
  size const resolution_8 = { .w = 1, .h = 1 };

  auto const actual_ratio_0 =
      AspectRatio::from_size( resolution_0 );
  auto const actual_ratio_1 =
      AspectRatio::from_size( resolution_1 );
  auto const actual_ratio_2 =
      AspectRatio::from_size( resolution_2 );
  auto const actual_ratio_3 =
      AspectRatio::from_size( resolution_3 );
  auto const actual_ratio_4 =
      AspectRatio::from_size( resolution_4 );
  auto const actual_ratio_5 =
      AspectRatio::from_size( resolution_5 );
  auto const actual_ratio_6 =
      AspectRatio::from_size( resolution_6 );
  auto const actual_ratio_7 =
      AspectRatio::from_size( resolution_7 );
  auto const actual_ratio_8 =
      AspectRatio::from_size( resolution_8 );

  REQUIRE( actual_ratio_0.has_value() );
  REQUIRE( actual_ratio_1.has_value() );
  REQUIRE( actual_ratio_2.has_value() );
  REQUIRE( actual_ratio_3.has_value() );
  REQUIRE( actual_ratio_4.has_value() );
  REQUIRE( actual_ratio_5.has_value() );
  REQUIRE( actual_ratio_6.has_value() );
  REQUIRE( actual_ratio_7.has_value() );
  REQUIRE( actual_ratio_8.has_value() );

  REQUIRE( actual_ratio_0->get() == size{ .w = 256, .h = 175 } );
  REQUIRE( actual_ratio_1->get() ==
           size{ .w = 1'024, .h = 807 } );
  REQUIRE( actual_ratio_2->get() == size{ .w = 512, .h = 403 } );
  REQUIRE( actual_ratio_3->get() == size{ .w = 1, .h = 1 } );
  REQUIRE( actual_ratio_4->get() ==
           size{ .w = 1'024, .h = 577 } );
  REQUIRE( actual_ratio_5->get() ==
           size{ .w = 1'024, .h = 605 } );
  REQUIRE( actual_ratio_6->get() == size{ .w = 16, .h = 9 } );
  REQUIRE( actual_ratio_7->get() == size{ .w = 8, .h = 5 } );
  REQUIRE( actual_ratio_8->get() == size{ .w = 1, .h = 1 } );
}

TEST_CASE( "[gfx/aspect] AspectRatio/to_str" ) {
  size const resolution_0 = { .w = 1'024, .h = 700 };
  size const resolution_1 = { .w = 1'024, .h = 807 };
  size const resolution_2 = { .w = 1'024, .h = 806 };
  size const resolution_3 = { .w = 1'024, .h = 1'024 };
  size const resolution_4 = { .w = 1'024, .h = 577 };
  size const resolution_5 = { .w = 1'024, .h = 605 };
  size const resolution_6 = { .w = 640, .h = 360 };
  size const resolution_7 = { .w = 320, .h = 200 };

  auto const actual_ratio_0 =
      AspectRatio::from_size( resolution_0 );
  auto const actual_ratio_1 =
      AspectRatio::from_size( resolution_1 );
  auto const actual_ratio_2 =
      AspectRatio::from_size( resolution_2 );
  auto const actual_ratio_3 =
      AspectRatio::from_size( resolution_3 );
  auto const actual_ratio_4 =
      AspectRatio::from_size( resolution_4 );
  auto const actual_ratio_5 =
      AspectRatio::from_size( resolution_5 );
  auto const actual_ratio_6 =
      AspectRatio::from_size( resolution_6 );
  auto const actual_ratio_7 =
      AspectRatio::from_size( resolution_7 );

  REQUIRE( base::to_str( actual_ratio_0 ) == "256:175" );
  REQUIRE( base::to_str( actual_ratio_1 ) == "1024:807" );
  REQUIRE( base::to_str( actual_ratio_2 ) == "512:403" );
  REQUIRE( base::to_str( actual_ratio_3 ) == "1:1" );
  REQUIRE( base::to_str( actual_ratio_4 ) == "1024:577" );
  REQUIRE( base::to_str( actual_ratio_5 ) == "1024:605" );
  REQUIRE( base::to_str( actual_ratio_6 ) == "16:9" );
  REQUIRE( base::to_str( actual_ratio_7 ) == "8:5" );
}

// This test tests that the methods in this module work on the
// list of most common primary-monitor resolutions that players
// have as reported by the the Sept. 2024 Steam hardware survey
// on monitor geometry. Specifically, it tests that they are
// bucketed correctly.
TEST_CASE( "[gfx/aspect] steam numbers" ) {
  using enum e_named_aspect_ratio;
  vector<size> const resolutions = {
    size{ .w = 1'920, .h = 1'080 }, // 0
    size{ .w = 2'560, .h = 1'440 }, // 1
    size{ .w = 2'560, .h = 1'600 }, // 2
    size{ .w = 3'840, .h = 2'160 }, // 3
    size{ .w = 1'366, .h = 768 },   // 4
    size{ .w = 3'440, .h = 1'440 }, // 5
    size{ .w = 1'920, .h = 1'200 }, // 6
    size{ .w = 1'600, .h = 900 },   // 7
    size{ .w = 1'440, .h = 900 },   // 8
    size{ .w = 2'560, .h = 1'080 }, // 9
    size{ .w = 1'680, .h = 1'050 }, // 10
    size{ .w = 1'360, .h = 768 },   // 11
    size{ .w = 800, .h = 1'280 },   // 12
    size{ .w = 1'280, .h = 800 },   // 13
    size{ .w = 2'880, .h = 1'800 }, // 14
    size{ .w = 5'120, .h = 1'440 }, // 15
    size{ .w = 1'280, .h = 1'024 }, // 16
  };

  using MR = maybe<AspectRatio>;
  using AR = AspectRatio;

  vector<MR> const expected_buckets = {
    MR{ AR::from_named( w_h_16_9 ) },
    MR{ AR::from_named( w_h_16_9 ) },
    MR{ AR::from_named( w_h_16_10 ) },
    MR{ AR::from_named( w_h_16_9 ) },
    MR{ AR::from_named( w_h_16_9 ) },
    MR{ AR::from_named( w_h_21_9 ) },
    MR{ AR::from_named( w_h_16_10 ) },
    MR{ AR::from_named( w_h_16_9 ) },
    MR{ AR::from_named( w_h_16_10 ) },
    MR{ AR::from_named( w_h_21_9 ) },
    MR{ AR::from_named( w_h_16_10 ) },
    MR{ AR::from_named( w_h_16_9 ) },
    MR{ AR::from_named( w_h_10_16 ) },
    MR{ AR::from_named( w_h_16_10 ) },
    MR{ AR::from_named( w_h_16_10 ) },
    MR{},
    MR{ AR::from_named( w_h_4_3 ) },
  };

  BASE_CHECK( expected_buckets.size() == resolutions.size() );

  for( int i = 0; i < ssize( resolutions ); ++i ) {
    INFO( fmt::format( "i={}", i ) );
    UNWRAP_CHECK_T( auto const ratio,
                    AspectRatio::from_size( resolutions[i] ) );
    auto const approximate_ratio = find_closest_aspect_ratio(
        AspectRatio::named_all(), ratio );
    REQUIRE( approximate_ratio == expected_buckets[i] );
  }
}

TEST_CASE( "[gfx/aspect] supported_logical_resolutions" ) {
  vector<LogicalResolution> expected;
  size                      resolution;

  auto f = [&] {
    return supported_logical_resolutions( resolution );
  };

  resolution = { .w = 1, .h = 1 };
  expected   = {
    { .dimensions = { .w = 1, .h = 1 }, .scale = 1 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 1, .h = 2 };
  expected   = {
    { .dimensions = { .w = 1, .h = 2 }, .scale = 1 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 2, .h = 2 };
  expected   = {
    { .dimensions = { .w = 2, .h = 2 }, .scale = 1 },
    { .dimensions = { .w = 1, .h = 1 }, .scale = 2 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 3, .h = 2 };
  expected   = {
    { .dimensions = { .w = 3, .h = 2 }, .scale = 1 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 4, .h = 2 };
  expected   = {
    { .dimensions = { .w = 4, .h = 2 }, .scale = 1 },
    { .dimensions = { .w = 2, .h = 1 }, .scale = 2 },
  };
  REQUIRE( f() == expected );

  resolution = { .w = 3'840, .h = 2'160 };
  expected   = {
    { .dimensions = { .w = 3'840, .h = 2'160 }, .scale = 1 },
    { .dimensions = { .w = 1'920, .h = 1'080 }, .scale = 2 },
    { .dimensions = { .w = 1'280, .h = 720 }, .scale = 3 },
    { .dimensions = { .w = 960, .h = 540 }, .scale = 4 },
    { .dimensions = { .w = 768, .h = 432 }, .scale = 5 },
    { .dimensions = { .w = 640, .h = 360 }, .scale = 6 },
    { .dimensions = { .w = 480, .h = 270 }, .scale = 8 },
    { .dimensions = { .w = 384, .h = 216 }, .scale = 10 },
    { .dimensions = { .w = 320, .h = 180 }, .scale = 12 },
    { .dimensions = { .w = 256, .h = 144 }, .scale = 15 },
    { .dimensions = { .w = 240, .h = 135 }, .scale = 16 },
    { .dimensions = { .w = 192, .h = 108 }, .scale = 20 },
    { .dimensions = { .w = 160, .h = 90 }, .scale = 24 },
    { .dimensions = { .w = 128, .h = 72 }, .scale = 30 },
    { .dimensions = { .w = 96, .h = 54 }, .scale = 40 },
    { .dimensions = { .w = 80, .h = 45 }, .scale = 48 },
    { .dimensions = { .w = 64, .h = 36 }, .scale = 60 },
    { .dimensions = { .w = 48, .h = 27 }, .scale = 80 },
    { .dimensions = { .w = 32, .h = 18 }, .scale = 120 },
    { .dimensions = { .w = 16, .h = 9 }, .scale = 240 },
  };
  REQUIRE( f() == expected );
}

TEST_CASE( "[gfx/aspect] named_ratio_canonical_name" ) {
  using enum e_named_aspect_ratio;

  auto f = []( e_named_aspect_ratio const r ) {
    return named_ratio_canonical_name( r );
  };

  REQUIRE( f( w_h_16_9 ) == "16:9" );
  REQUIRE( f( w_h_16_10 ) == "16:10" );
  REQUIRE( f( w_h_4_3 ) == "4:3" );
  REQUIRE( f( w_h_21_9 ) == "21:9" );
  REQUIRE( f( w_h_1_1 ) == "1:1" );
}

TEST_CASE( "[gfx/aspect] resolution_analysis" ) {
}

TEST_CASE( "[gfx/aspect] recommended_resolution" ) {
}

} // namespace
} // namespace gfx
