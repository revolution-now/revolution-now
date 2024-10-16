/****************************************************************
**resolution.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-14.
*
* Description: Handles things related to the physical and logical
*              resolutions used by the game.
*
*****************************************************************/
#include "resolution.hpp"

// gfx
#include "gfx/aspect.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::maybe;

// TODO

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
vector<gfx::Resolution> compute_available_logical_resolutions(
    gfx::size const physical_size ) {
  vector<gfx::Resolution> res;
  vector<gfx::size> const target_logical_resolutions{
    // 16:9
    { .w = 768, .h = 432 }, // for 27 in. monitors.
    { .w = 640, .h = 360 }, // for laptops.

    // 16:10
    // { .w = 720, .h = 450 }, // for macbook pro.
    // { .w = 576, .h = 360 }, // for macbook pro.
    { .w = 640, .h = 400 },

    // 4:3
    { .w = 640, .h = 480 },
  };

  gfx::ResolutionAnalysis const analysis = resolution_analysis(
      target_logical_resolutions, physical_size );

  double const tolerance = gfx::default_aspect_ratio_tolerance();

  return available_resolutions( analysis, tolerance );
}

maybe<gfx::Resolution> recompute_best_logical_resolution(
    gfx::size const physical_size ) {
  auto recommended =
      compute_available_logical_resolutions( physical_size );
  if( recommended.empty() ) return nothing;
  return std::move( recommended[0] );
}

} // namespace rn
