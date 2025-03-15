/****************************************************************
**spread-builder.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Helper for building renderable tile spreads.
*
*****************************************************************/
#include "spread-builder.hpp"

// Revolution Now
#include "spread-render.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

// gfx
#include "gfx/spread-algo.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

using ::base::maybe;
using ::gfx::pixel;
using ::std::ranges::views::zip;

namespace {

// This could theoretically cause the labels to go outside of the
// bounds that was computed on the TileSpreadRenderPlans objec-
// t(s), but that should be very rare and not a big deal if/when
// it happens anyway.
void ensure_labels_are_non_overlapping(
    TileSpreadRenderPlans& plans ) {
  // Ensure that if there are muliple labels per spread that they
  // are spread out properly.
  for( auto& plan : plans.plans ) {
    if( plan.tiles.empty() ) continue;
    int right = numeric_limits<int>::min();
    for( auto& label_plan : plan.labels ) {
      int const new_min = right + 1;
      if( label_plan.bounds.left() < new_min )
        label_plan.bounds.origin.x = new_min;
      right = label_plan.bounds.right();
    }
  }
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
TileSpreadRenderPlans build_tile_spread_multi(
    rr::ITextometer const& textometer,
    TileSpreadConfigMulti const& configs ) {
  SpreadSpecs const specs = [&] {
    SpreadSpecs res;
    res.bounds        = configs.options.bounds;
    res.group_spacing = configs.group_spacing;
    for( auto const& config : configs.tiles ) {
      res.specs.push_back(
          SpreadSpec{ .count   = config.count,
                      .trimmed = trimmed_area_for( config.tile )
                                     .horizontal_slice() } );
    }
    return res;
  }();
  Spreads const icon_spreads = [&] {
    Spreads spreads;
    if( auto spreads_main_algo = compute_icon_spread( specs );
        spreads_main_algo.has_value() )
      spreads = std::move( *spreads_main_algo );
    else
      spreads = compute_icon_spread_proportionate( specs );
    CHECK_EQ( spreads.spreads.size(), specs.specs.size() );
    return spreads;
  }();
  TileSpreadSpecs const tile_spreads = [&] {
    TileSpreadSpecs res;
    res.label_policy  = configs.options.label_policy;
    res.group_spacing = specs.group_spacing;
    for( auto config_it = configs.tiles.begin();
         auto const [spec, icon_spread] :
         zip( specs.specs, icon_spreads.spreads ) ) {
      CHECK( config_it != configs.tiles.end() );
      TileSpreadSpec tile_spread_spec{
        .icon_spread = icon_spread,
        .tile        = config_it->tile,
        .label_opts  = configs.options.label_opts,
        .label_count = config_it->label_override };
      if( config_it->red_xs.has_value() ) {
        auto& overlay_tile =
            tile_spread_spec.overlay_tile.emplace();
        switch( config_it->red_xs->size ) {
          case rn::e_red_x_size::small: {
            overlay_tile.tile = e_tile::red_x_16;
            break;
          }
          case rn::e_red_x_size::large: {
            overlay_tile.tile = e_tile::red_x_20;
            break;
          }
        }
        overlay_tile.label_opts = configs.options.label_opts;
        overlay_tile.label_opts.color_fg = pixel::red();
        overlay_tile.starting_position =
            config_it->red_xs->starting_position;
        if( icon_spread.rendered_count != spec.count ) {
          // The count has been adjusted, thus we must adjust the
          // start of the X labels as well.
          double const fraction =
              double( config_it->red_xs->starting_position ) /
              spec.count;
          overlay_tile.starting_position = clamp(
              int( lround( fraction *
                           icon_spread.rendered_count ) ),
              ( config_it->red_xs->starting_position > 0 ) ? 1
                                                           : 0,
              icon_spread.rendered_count );
          if( !tile_spread_spec.label_count.has_value() )
            tile_spread_spec.label_count =
                config_it->red_xs->starting_position;
          overlay_tile.label_count = std::max(
              spec.count - config_it->red_xs->starting_position,
              0 );
        }
      }
      res.spreads.push_back(
          { .algo_spec = spec, .tile_spec = tile_spread_spec } );
      ++config_it;
    }
    return res;
  }();
  TileSpreadRenderPlans plans =
      render_plan_for_tile_spread( textometer, tile_spreads );
  ensure_labels_are_non_overlapping( plans );
  return plans;
}

TileSpreadRenderPlan build_tile_spread(
    rr::ITextometer const& textometer,
    TileSpreadConfig const& config ) {
  TileSpreadRenderPlan res;
  auto plans = build_tile_spread_multi(
      textometer,
      TileSpreadConfigMulti{ .tiles         = { config.tile },
                             .options       = config.options,
                             .group_spacing = 0 } );
  // Could be length zero if the input config has zero count.
  CHECK_LE( plans.plans.size(), 1u );
  if( !plans.plans.empty() ) res = std::move( plans.plans[0] );
  return res;
}

TileSpreadRenderPlan build_progress_tile_spread(
    rr::ITextometer const& textometer,
    ProgressTileSpreadConfig const& config_unadjusted ) {
  TileSpreadRenderPlan res;
  ProgressTileSpreadConfig const config = [&] {
    ProgressTileSpreadConfig res = config_unadjusted;
    // This is done for two reasons: one because if the progress
    // happens to be larger than the total (which can happen,
    // e.g. if the player transiently has more liberty bells than
    // are needed for the next founding father) then the below
    // spread algo will crash. Second, in that same scenario, if
    // we were to instead cap the player's progress count then
    // there would be a visual discrepancy between how many tiles
    // are rendered and what is shown on the label. So we just
    // increase the total to be equal to the progress, which
    // should lead to the right thing visually happening.
    res.count = std::max( res.count, res.progress_count );
    return res;
  }();
  ProgressSpreadSpec const progress_spec = [&] {
    ProgressSpreadSpec res;
    res.bounds      = config.options.bounds;
    res.spread_spec = SpreadSpec{
      .count = config.count,
      .trimmed =
          trimmed_area_for( config.tile ).horizontal_slice() };
    return res;
  }();

  if( auto progress_spread =
          compute_icon_spread_progress_bar( progress_spec );
      progress_spread.has_value() ) {
    int const rendered_count =
        clamp( progress_spec.spread_spec.count, 0,
               config.progress_count );
    ProgressTileSpreadSpec const tile_spec{
      .source_spec     = progress_spec,
      .progress_spread = *progress_spread,
      .rendered_count  = rendered_count,
      .tile            = config.tile,
      .label_opts      = config.options.label_opts,
      .label_count     = config.label_override.has_value()
                             ? config.label_override
                             : config.progress_count,
      .label_policy    = config.options.label_policy,
    };
    return render_plan_for_tile_progress_spread( textometer,
                                                 tile_spec );
  }
  SpreadSpec const& spec = progress_spec.spread_spec;
  SpreadSpecs const specs{ .bounds = config.options.bounds,
                           .specs  = { spec },
                           .group_spacing = 0 };
  Spreads spreads = compute_icon_spread_proportionate( specs );
  CHECK_EQ( spreads.spreads.size(), 1u );
  auto& spread = spreads.spreads[0];
  adjust_rendered_count_for_progress_count(
      spec, spread, config.progress_count );
  TileSpreadSpecs const tile_specs{
    .spreads       = { TileSpreadSpecWithSourceSpec{
            .algo_spec = spec,
            .tile_spec =
          TileSpreadSpec{
                  .icon_spread  = spread,
                  .tile         = config.tile,
                  .label_opts   = config.options.label_opts,
                  .label_count  = config.label_override.has_value()
                                      ? config.label_override
                                      : config.progress_count,
                  .overlay_tile = nothing } } },
    .group_spacing = 0,
    .label_policy  = config.options.label_policy };
  TileSpreadRenderPlans plans =
      render_plan_for_tile_spread( textometer, tile_specs );
  CHECK_EQ( plans.plans.size(), 1u );
  return std::move( plans.plans[0] );
}

TileSpreadRenderPlan build_inhomogenous_tile_spread(
    InhomogeneousTileSpreadConfig const& config ) {
  TileSpreadRenderPlan res;
  InhomogeneousSpreadSpec const spec{
    .bounds      = config.options.bounds,
    .max_spacing = config.max_spacing.value_or( 1 ),
    .widths      = [&] {
      vector<int> res;
      res.reserve( config.tiles.size() );
      for( e_tile const tile : config.tiles )
        res.push_back( trimmed_area_for( tile ).size.w );
      return res;
    }() };
  auto spread = compute_icon_spread_inhomogeneous( spec );
  if( !spread.has_value() ) return res;
  InhomogeneousTileSpreadSpec const tile_spec{
    .source_spec = spec,
    .spread      = std::move( *spread ),
    .tiles       = config.tiles };
  return render_plan_for_tile_inhomogeneous( tile_spec );
}

} // namespace rn
