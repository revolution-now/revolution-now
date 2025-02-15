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
#include "render/typer.hpp" // FIXME: remove

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/spread-algo.hpp"

// rds
#include "rds/switch-macro.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::gfx::e_cdirection;
using ::gfx::oriented_point;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
TileSpreadRenderPlans render_plan_for_tile_spread(
    TileSpreadSpecs const& tile_spreads ) {
  TileSpreadRenderPlans plans;
  point p                       = {};
  bool const has_required_label = [&] {
    for( TileSpreadSpec const& tile_spread :
         tile_spreads.spreads )
      if( requires_label( tile_spread.icon_spread ) )
        return true;
    return false;
  }();
  auto const label_options =
      [&]( TileSpreadSpec const& tile_spread )
      -> maybe<SpreadLabelOptions const&> {
    SWITCH( tile_spreads.label_policy ) {
      CASE( never ) { return nothing; }
      CASE( always ) { return tile_spread.label_opts; }
      CASE( auto_decide ) {
        bool const force =
            has_required_label && auto_decide.viral;
        if( force || requires_label( tile_spread.icon_spread ) )
          return tile_spread.label_opts;
        return nothing;
      }
    }
  };
  for( TileSpreadSpec const& tile_spread :
       tile_spreads.spreads ) {
    CHECK_LE( tile_spread.icon_spread.rendered_count,
              tile_spread.icon_spread.spec.count );
    if( tile_spread.icon_spread.rendered_count == 0 ) continue;
    auto& plan          = plans.plans.emplace_back();
    point const p_start = p;
    auto const& tile_trimmed_len =
        tile_spread.icon_spread.spec.trimmed.len;
    auto const& tile_trimmed_start =
        tile_spread.icon_spread.spec.trimmed.start;
    size const tile_size = sprite_size( tile_spread.tile );
    for( int i = 0; i < tile_spread.icon_spread.rendered_count;
         ++i ) {
      point const p_drawn = p.moved_left( tile_trimmed_start );
      plan.tiles.push_back(
          TileRenderPlan{ .tile       = tile_spread.tile,
                          .where      = p_drawn,
                          .is_overlay = false } );
      // Must appear just after the tile it is overlaying.
      if( tile_spread.overlay_tile.has_value() ) {
        // We need to place the overlay tile against the middle
        // left wall of the trimmed (opaque) part of the sprite.
        rect const base_tile_trimmed_rect{
          .origin = p,
          .size   = sprite_size( tile_spread.tile )
                      .with_w( tile_trimmed_len ) };
        point const p_overlay_drawn = [&] {
          // Make sure that the trimmed X start of the overlay
          // tile aligns with the trimmed X start of the base
          // sprite. This is important so that the overlay sprite
          // is still visible even when the spread is only one
          // pixel apart.
          point res = gfx::centered_at(
              sprite_size( *tile_spread.overlay_tile ),
              base_tile_trimmed_rect, e_cdirection::w );
          res.x -= trimmed_area_for( *tile_spread.overlay_tile )
                       .horizontal_slice()
                       .start;
          return res;
        }();
        plan.tiles.push_back(
            TileRenderPlan{ .tile  = *tile_spread.overlay_tile,
                            .where = p_overlay_drawn,
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
    auto add_label = [&]( SpreadLabelOptions const& options ) {
      rect const first_tile_rect = tiles_all.with_size(
          size{ .w = tile_spread.icon_spread.spec.trimmed.len,
                .h = tile_size.h } );
      rect const placement_rect = [&] {
        if( !options.placement.has_value() )
          return first_tile_rect;
        SWITCH( *options.placement ) {
          CASE( left_middle_adjusted ) {
            return first_tile_rect;
          }
          CASE( in_first_tile ) { return first_tile_rect; }
          CASE( in_total_rect ) { return tiles_all; }
        }
      }();
      e_cdirection const placement = [&] {
        if( !options.placement.has_value() )
          return config_ui.tile_spreads.default_label_placement;
        SWITCH( *options.placement ) {
          CASE( left_middle_adjusted ) {
            if( tile_spread.icon_spread.rendered_count == 1 )
              return e_cdirection::c;
            if( tile_spread.icon_spread.spacing <
                tile_spread.icon_spread.spec.trimmed.len )
              return e_cdirection::w;
            return e_cdirection::c;
          }
          CASE( in_first_tile ) {
            return in_first_tile.placement;
          }
          CASE( in_total_rect ) {
            return in_total_rect.placement;
          }
        }
      }();
      int const label_count = tile_spread.label_count.value_or(
          tile_spread.icon_spread.spec.count );
      string const label_text      = to_string( label_count );
      size const padded_label_size = [&] {
        size const label_size =
            rr::rendered_text_line_size_pixels( label_text );
        int const padding = options.text_padding.value_or(
            config_ui.tile_spreads.label_text_padding );
        return size{ .w = label_size.w + padding * 2,
                     .h = label_size.h + padding * 2 };
      }();
      plan.label = SpreadLabelRenderPlan{
        .options = options,
        .text    = label_text,
        .where   = gfx::centered_at( padded_label_size,
                                     placement_rect, placement ),
      };
    };
    label_options( tile_spread ).visit( add_label );
    p.x += tile_spreads.group_spacing;
  }
  // Populate total bounds.
  rect bounds;
  for( auto const& plan : plans.plans )
    bounds = bounds.uni0n( plan.bounds );
  plans.bounds = bounds.size;
  return plans;
}

void replace_first_n_tiles( TileSpreadRenderPlans& plans,
                            int const n_replace,
                            e_tile const from,
                            e_tile const to ) {
  int countdown = n_replace;
  for( auto& plan : plans.plans ) {
    for( auto& tile : plan.tiles ) {
      if( tile.is_overlay ) continue;
      if( countdown-- == 0 ) return;
      if( tile.tile != from )
        // Sometimes there may not be as many `from` tiles as we
        // expect, which can happen if there was not enough space
        // and the spread algo had to reduce the number of tiles
        // emitted in order to fit.
        return;
      tile.tile = to;
    }
  }
}

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlan const& plan ) {
  for( auto const& [tile, p, is_overlay] : plan.tiles )
    render_sprite( renderer, p.origin_becomes_point( origin ),
                   tile );
  if( auto const& label = plan.label; label.has_value() ) {
    render_text_line_with_background(
        renderer, label->text,
        oriented_point{
          .anchor = label->where.origin_becomes_point( origin ),
          // This is always nw here because the placement
          // calcu- lation has already been done, so the point
          // we are given is always the nw.
          .placement = gfx::e_cdirection::nw },
        label->options.color_fg.value_or(
            config_ui.tile_spreads.default_label_fg_color ),
        label->options.color_bg.value_or(
            config_ui.tile_spreads.default_label_bg_color ),
        label->options.text_padding.value_or(
            config_ui.tile_spreads.label_text_padding ),
        config_ui.tile_spreads.bg_box_has_corners );
  }
#if 0
  // render
#  include "render/painter.hpp"  // FIXME: remove
#  include "render/renderer.hpp" // FIXME: remove
  renderer.painter().draw_empty_rect(
      plan.bounds.origin_becomes_point( origin ),
      rr::Painter::e_border_mode::inside, gfx::pixel::green() );
#endif
}

void draw_rendered_icon_spread(
    rr::Renderer& renderer, point const origin,
    TileSpreadRenderPlans const& plans ) {
  for( auto const& plan : plans.plans )
    draw_rendered_icon_spread( renderer, origin, plan );
}

} // namespace rn
