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
TEST_CASE( "[gfx/aspect] AspectRatio construction/getters" ) {
  size const resolution_0  = { .w = 1'024, .h = 700 };
  size const resolution_1  = { .w = 1'024, .h = 807 };
  size const resolution_2  = { .w = 1'024, .h = 806 };
  size const resolution_3  = { .w = 1'024, .h = 1'024 };
  size const resolution_4  = { .w = 1'024, .h = 577 };
  size const resolution_5  = { .w = 1'024, .h = 605 };
  size const resolution_6  = { .w = 640, .h = 360 };
  size const resolution_7  = { .w = 320, .h = 200 };
  size const resolution_8  = { .w = 1, .h = 1 };
  size const resolution_9  = { .w = 0, .h = 0 };
  size const resolution_10 = { .w = 1, .h = 0 };

  auto const actual_ratio_0  = AspectRatio( resolution_0 );
  auto const actual_ratio_1  = AspectRatio( resolution_1 );
  auto const actual_ratio_2  = AspectRatio( resolution_2 );
  auto const actual_ratio_3  = AspectRatio( resolution_3 );
  auto const actual_ratio_4  = AspectRatio( resolution_4 );
  auto const actual_ratio_5  = AspectRatio( resolution_5 );
  auto const actual_ratio_6  = AspectRatio( resolution_6 );
  auto const actual_ratio_7  = AspectRatio( resolution_7 );
  auto const actual_ratio_8  = AspectRatio( resolution_8 );
  auto const actual_ratio_9  = AspectRatio( resolution_9 );
  auto const actual_ratio_10 = AspectRatio( resolution_10 );

  REQUIRE( actual_ratio_0.scalar().has_value() );
  REQUIRE( actual_ratio_1.scalar().has_value() );
  REQUIRE( actual_ratio_2.scalar().has_value() );
  REQUIRE( actual_ratio_3.scalar().has_value() );
  REQUIRE( actual_ratio_4.scalar().has_value() );
  REQUIRE( actual_ratio_5.scalar().has_value() );
  REQUIRE( actual_ratio_6.scalar().has_value() );
  REQUIRE( actual_ratio_7.scalar().has_value() );
  REQUIRE( actual_ratio_8.scalar().has_value() );
  REQUIRE_FALSE( actual_ratio_9.scalar().has_value() );
  REQUIRE_FALSE( actual_ratio_10.scalar().has_value() );

  REQUIRE( actual_ratio_0.scalar() == 1024.0 / 700 );
  REQUIRE( actual_ratio_1.scalar() == 1024.0 / 807 );
  REQUIRE( actual_ratio_2.scalar() == 1024.0 / 806 );
  REQUIRE( actual_ratio_3.scalar() == 1.0 );
  REQUIRE( actual_ratio_4.scalar() == 1024.0 / 577 );
  REQUIRE( actual_ratio_5.scalar() == 1024.0 / 605 );
  REQUIRE( actual_ratio_6.scalar() == 640.0 / 360 );
  REQUIRE( actual_ratio_7.scalar() == 320.0 / 200 );
  REQUIRE( actual_ratio_8.scalar() == 1.0 );
  REQUIRE_FALSE( actual_ratio_9.scalar().has_value() );
  REQUIRE_FALSE( actual_ratio_10.scalar().has_value() );

  REQUIRE( actual_ratio_0.get() == size{ .w = 256, .h = 175 } );
  REQUIRE( actual_ratio_1.get() ==
           size{ .w = 1'024, .h = 807 } );
  REQUIRE( actual_ratio_2.get() == size{ .w = 512, .h = 403 } );
  REQUIRE( actual_ratio_3.get() == size{ .w = 1, .h = 1 } );
  REQUIRE( actual_ratio_4.get() ==
           size{ .w = 1'024, .h = 577 } );
  REQUIRE( actual_ratio_5.get() ==
           size{ .w = 1'024, .h = 605 } );
  REQUIRE( actual_ratio_6.get() == size{ .w = 16, .h = 9 } );
  REQUIRE( actual_ratio_7.get() == size{ .w = 8, .h = 5 } );
  REQUIRE( actual_ratio_8.get() == size{ .w = 1, .h = 1 } );
  REQUIRE( actual_ratio_9.get() == size{ .w = 0, .h = 0 } );
  REQUIRE( actual_ratio_10.get() == size{ .w = 1, .h = 0 } );
}

TEST_CASE( "[gfx/aspect] find_closest_named_aspect_ratio" ) {
  using enum e_named_aspect_ratio;
  size const resolution_0 = { .w = 1'024, .h = 700 };
  size const resolution_1 = { .w = 1'024, .h = 807 };
  size const resolution_2 = { .w = 1'024, .h = 806 };
  size const resolution_3 = { .w = 1'024, .h = 438 };
  size const resolution_4 = { .w = 1'024, .h = 577 };
  size const resolution_5 = { .w = 1'024, .h = 305 };

  auto const actual_ratio_0 = AspectRatio( resolution_0 );
  auto const actual_ratio_1 = AspectRatio( resolution_1 );
  auto const actual_ratio_2 = AspectRatio( resolution_2 );
  auto const actual_ratio_3 = AspectRatio( resolution_3 );
  auto const actual_ratio_4 = AspectRatio( resolution_4 );
  auto const actual_ratio_5 = AspectRatio( resolution_5 );
  REQUIRE( actual_ratio_0.scalar().has_value() );
  REQUIRE( actual_ratio_1.scalar().has_value() );
  REQUIRE( actual_ratio_2.scalar().has_value() );
  REQUIRE( actual_ratio_3.scalar().has_value() );
  REQUIRE( actual_ratio_4.scalar().has_value() );
  REQUIRE( actual_ratio_5.scalar().has_value() );

  auto const snapped_ratio_0 =
      find_closest_named_aspect_ratio( actual_ratio_0 );
  auto const snapped_ratio_1 =
      find_closest_named_aspect_ratio( actual_ratio_1 );
  auto const snapped_ratio_2 =
      find_closest_named_aspect_ratio( actual_ratio_2 );
  auto const snapped_ratio_3 =
      find_closest_named_aspect_ratio( actual_ratio_3 );
  auto const snapped_ratio_4 =
      find_closest_named_aspect_ratio( actual_ratio_4 );
  auto const snapped_ratio_5 =
      find_closest_named_aspect_ratio( actual_ratio_5 );

  REQUIRE( snapped_ratio_0 == nothing );
  REQUIRE( snapped_ratio_1 == _4_3 );
  REQUIRE( snapped_ratio_2 == _4_3 );
  REQUIRE( snapped_ratio_3 == _21_9 );
  REQUIRE( snapped_ratio_4 == _16_9 );
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

  auto const actual_ratio_0 = AspectRatio( resolution_0 );
  auto const actual_ratio_1 = AspectRatio( resolution_1 );
  auto const actual_ratio_2 = AspectRatio( resolution_2 );
  auto const actual_ratio_3 = AspectRatio( resolution_3 );
  auto const actual_ratio_4 = AspectRatio( resolution_4 );
  auto const actual_ratio_5 = AspectRatio( resolution_5 );
  auto const actual_ratio_6 = AspectRatio( resolution_6 );
  auto const actual_ratio_7 = AspectRatio( resolution_7 );

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
    size{ .w = 1'280, .h = 800 },   // 12
    size{ .w = 2'880, .h = 1'800 }, // 13
    size{ .w = 5'120, .h = 1'440 }, // 14
    size{ .w = 1'280, .h = 1'024 }, // 15
  };

  using MR = maybe<e_named_aspect_ratio>;

  vector<MR> const expected_buckets = {
    MR{ e_named_aspect_ratio::_16_9 },
    MR{ e_named_aspect_ratio::_16_9 },
    MR{ e_named_aspect_ratio::_16_10 },
    MR{ e_named_aspect_ratio::_16_9 },
    MR{ e_named_aspect_ratio::_16_9 },
    MR{ e_named_aspect_ratio::_21_9 },
    MR{ e_named_aspect_ratio::_16_10 },
    MR{ e_named_aspect_ratio::_16_9 },
    MR{ e_named_aspect_ratio::_16_10 },
    MR{ e_named_aspect_ratio::_21_9 },
    MR{ e_named_aspect_ratio::_16_10 },
    MR{ e_named_aspect_ratio::_16_9 },
    MR{ e_named_aspect_ratio::_16_10 },
    MR{ e_named_aspect_ratio::_16_10 },
    MR{},
    MR{ e_named_aspect_ratio::_4_3 },
  };

  BASE_CHECK( expected_buckets.size() == resolutions.size() );

  for( int i = 0; i < ssize( resolutions ); ++i ) {
    INFO( fmt::format( "i={}", i ) );
    auto const ratio = AspectRatio( resolutions[i] );
    auto const approximate_ratio =
        find_closest_named_aspect_ratio( ratio );
    REQUIRE( approximate_ratio == expected_buckets[i] );
  }
}

TEST_CASE( "[gfx/aspect] named_aspect_ratio" ) {
  e_named_aspect_ratio ratio    = {};
  size                 expected = {};

  auto f = [&] { return named_aspect_ratio( ratio ).get(); };

  using enum e_named_aspect_ratio;

  ratio    = _16_9;
  expected = { .w = 16, .h = 9 };
  REQUIRE( f() == expected );

  ratio    = _16_10;
  expected = { .w = 8, .h = 5 };
  REQUIRE( f() == expected );

  ratio    = _4_3;
  expected = { .w = 4, .h = 3 };
  REQUIRE( f() == expected );

  ratio    = _21_9;
  expected = { .w = 7, .h = 3 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[gfx/aspect] named_ratio_canonical_name" ) {
  using enum e_named_aspect_ratio;

  auto f = []( e_named_aspect_ratio const r ) {
    return named_ratio_canonical_name( r );
  };

  REQUIRE( f( _16_9 ) == "16:9" );
  REQUIRE( f( _16_10 ) == "16:10" );
  REQUIRE( f( _4_3 ) == "4:3" );
  REQUIRE( f( _21_9 ) == "21:9" );
}

} // namespace
} // namespace gfx
