/****************************************************************
**spread-render.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-20.
*
* Description: Creates and renders spreads of tiles.
*
*****************************************************************/
#include "spread-render.hpp"

// Revolution Now
#include "text.hpp"
#include "tiles.hpp"

// config
#include "config/ui.rds.hpp"

// render
#include "render/itextometer.hpp"
#include "render/renderer.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/spread-algo.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/range-lite.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

namespace {

namespace rl = ::base::rl;

using ::base::maybe;
using ::base::nothing;
using ::gfx::e_cdirection;
using ::gfx::interval;
using ::gfx::oriented_point;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

rr::TextLayout const kLabelTextLayout;

void add_label( rr::ITextometer const& textometer,
                interval const& trimmed,
                int const rendered_count, int const spacing,
                rect const tiles_all, size const tile_size,
                int const label_count, int const x_offset,
                SpreadLabelOptions const& options,
                vector<SpreadLabelRenderPlan>& out ) {
  rect const first_tile_rect = [&] {
    rect res = tiles_all.with_size(
        size{ .w = trimmed.len, .h = tile_size.h } );
    res.origin.x += x_offset;
    return res;
  }();
  rect const placement_rect    = first_tile_rect;
  e_cdirection const placement = [&] {
    if( !options.placement.has_value() )
      return config_ui.tile_spreads.default_label_placement;
    SWITCH( *options.placement ) {
      CASE( left_middle_adjusted ) {
        if( rendered_count == 1 ) return e_cdirection::c;
        if( spacing < trimmed.len ) return e_cdirection::w;
        return e_cdirection::c;
      }
      CASE( in_first_tile ) { return in_first_tile.placement; }
    }
  }();
  string const label_text      = to_string( label_count );
  size const padded_label_size = [&] {
    size const label_size = textometer.dimensions_for_line(
        kLabelTextLayout, label_text );
    int const padding = options.text_padding.value_or(
        config_ui.tile_spreads.label_text_padding );
    return size{ .w = label_size.w + padding * 2,
                 .h = label_size.h + padding * 2 };
  }();
  out.push_back( SpreadLabelRenderPlan{
    .options = options,
    .text    = label_text,
    .bounds  = {
       .origin = gfx::centered_at( padded_label_size,
                                   placement_rect, placement ),
       .size   = padded_label_size } } );
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
TileSpreadRenderPlans render_plan_for_tile_spread(
    rr::ITextometer const& textometer,
    TileSpreadSpecs const& tile_spreads ) {
  TileSpreadRenderPlans plans;
  point p                       = {};
  bool const has_required_label = [&] {
    for( auto const& [spec, tile_spread] : tile_spreads.spreads )
      if( requires_label( tile_spread.icon_spread ) )
        return true;
    return false;
  }();
  auto const label_options_impl =
      [&]( Spread const& icon_spread,
           SpreadLabelOptions const& label_opts
               ATTR_LIFETIMEBOUND )
      -> maybe<SpreadLabelOptions const&> {
    SWITCH( tile_spreads.label_policy ) {
      CASE( never ) { return nothing; }
      CASE( always ) { return label_opts; }
      CASE( auto_decide ) {
        bool const force =
            has_required_label && auto_decide.viral;
        if( force || requires_label( icon_spread ) )
          return label_opts;
        return nothing;
      }
    }
  };
  auto const label_options =
      [&]( TileSpreadSpec const& tile_spread )
      -> maybe<SpreadLabelOptions const&> {
    return label_options_impl( tile_spread.icon_spread,
                               tile_spread.label_opts );
  };
  auto const label_options_overlay =
      [&]( TileSpreadSpec const& tile_spread )
      -> maybe<SpreadLabelOptions const&> {
    CHECK( tile_spread.overlay_tile.has_value() );
    return label_options_impl(
        tile_spread.icon_spread,
        tile_spread.overlay_tile->label_opts );
  };
  for( auto const& [spec, tile_spread] : tile_spreads.spreads ) {
    CHECK_LE( tile_spread.icon_spread.rendered_count,
              spec.count );
    if( tile_spread.icon_spread.rendered_count == 0 ) continue;
    auto& plan                     = plans.plans.emplace_back();
    point const p_start            = p;
    auto const& tile_trimmed_len   = spec.trimmed.len;
    auto const& tile_trimmed_start = spec.trimmed.start;
    size const tile_size = sprite_size( tile_spread.tile );
    bool const is_overlapping =
        ( tile_spread.icon_spread.rendered_count > 1 ) &&
        tile_spread.icon_spread.spacing < spec.trimmed.len;
    struct OverlayStart {
      int idx         = {};
      point p_start   = {};
      int label_count = {};
    };
    maybe<OverlayStart> overlay;
    for( int i = 0; i < tile_spread.icon_spread.rendered_count;
         ++i ) {
      point const p_drawn = p.moved_left( tile_trimmed_start );
      plan.tiles.push_back(
          TileRenderPlan{ .tile       = tile_spread.tile,
                          .where      = p_drawn,
                          .is_overlay = false } );
      // Must appear just after the tile it is overlaying.
      if( tile_spread.overlay_tile.has_value() &&
          i >= tile_spread.overlay_tile->starting_position ) {
        if( !overlay.has_value() ) {
          overlay.emplace();
          overlay->idx     = i;
          overlay->p_start = p;
          overlay->label_count =
              tile_spread.overlay_tile->label_count.value_or(
                  spec.count - i );
        }
        // We need to place the overlay tile against the middle
        // left wall of the trimmed (opaque) part of the sprite.
        rect const base_tile_trimmed_rect{
          .origin = p,
          .size   = sprite_size( tile_spread.tile )
                      .with_w( tile_trimmed_len ) };
        point const p_overlay_drawn = [&] {
          // If the underlying tiles are overlapping then make
          // sure that the trimmed X start of the overlay tile
          // aligns with the trimmed X start of the base sprite.
          // This is important so that the overlay sprite is
          // still visible even when the spread is only one pixel
          // apart.
          auto const alignment =
              is_overlapping ? e_cdirection::w : e_cdirection::c;
          int const left_shift =
              is_overlapping
                  ? trimmed_area_for(
                        tile_spread.overlay_tile->tile )
                        .horizontal_slice()
                        .start
                  : 0;
          point res = gfx::centered_at(
              sprite_size( tile_spread.overlay_tile->tile ),
              base_tile_trimmed_rect, alignment );
          res.x -= left_shift;
          return res;
        }();
        plan.tiles.push_back( TileRenderPlan{
          .tile       = tile_spread.overlay_tile->tile,
          .where      = p_overlay_drawn,
          .is_overlay = true } );
      }
      p.x += tile_spread.icon_spread.spacing;
    }
    if( tile_spread.icon_spread.rendered_count > 0 )
      p.x += std::max(
          ( tile_trimmed_len - tile_spread.icon_spread.spacing ),
          0 );
    rect const tiles_all = [&] {
      rect res{ .origin = p_start };
      for( auto const& tile : plan.tiles )
        if( !tile.is_overlay )
          res = res.uni0n( rect{
            .origin =
                tile.where + size{ .w = tile_trimmed_start },
            .size = { .w = tile_trimmed_len,
                      .h = tile_size.h } } );
      return res;
    }();
    plan.bounds = tiles_all;
    // Need to do the label after the tiles but before we add the
    // group spacing so that we know the total rect occupied by
    // the tiles.
    int const primary_label_count =
        tile_spread.label_count.value_or(
            as_const( overlay )
                .member( &OverlayStart::idx )
                .value_or( spec.count ) );
    if( primary_label_count > 0 ) {
      auto const options = label_options( tile_spread );
      if( options.has_value() )
        add_label( textometer, spec.trimmed,
                   tile_spread.icon_spread.rendered_count,
                   tile_spread.icon_spread.spacing, tiles_all,
                   tile_size, primary_label_count,
                   /*x_offset=*/0, *options, plan.labels );
    }
    if( overlay.has_value() && overlay->label_count > 0 ) {
      auto const options = label_options_overlay( tile_spread );
      if( options.has_value() )
        add_label( textometer, spec.trimmed,
                   tile_spread.icon_spread.rendered_count,
                   tile_spread.icon_spread.spacing, tiles_all,
                   tile_size, overlay->label_count,
                   /*x_offset=*/overlay->p_start.x - p_start.x,
                   *options, plan.labels );
    }
    p.x += tile_spreads.group_spacing;
  }
  // Populate total bounds.
  rect bounds;
  for( auto const& plan : plans.plans )
    bounds = bounds.uni0n( plan.bounds );
  plans.bounds = bounds.size;
  return plans;
}

TileSpreadRenderPlan render_plan_for_tile_progress_spread(
    rr::ITextometer const& textometer,
    ProgressTileSpreadSpec const& tile_spec ) {
  TileSpreadRenderPlan plan;
  point p = {};
  bool const has_required_label =
      requires_label( tile_spec.progress_spread );
  auto const label_options =
      [&]() -> maybe<SpreadLabelOptions const&> {
    SWITCH( tile_spec.label_policy ) {
      CASE( never ) { return nothing; }
      CASE( always ) { return tile_spec.label_opts; }
      CASE( auto_decide ) {
        bool const force =
            has_required_label && auto_decide.viral;
        if( force ||
            requires_label( tile_spec.progress_spread ) )
          return tile_spec.label_opts;
        return nothing;
      }
    }
  };
  point const p_start = p;
  auto const& trimmed =
      tile_spec.source_spec.spread_spec.trimmed;
  auto const& tile_trimmed_len   = trimmed.len;
  auto const& tile_trimmed_start = trimmed.start;
  size const tile_size           = sprite_size( tile_spec.tile );
  int const rendered_count       = tile_spec.rendered_count;
  CHECK_LE( rendered_count,
            tile_spec.source_spec.spread_spec.count );
  if( rendered_count == 0 ) return plan;
  maybe<int> first_spacing;
  for( int i = 0; i < rendered_count; ++i ) {
    point const p_drawn = p.moved_left( tile_trimmed_start );
    plan.tiles.push_back(
        TileRenderPlan{ .tile       = tile_spec.tile,
                        .where      = p_drawn,
                        .is_overlay = false } );
    for( auto const& [mod, spacing] :
         tile_spec.progress_spread.spacings ) {
      CHECK_GT( mod, 0 );
      if( ( i + 1 ) % mod == 0 ) p.x += spacing;
    }
    if( !first_spacing.has_value() )
      first_spacing = p.x - p_start.x;
  }
  rect const tiles_all = [&] {
    rect res{ .origin = p_start };
    for( auto const& tile : plan.tiles )
      if( !tile.is_overlay )
        res = res.uni0n( rect{
          .origin = tile.where + size{ .w = tile_trimmed_start },
          .size   = { .w = tile_trimmed_len,
                      .h = tile_size.h } } );
    return res;
  }();
  plan.bounds = tiles_all;
  if( auto const options = label_options();
      options.has_value() ) {
    int const label_count = tile_spec.label_count.value_or(
        tile_spec.source_spec.spread_spec.count );
    add_label(
        textometer, tile_spec.source_spec.spread_spec.trimmed,
        tile_spec.rendered_count,
        first_spacing.value_or( numeric_limits<int>::max() ),
        tiles_all, tile_size, label_count,
        /*x_offset=*/0, *options, plan.labels );
  }
  return plan;
}

maybe<TileSpreadRenderPlan>
render_plan_for_tile_uncompressed_spread(
    UncompressedTileSpreadSpec const& tile_spec ) {
  maybe<TileSpreadRenderPlan> res;
  auto& plan = res.emplace();
  plan.tiles.reserve( tile_spec.tiles.size() );
  point p;
  for( TileWithOptions const& tile_info : tile_spec.tiles ) {
    interval const iv =
        trimmed_area_for( tile_info.tile ).horizontal_slice();
    point const render_p = p.moved_left( iv.start );
    plan.tiles.push_back(
        TileRenderPlan{ .tile       = tile_info.tile,
                        .where      = render_p,
                        .is_overlay = false,
                        .is_greyed  = tile_info.greyed } );
    int const delta = iv.len + 1;
    p.x += delta;
  }
  rect const tiles_all = [&] {
    rect bounds;
    for( auto const& plan : plan.tiles )
      bounds = bounds.uni0n(
          trimmed_area_for( plan.tile )
              .origin_becomes_point( plan.where ) );
    return bounds;
  }();
  plan.bounds = tiles_all;
  if( plan.bounds.size.w > tile_spec.bounds ) res.reset();
  return res;
}

void replace_first_n_tiles( TileSpreadRenderPlans& plans,
                            int const n_replace,
                            e_tile const from,
                            e_tile const to ) {
  int countdown = n_replace;
  for( auto& plan : plans.plans ) {
    for( auto& tile : plan.tiles ) {
      if( tile.is_overlay ) continue;
      if( tile.tile != from )
        // Sometimes there may not be as many `from` tiles as we
        // expect, which can happen if there was not enough space
        // and the spread algo had to reduce the number of tiles
        // emitted in order to fit.
        return;
      if( countdown-- == 0 ) return;
      tile.tile = to;
    }
  }
}

void draw_rendered_icon_spread(
    rr::Renderer& renderer, point const origin,
    TileSpreadRenderPlan const& plan,
    TileSpreadRenderOptions const& options ) {
  for( auto const [idx, tile_plan] :
       rl::all( plan.tiles ).enumerate() ) {
    if( options.suppress.has_value() &&
        *options.suppress == idx )
      continue;
    auto const& [tile, p, _, greyed] = tile_plan;
    if( options.shadow.has_value() ) {
      SCOPED_RENDERER_MOD_SET( painter_mods.fixed_color,
                               options.shadow->color );
      render_sprite(
          renderer,
          p.origin_becomes_point( origin ).moved_right(
              options.shadow->offset ),
          tile );
    }
    render_sprite_dulled( renderer,
                          p.origin_becomes_point( origin ), tile,
                          greyed );
  }
  for( auto const& label : plan.labels )
    render_text_line_with_background(
        renderer, kLabelTextLayout, label.text,
        oriented_point{
          .anchor =
              label.bounds.origin.origin_becomes_point( origin ),
          // This is always nw here because the placement
          // calcu- lation has already been done, so the point
          // we are given is always the nw.
          .placement = gfx::e_cdirection::nw },
        label.options.color_fg.value_or(
            config_ui.tile_spreads.default_label_fg_color ),
        label.options.color_bg.value_or(
            config_ui.tile_spreads.default_label_bg_color ),
        label.options.text_padding.value_or(
            config_ui.tile_spreads.label_text_padding ),
        config_ui.tile_spreads.bg_box_has_corners );
}

void draw_rendered_icon_spread(
    rr::Renderer& renderer, point const origin,
    TileSpreadRenderPlans const& plans ) {
  for( auto const& plan : plans.plans )
    draw_rendered_icon_spread( renderer, origin, plan );
}

} // namespace rn
