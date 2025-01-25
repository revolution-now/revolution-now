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

// In the OG sometimes labels can be turned on unconditionally
// (e.g. in the colony view), but even when that does not happen,
// the OG still sometimes puts labels on spreads that it deems to
// be difficult to read, and that's what this function tries to
// replicate by applying some heuristics to decide if the spread
// is such that it would be difficult for the player to read the
// count visually.
[[nodiscard]] bool requires_label( Spread const& spread ) {
  if( spread.spacing <= 1 ) return true;
  if( spread.rendered_count > 10 ) return true;
  if( spread.spec.trimmed.len < 8 ) return true;
  return false;
}

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
    if( tile_spread.icon_spread.rendered_count == 0 ) continue;
    auto& plan          = plans.plans.emplace_back();
    point const p_start = p;
    for( int i = 0; i < tile_spread.icon_spread.rendered_count;
         ++i ) {
      point const p_drawn = p.moved_left(
          tile_spread.icon_spread.spec.trimmed.start );
      plan.tiles.push_back( TileRenderPlan{
        .tile = tile_spread.tile, .where = p_drawn } );
      // Must appear just after the tile it is overlaying.
      if( tile_spread.overlay_tile.has_value() ) {
        // We need to place the overlay tile against the middle
        // left wall of the trimmed (opaque) part of the sprite.
        rect const base_tile_trimmed_rect{
          .origin = p,
          .size   = sprite_size( tile_spread.tile )
                      .with_w( tile_spread.icon_spread.spec
                                   .trimmed.len ) };
        point const p_overlay_drawn = [&] {
          // Make sure that the trimmed X start of the overlay
          // tile aligns with the trimmed X start of the base
          // sprite. This is important so that the overlay sprite
          // is still visible even when the spread is only one
          // pixel apart.
          point res = gfx::centered_at(
              sprite_size( *tile_spread.overlay_tile ),
              base_tile_trimmed_rect, e_cdirection::w );
          res.x -= opaque_area_for( *tile_spread.overlay_tile )
                       .horizontal_slice()
                       .start;
          return res;
        }();
        plan.tiles.push_back(
            TileRenderPlan{ .tile  = *tile_spread.overlay_tile,
                            .where = p_overlay_drawn } );
      }
      p.x += tile_spread.icon_spread.spacing;
    }
    if( tile_spread.icon_spread.rendered_count > 0 )
      p.x +=
          std::max( ( tile_spread.icon_spread.spec.trimmed.len -
                      tile_spread.icon_spread.spacing ),
                    0 );
    int const tile_h = sprite_size( tile_spread.tile ).h;
    rect const tiles_all{
      .origin = p_start,
      .size   = { .w = p.x - p_start.x, .h = tile_h } };
    plan.bounds = tiles_all;
    // Need to do the label after the tiles but before we add the
    // group spacing so that we know the total rect occupied by
    // the tiles.
    auto add_label = [&]( SpreadLabelOptions const& options ) {
      e_cdirection const placement = options.placement.value_or(
          config_ui.tile_spreads.default_label_placement );
      string const label_text =
          to_string( tile_spread.icon_spread.spec.count );
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
        .where   = gfx::centered_at(
            padded_label_size, tiles_all.with_edges_removed( 2 ),
            placement ),
      };
    };
    label_options( tile_spread ).visit( add_label );
    p.x += tile_spreads.group_spacing;
  }
  return plans;
}

void draw_rendered_icon_spread(
    rr::Renderer& renderer, gfx::point origin,
    TileSpreadRenderPlan const& plan ) {
  for( auto const& [tile, p] : plan.tiles )
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
