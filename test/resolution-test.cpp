/****************************************************************
**resolution-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-30.
*
* Description: Unit tests for the resolution module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/config/resolutions.hpp"
#include "src/resolution.hpp"

// Testing.
#include "test/data/steam/steam-resolutions.hpp"

// rcl
#include "src/rcl/golden.hpp"

// refl
#include "src/refl/cdr.hpp"
#include "src/refl/to-str.hpp"

// cdr
#include "src/cdr/ext-base.hpp"
#include "src/cdr/ext-builtin.hpp"
#include "src/cdr/ext-std.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

// C++ standard library
#include <unordered_set>

namespace rn {
namespace {

using namespace std;
using namespace ::gfx;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[resolution] resolution_size" ) {
  using enum e_resolution;
  using S       = gfx::size;
  auto const& f = resolution_size;
  REQUIRE( f( _640x360 ) == S{ .w = 640, .h = 360 } );
  REQUIRE( f( _640x400 ) == S{ .w = 640, .h = 400 } );
  REQUIRE( f( _768x432 ) == S{ .w = 768, .h = 432 } );
}

TEST_CASE( "[resolution] supported_resolutions" ) {
  vector<gfx::size> const expected{
    gfx::size{ .w = 640, .h = 360 },
    gfx::size{ .w = 768, .h = 432 },
    gfx::size{ .w = 640, .h = 400 },
  };
  REQUIRE( supported_resolutions() == expected );
}

// Ensure that each of the steam resolutions has at least one
// fullscreen or near-fullscreen possibility given the resolu-
// tions that we support.
TEST_CASE( "[resolution] steam numbers / fullscreen" ) {
  using ::gfx::size;
  using ::testing::steam_resolutions;

  ResolutionAnalysisOptions options{
    .rating_options =
        ResolutionRatingOptions{
          .prefer_fullscreen = true,
          .tolerance =
              ResolutionTolerance{
                .min_percent_covered  = nothing,
                .fitting_score_cutoff = nothing },
          .ideal_pixel_size_mm = .66145,
          .remove_redundant    = true },
    .supported_logical_dimensions = supported_resolutions() };

  auto f = [&] { return resolution_analysis( options ); };

  auto is_within_n = []( int const                tolerance,
                         ResolutionRatings const& rr ) {
    if( rr.available.empty() ) return false;
    int const scale = rr.available[0].resolution.scale;
    BASE_CHECK( scale > 0 );
    point const viewport =
        rr.available[0].resolution.viewport.origin;
    return viewport.x / scale <= tolerance &&
           viewport.y / scale <= tolerance;
  };

  int tolerance = {};

  auto check_fits = [&]( auto const& expected_fail ) {
    for( gfx::size const sz : steam_resolutions() ) {
      INFO(
          fmt::format( "sz={}, tolerance={}", sz, tolerance ) );
      options.monitor.physical_screen = sz;
      options.physical_window         = sz;
      bool const should_match = !expected_fail.contains( sz );
      REQUIRE( is_within_n( tolerance, f() ) == should_match );
    }
  };

  tolerance = 0; // exact fullscreen fit.
  {
    static unordered_set const expected_fail{
      size{ .w = 1366, .h = 768 },  //
      size{ .w = 3440, .h = 1440 }, //
      size{ .w = 1600, .h = 900 },  //
      size{ .w = 1440, .h = 900 },  //
      size{ .w = 2560, .h = 1080 }, //
      size{ .w = 1680, .h = 1050 }, //
      size{ .w = 1360, .h = 768 },  //
      size{ .w = 2880, .h = 1800 }, //
      size{ .w = 5120, .h = 1440 }, //
      size{ .w = 1280, .h = 1024 }, //
    };
    check_fits( expected_fail );
  }

  tolerance = 32; // 32-logical pixel gap around edges.
  {
    static unordered_set const expected_fail{
      size{ .w = 1680, .h = 1050 }, //
      size{ .w = 5120, .h = 1440 }, //
      size{ .w = 3440, .h = 1440 }, //
      size{ .w = 1440, .h = 900 },  //
      size{ .w = 2560, .h = 1080 }, //
      size{ .w = 2880, .h = 1800 }, //
      size{ .w = 1280, .h = 1024 }, //
    };
    check_fits( expected_fail );
  }

  tolerance = 128; // 128-logical pixel gap around edges.
  {
    static unordered_set const expected_fail{
      size{ .w = 5120, .h = 1440 }, //
    };
    check_fits( expected_fail );
  }
}

TEST_CASE( "[resolution] compute_resolutions 3840x2160 27in" ) {
  gfx::size const physical{ .w = 3840, .h = 2160 };

  // 27in 4K monitor.
  gfx::Monitor const monitor{ .physical_screen = physical,
                              .dpi =
                                  MonitorDpi{
                                    .horizontal = 192.0,
                                    .vertical   = 192.0,
                                    .diagonal   = 163.355,
                                  },
                              .diagonal_inches = 22.9 };

  Resolutions const resolutions =
      compute_resolutions( monitor, physical );

  rcl::Golden const gold( resolutions,
                          "analysis-3840x2160-27in" );

  REQUIRE( gold.is_golden() == base::valid );
}

TEST_CASE( "[resolution] compute_resolutions 1920x1080 15in" ) {
  gfx::size const physical{ .w = 1920, .h = 1080 };

  // 15in 1080p monitor.
  gfx::Monitor const monitor{
    .physical_screen = { .w = 1920, .h = 1080 },
    .dpi             = MonitorDpi{ .horizontal = 96.0,
                                   .vertical   = 96.0,
                                   .diagonal   = 141.68 },
    .diagonal_inches = 15.0 };

  Resolutions const resolutions =
      compute_resolutions( monitor, physical );

  rcl::Golden const gold( resolutions,
                          "analysis-1920x1080-15in" );

  REQUIRE( gold.is_golden() == base::valid );
}

} // namespace
} // namespace rn
