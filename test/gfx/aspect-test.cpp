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
#include "test/testing.hpp"

// Under test.
#include "src/gfx/aspect.hpp"

// refl
// #include "src/refl/to-str.hpp"

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
  size const resolution_0 = { .w = 1'024, .h = 700 };
  size const resolution_1 = { .w = 1'024, .h = 807 };
  size const resolution_2 = { .w = 1'024, .h = 806 };
  size const resolution_3 = { .w = 1'024, .h = 1'024 };
  size const resolution_4 = { .w = 1'024, .h = 577 };
  size const resolution_5 = { .w = 1'024, .h = 605 };

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

  double const tolerance = default_ratio_tolerance();

  auto const all_aspect_ratios = {
    AspectRatio::from_named( e_named_aspect_ratio::_16x9 ),
    AspectRatio::from_named( e_named_aspect_ratio::_16x10 ),
    AspectRatio::from_named( e_named_aspect_ratio::_4x3 ),
    AspectRatio::from_named( e_named_aspect_ratio::_1x1 ),
  };

  auto const snapped_ratio_0 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_0, tolerance );
  auto const snapped_ratio_1 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_1, tolerance );
  auto const snapped_ratio_2 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_2, tolerance );
  auto const snapped_ratio_3 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_3, tolerance );
  auto const snapped_ratio_4 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_4, tolerance );
  auto const snapped_ratio_5 = find_closest_aspect_ratio(
      all_aspect_ratios, *actual_ratio_5, tolerance );

  REQUIRE( snapped_ratio_0 == nothing );
  REQUIRE( snapped_ratio_1 == nothing );
  REQUIRE( snapped_ratio_2 == AspectRatio::from_named(
                                  e_named_aspect_ratio::_4x3 ) );
  REQUIRE( snapped_ratio_3 == AspectRatio::from_named(
                                  e_named_aspect_ratio::_1x1 ) );
  REQUIRE(
      snapped_ratio_4 ==
      AspectRatio::from_named( e_named_aspect_ratio::_16x9 ) );
  REQUIRE( snapped_ratio_5 == nothing );
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
  vector<size> const resolutions = {
    size{ .w = 1'920, .h = 1'080 },
    size{ .w = 2'560, .h = 1'440 },
    size{ .w = 2'560, .h = 1'600 },
    size{ .w = 3'840, .h = 2'160 },
    size{ .w = 1'366, .h = 768 },
    size{ .w = 3'440, .h = 1'440 },
    size{ .w = 1'920, .h = 1'200 },
    size{ .w = 1'600, .h = 900 },
    size{ .w = 1'440, .h = 900 },
    size{ .w = 2'560, .h = 1'080 },
    size{ .w = 1'680, .h = 1'050 },
    size{ .w = 1'360, .h = 768 },
    size{ .w = 800, .h = 1'280 },
    size{ .w = 1'280, .h = 800 },
    size{ .w = 2'880, .h = 1'800 },
    size{ .w = 5'120, .h = 1'440 },
    size{ .w = 1'280, .h = 1'024 },
  };

  using EB = maybe<AspectRatio>;
  using AR = AspectRatio;

  vector<EB> const expected_buckets = {
    EB{ AR::from_named( e_named_aspect_ratio::_16x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x10 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_21x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x10 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x10 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_21x9 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x10 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x9 ) },
    EB{},
    EB{ AR::from_named( e_named_aspect_ratio::_16x10 ) },
    EB{ AR::from_named( e_named_aspect_ratio::_16x10 ) },
    EB{},
    EB{},
  };

  BASE_CHECK( expected_buckets.size() == resolutions.size() );

  double const tolerance = default_ratio_tolerance();

  for( int i = 0; i < ssize( resolutions ); ++i ) {
    INFO( fmt::format( "i={}", i ) );
    UNWRAP_CHECK_T( auto const ratio,
                    AspectRatio::from_size( resolutions[i] ) );
    auto const approximate_ratio = find_closest_aspect_ratio(
        AspectRatio::named_all(), ratio, tolerance );
    REQUIRE( approximate_ratio == expected_buckets[i] );
  }
}

} // namespace
} // namespace gfx
