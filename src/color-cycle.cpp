/****************************************************************
**color-cycle.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2024-10-04.
*
* Description: Game-specific color-cycling logic.
*
*****************************************************************/
#include "color-cycle.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "gfx/pixel.hpp"
#include "igui.hpp"
#include "logger.hpp"

// config
#include "config/gfx.rds.hpp"

// render
#include "render/irenderer.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/scope-exit.hpp"

using namespace std;

namespace rn {

namespace {

/****************************************************************
** Constants.
*****************************************************************/
size_t constexpr kCyclePlanSpan = std::tuple_size_v<
    decltype( config::graphics::ColorCyclePlan::slots )>;

static_assert( kCyclePlanSpan > 0 );

size_t constexpr kNumCyclePlans =
    refl::enum_count<e_color_cycle_plan>;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
wait<> cycle_map_colors_thread( rr::IRenderer& renderer,
                                IGui&          gui,
                                bool const&    enabled ) {
  SCOPE_EXIT { renderer.set_color_cycle_stage( 0 ); };
  int stage = 0;

  while( true ) {
    // The "stage > 0" clause is so that we do a smooth transi-
    // tion from enabled to disabled. This is because when we are
    // disabled we must have stage=0 for things to look good, and
    // we don't want to just jump there.
    bool const on = enabled || stage > 0;
    renderer.set_color_cycle_stage( on ? stage++ : 0 );
    stage %= kCyclePlanSpan;
    co_await gui.wait_for( 600ms );
  }
}

void set_color_cycle_plans( rr::IRenderer& renderer ) {
  vector<gfx::pixel> flattened;
  size_t constexpr kTotalElems = kNumCyclePlans * kCyclePlanSpan;
  flattened.reserve( kTotalElems );
  for( auto const& [mode, plan] :
       config_gfx.color_cycle_plans.plans ) {
    lg.debug( "installing color cycling plan: {}", mode );
    flattened.insert( flattened.end(), plan.slots.begin(),
                      plan.slots.end() );
  }
  CHECK_EQ( flattened.size(), kTotalElems ); // Sanity check.
  renderer.set_color_cycle_plans( flattened );
}

int cycle_plan_idx( e_color_cycle_plan const plan ) {
  return static_cast<int>( plan );
}

} // namespace rn
