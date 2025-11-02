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
#include <algorithm>
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

using ::base::maybe;
using ::gfx::interval;
using ::gfx::pixel;
using ::gfx::rect;
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
    CHECK( !plan.tiles.empty() );
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
namespace detail {

e_tile choose_x_tile_for( e_tile const tile ) {
  int const trimmed_w = trimmed_area_for( tile ).size.w;
  switch( trimmed_w ) {
    case 0:
      return e_tile::red_x_6;
    case 1:
      return e_tile::red_x_6;
    case 2:
      return e_tile::red_x_6;
    case 3:
      return e_tile::red_x_6;
    case 4:
      return e_tile::red_x_6;
    case 5:
      return e_tile::red_x_6;
    case 6:
      return e_tile::red_x_6;
    case 7:
      return e_tile::red_x_6;
    case 8:
      return e_tile::red_x_8;
    case 9:
      return e_tile::red_x_8;
    case 10:
      return e_tile::red_x_8;
    case 11:
      return e_tile::red_x_8;
    case 12:
      return e_tile::red_x_12;
    default:
      return e_tile::red_x_12;
  }
}

} // namespace detail

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
        overlay_tile.tile =
            detail::choose_x_tile_for( config_it->tile );
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
  if( plans.plans.empty() )
    // This can happen when the rendered count is zero.
    return TileSpreadRenderPlan{};
  CHECK_EQ( plans.plans.size(), 1u );
  return std::move( plans.plans[0] );
}

TileSpreadRenderPlan build_fixed_tile_spread(
    FixedTileSpreadConfig const& config ) {
  FixedTileSpreadSpec const tile_spec{
    .tile           = config.tile,
    .rendered_count = config.rendered_count,
    .spacing        = config.spacing };
  return render_plan_for_tile_fixed_spread( tile_spec );
}

TileSpreadRenderPlan build_inhomogeneous_tile_spread(
    rr::ITextometer const& textometer,
    InhomogeneousTileSpreadConfig const& config_unprocessed ) {
  TileSpreadRenderPlan res;
  InhomogeneousTileSpreadConfig const config = [&] {
    InhomogeneousTileSpreadConfig res = config_unprocessed;
    if( config.sort_tiles )
      sort( res.tiles.begin(), res.tiles.end(),
            []( TileWithOptions const& l,
                TileWithOptions const& r ) {
              return trimmed_area_for( l.tile )
                         .horizontal_slice()
                         .len > trimmed_area_for( r.tile )
                                    .horizontal_slice()
                                    .len;
            } );
    return res;
  }();
  if( config.tiles.empty() ) return res;
  // Our strategy here is to do the tile spread with a constant
  // tile in order to produce a result with all of the fields
  // filled out, then we will replace the tiles and then adjust
  // the spacing to account for the fact that the tiles will have
  // differently sized trimmed areas. We need to use the first
  // tile otherwise if we use a different tile (which may have a
  // different trimmed width) it might cause the first tile to be
  // placed at an incorrect offset.
  e_tile const first_tile = config.tiles[0].tile;
  TileSpreadConfig const tile_spread_config{
    .tile    = TileSpread{ .tile  = first_tile,
                           .count = int( config.tiles.size() ) },
    .options = config.options };
  res = build_tile_spread( textometer, tile_spread_config );
  // Restore the true set of (varying) tiles. This `zip` should
  // correctly handle the case where there are fewer tiles ren-
  // dered than requested.
  for( auto const [tile_w_opts, tile] :
       rv::zip( config.tiles, res.tiles ) ) {
    tile.tile      = tile_w_opts.tile;
    tile.is_greyed = tile_w_opts.greyed;
  }
  // Need to recompute bounds now that we've moved things.
  auto const bounds = []( TileSpreadRenderPlan const& plan ) {
    rect bounds;
    for( auto const& tile_plan : plan.tiles )
      bounds = bounds.uni0n(
          trimmed_area_for( tile_plan.tile )
              .origin_becomes_point( tile_plan.where ) );
    return bounds;
  };
  // Expand to the right if the tiles that can be further
  // apart.
  auto const expand = [&] {
    TileSpreadRenderPlan scaled = res;
    while( true ) {
      for( int delta = 0; auto& tile_render_plan : scaled.tiles )
        tile_render_plan.where.x += delta++;
      rect const bounds_scaled = bounds( scaled );
      if( bounds_scaled.size.w > config.options.bounds ) break;
      if( bounds_scaled == bounds( res ) ) break;
      res = scaled;
    }
  };
  // Contract if the tiles exceed bounds.
  auto const contract = [&] {
    while( true ) {
      int const old_width = bounds( res ).size.w;
      if( old_width <= config.options.bounds ) break;
      for( int delta = 0; auto& tile_render_plan : res.tiles )
        tile_render_plan.where.x -= delta++;
      int const new_width = bounds( res ).size.w;
      if( new_width == old_width ) break;
    }
  };
  // Move left any tiles that are spaced too far apart.
  auto const squeeze = [&] {
    for( int accum = 0, pos = numeric_limits<int>::max();
         auto& tile_plan : res.tiles ) {
      tile_plan.where.x -= accum;
      interval const iv =
          trimmed_area_for( tile_plan.tile ).horizontal_slice();
      int const start = tile_plan.where.x + iv.start;
      int const delta = std::max( 0, start - pos );
      tile_plan.where.x -= delta;
      accum += delta;
      pos = tile_plan.where.x + iv.start + iv.len +
            config.max_spacing.value_or( 1 );
    }
  };

  // The steps below and their ordering are carefully chosen so
  // that we maximum spacing of the icons but never violate the
  // contract of this function:
  //   1. The bounds must never be exceeded.
  //   2. The true spacing between the trimmed sections of adja-
  //      cent tiles must be respected.
  // Note that in some cases it is not possible to fit the result
  // within the bounds if the bounds are not large enough to hold
  // even one tile. But in most normal cases that is not a con-
  // cern.

  // At this point they are all uniformly spaced, though they may
  // not fit within bounds (which can happen if the reference
  // tile we chose above was slim). So first do a compression to
  // ensure they fit within bounds.
  contract();

  // Now, just in case they are spaced too closely (which can
  // happen if the reference tile we chose above was wider than
  // average) and there is more room to uniformly expand them, do
  // that now.
  expand();

  // We now have the tiles fit within the bounds and expanded to
  // maximum spacing subject to the bounds. However, we need to
  // clip any that are spaced too far apart. This is because the
  // uniform spacing of the icons won't take into account the
  // true apparent spacing given that the icons are not of uni-
  // form width.
  squeeze();

  // At this point we have the icons fit within the bounds and
  // without violating max true spacing between them. Since the
  // last time we did an "expand" they were uniformly space and
  // since now some of them may have less space between them do
  // to the squeeze we just did, there could be room to uniformly
  // expand again.
  expand();

  // One more squeeze to enforce max spacing between adjacent
  // trimmed areas in case it was violated on the last expand.
  squeeze();

  // Need to recompute bounds now that we've moved things.
  res.bounds = bounds( res );
  // NOTE: there are edge cases where res.bounds might not actu-
  // ally fit in config.options.bounds; this can happen when the
  // target bounds are not large enough to hold the first tile.
  // Or another scenario is that the bounds were enought to hold
  // two of the first (reference) tile, then when we replace the
  // reference tiles with the real tiles, the second tile was
  // larger and caused the bounds to be exceeded, and the con-
  // traction wasn't able to fix it.
  return res;
}

} // namespace rn
