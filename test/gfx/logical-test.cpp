/****************************************************************
**logical-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-25.
*
* Description: Unit tests for the gfx/logical module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/gfx/logical.hpp"
#include "src/gfx/monitor.hpp"

// gfx
#include "src/gfx/logical-impl.rds.hpp"

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

namespace gfx {
namespace {

using namespace std;

using ::base::maybe;
using ::base::nothing;

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[gfx/logical] monitor_properties" ) {
  size physical = {};
  maybe<MonitorDpi> dpi;
  Monitor expected;

  auto f = [&] { return monitor_properties( physical, dpi ); };

  physical = { .w = 1920, .h = 1080 };
  dpi      = nothing;
  expected = { .physical_screen = physical,
               .dpi             = nothing,
               .diagonal_inches = nothing };
  REQUIRE( f() == expected );

  physical = { .w = 1920, .h = 1080 };

  dpi = MonitorDpi{
    .horizontal = 200.0, .vertical = 200.0, .diagonal = 141.68 };
  expected = { .physical_screen = physical,
               .dpi             = dpi,
               .diagonal_inches = 15.548469579914583 };
  REQUIRE( f() == expected );
}

TEST_CASE( "[gfx/logical] resolution_analysis empty options" ) {
  ResolutionAnalysisOptions const options{};
  vector<ScoredResolution> const analysis =
      resolution_analysis( options );
  REQUIRE( analysis == vector<ScoredResolution>{} );
}

// 27in 4K monitor.
TEST_CASE( "[gfx/logical] resolution_analysis 3840x2160 27in" ) {
  ResolutionAnalysisOptions const options{
    .monitor =
        Monitor{ .physical_screen = { .w = 3840, .h = 2160 },
                 .dpi =
                     MonitorDpi{
                       .horizontal = 192.0,
                       .vertical   = 192.0,
                       .diagonal   = 163.355,
                     },
                 .diagonal_inches = 22.9 },
    .physical_window = { .w = 3840, .h = 2160 },
    .scoring_options = ResolutionScoringOptions{
      .prefer_fullscreen = true,
      .tolerance =
          ResolutionTolerance{ .min_percent_covered  = nothing,
                               .fitting_score_cutoff = nothing },
      .ideal_pixel_size_mm = .66145,
      .remove_redundant    = true } };

  WrappedScoredResolutions const analysis{
    .v = resolution_analysis( options ) };

  rcl::Golden const gold( analysis, "analysis-3840x2160-27in" );

  REQUIRE( gold.is_golden() == base::valid );
}

// 15in 1080p laptop monitor.
TEST_CASE( "[gfx/logical] resolution_analysis 1920x1080 15in" ) {
  ResolutionAnalysisOptions const options{
    .monitor =
        Monitor{ .physical_screen = { .w = 1920, .h = 1080 },
                 .dpi = MonitorDpi{ .horizontal = 96.0,
                                    .vertical   = 96.0,
                                    .diagonal   = 141.68 },
                 .diagonal_inches = 15.0 },
    .physical_window = { .w = 1920, .h = 1080 },
    .scoring_options = ResolutionScoringOptions{
      .prefer_fullscreen = true,
      .tolerance =
          ResolutionTolerance{ .min_percent_covered  = nothing,
                               .fitting_score_cutoff = nothing },
      .ideal_pixel_size_mm = .66145,
      .remove_redundant    = true } };

  WrappedScoredResolutions const analysis{
    .v = resolution_analysis( options ) };

  rcl::Golden const gold( analysis, "analysis-1920x1080-15in" );

  REQUIRE( gold.is_golden() == base::valid );
}

// Since this is such an important resolution we'll explicitly
// test that the best selection is 640x360.
TEST_CASE(
    "[gfx/logical] resolution_analysis 1920x1080 first" ) {
  ResolutionAnalysisOptions const options{
    // 15in 2K monitor.
    .monitor =
        Monitor{ .physical_screen = { .w = 1920, .h = 1080 },
                 .dpi = MonitorDpi{ .horizontal = 96.0,
                                    .vertical   = 96.0,
                                    .diagonal   = 141.68 },
                 .diagonal_inches = 15.0 },
    .physical_window = { .w = 1920, .h = 1080 },
    .scoring_options = ResolutionScoringOptions{
      .prefer_fullscreen = true,
      .tolerance =
          ResolutionTolerance{ .min_percent_covered  = nothing,
                               .fitting_score_cutoff = nothing },
      .ideal_pixel_size_mm = .66145,
      .remove_redundant    = true } };

  vector<ScoredResolution> const analysis =
      resolution_analysis( options );

  REQUIRE( !analysis.empty() );

  auto const best_logical_resolution =
      analysis[0].resolution.logical;
  size const expected{ .w = 640, .h = 360 };

  REQUIRE( best_logical_resolution == expected );
}

TEST_CASE( "[gfx/logical] resolution_analysis 1920x1200 15in" ) {
  // TODO
}

TEST_CASE( "[gfx/logical] resolution_analysis 2560x1440 15in" ) {
  // TODO
}

TEST_CASE( "[gfx/logical] resolution_analysis 2560x1600 15in" ) {
  // TODO
}

} // namespace
} // namespace gfx
