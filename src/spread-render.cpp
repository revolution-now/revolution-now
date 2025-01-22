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
TileSpread render_plan_for_tile_spread(
    TileSpreadSpecs const& tile_spreads ) {
  TileSpread res;
  point p                       = {};
  bool const has_required_label = [&] {
    for( TileSpreadSpec const& tile_spread :
         tile_spreads.spreads )
      if( requires_label( tile_spread.icon_spread ) )
        return true;
    return false;
  }();
  auto const label_options = [&]( Spread const& icon_spread )
      -> maybe<SpreadLabelOptions const&> {
    SWITCH( tile_spreads.label_policy ) {
      CASE( never ) { return nothing; }
      CASE( always ) { return always.opts; }
      CASE( auto_decide ) {
        bool const force =
            has_required_label && auto_decide.viral;
        if( force || requires_label( icon_spread ) )
          return auto_decide.opts;
        return nothing;
      }
    }
  };
  for( TileSpreadSpec const& tile_spread :
       tile_spreads.spreads ) {
    if( tile_spread.icon_spread.rendered_count == 0 ) continue;
    point const p_start = p;
    for( int i = 0; i < tile_spread.icon_spread.rendered_count;
         ++i ) {
      point const p_drawn = p.moved_left(
          tile_spread.icon_spread.spec.trimmed.start );
      res.tiles.push_back( { tile_spread.tile, p_drawn } );
      p.x += tile_spread.icon_spread.spacing;
    }
    if( tile_spread.icon_spread.rendered_count > 0 )
      p.x +=
          std::max( ( tile_spread.icon_spread.spec.trimmed.len -
                      tile_spread.icon_spread.spacing ),
                    0 );
    // Need to do the label after the tiles but before we add the
    // group spacing so that we know the total rect occupied by
    // the tiles.
    auto add_label = [&]( SpreadLabelOptions const& options ) {
      int const tile_h = sprite_size( tile_spread.tile ).h;
      rect const tiles_all{
        .origin = p_start,
        .size   = { .w = p.x - p_start.x, .h = tile_h } };
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
      res.labels.push_back( SpreadLabelRenderPlan{
        .options = options,
        .text    = label_text,
        .where   = gfx::centered_at(
            padded_label_size, tiles_all.with_edges_removed( 2 ),
            placement ),
      } );
    };
    label_options( tile_spread.icon_spread ).visit( add_label );
    p.x += tile_spreads.group_spacing;
  }
  return res;
}

void draw_rendered_icon_spread( rr::Renderer& renderer,
                                point const origin,
                                TileSpread const& plan ) {
  for( auto const& [tile, p] : plan.tiles )
    render_sprite( renderer, p.origin_becomes_point( origin ),
                   tile );
  for( auto const& plan : plan.labels )
    render_text_line_with_background(
        renderer, plan.text,
        oriented_point{
          .anchor = plan.where.origin_becomes_point( origin ),
          // This is always nw here because the placement calcu-
          // lation has already been done, so the point we are
          // given is always the nw.
          .placement = gfx::e_cdirection::nw },
        plan.options.color_fg.value_or(
            config_ui.tile_spreads.default_label_fg_color ),
        plan.options.color_bg.value_or(
            config_ui.tile_spreads.default_label_bg_color ),
        plan.options.text_padding.value_or(
            config_ui.tile_spreads.label_text_padding ),
        config_ui.tile_spreads.bg_box_has_corners );
}

} // namespace rn
