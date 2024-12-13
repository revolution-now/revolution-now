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
#include "test/testing.hpp"

// Under test.
#include "src/gfx/aspect.hpp"

// testing
#include "test/data/steam/steam-resolutions.hpp"

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
TEST_CASE( "[gfx/aspect] named_aspect_ratio" ) {
  using enum e_named_aspect_ratio;
  auto f = [&]( e_named_aspect_ratio const ratio ) {
    return named_aspect_ratio( ratio );
  };
  REQUIRE( f( _16x9 ) == size{ .w = 16, .h = 9 } );
  REQUIRE( f( _16x10 ) == size{ .w = 16, .h = 10 } );
  REQUIRE( f( _4x3 ) == size{ .w = 4, .h = 3 } );
  REQUIRE( f( _21x9 ) == size{ .w = 21, .h = 9 } );
}

TEST_CASE( "[gfx/aspect] find_close_named_aspect_ratio" ) {
  using enum e_named_aspect_ratio;
  double const kTolerance = 0.04;

  auto const& f = [&]( size const sz ) {
    return find_close_named_aspect_ratio( sz, kTolerance );
  };

  size const res_0 = { .w = 1024, .h = 700 };
  size const res_1 = { .w = 1024, .h = 807 };
  size const res_2 = { .w = 1024, .h = 806 };
  size const res_3 = { .w = 1024, .h = 438 };
  size const res_4 = { .w = 1024, .h = 577 };
  size const res_5 = { .w = 1024, .h = 305 };

  REQUIRE( f( res_0 ) == nothing );
  REQUIRE( f( res_1 ) == _4x3 );
  REQUIRE( f( res_2 ) == _4x3 );
  REQUIRE( f( res_3 ) == _21x9 );
  REQUIRE( f( res_4 ) == _16x9 );
  REQUIRE( f( res_5 ) == nothing );
}

TEST_CASE(
    "[gfx/aspect] find_close_named_aspect_ratio works on steam "
    "numbers" ) {
  using enum e_named_aspect_ratio;
  double const kTolerance         = 0.04;
  vector<size> const& resolutions = testing::steam_resolutions();

  using MR = maybe<e_named_aspect_ratio>;

  vector<MR> const expected_buckets = {
    MR{ _16x9 },  MR{ _16x9 },  MR{ _16x10 }, MR{ _16x9 },
    MR{ _16x9 },  MR{ _21x9 },  MR{ _16x10 }, MR{ _16x9 },
    MR{ _16x10 }, MR{ _21x9 },  MR{ _16x10 }, MR{ _16x9 },
    MR{ _16x10 }, MR{ _16x10 }, MR{},         MR{ _4x3 },
  };

  BASE_CHECK( expected_buckets.size() == resolutions.size() );

  for( int i = 0; i < ssize( resolutions ); ++i ) {
    INFO( fmt::format( "i={}", i ) );
    auto const r = resolutions[i];
    auto const approximate_ratio =
        find_close_named_aspect_ratio( r, kTolerance );
    REQUIRE( approximate_ratio == expected_buckets[i] );
  }
}

} // namespace
} // namespace gfx
