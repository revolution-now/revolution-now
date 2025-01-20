/****************************************************************
**spread.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Algorithm for doing the icon spread.
*
*****************************************************************/
#include "spread.hpp"

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

int64_t bounds_for_output( SpreadSpec const& spec,
                           Spread const& spread ) {
  if( spread.rendered_count == 0 ) return 0;
  return ( spread.rendered_count - 1 ) * spread.spacing +
         spec.trimmed.len;
}

int64_t total_bounds( SpreadSpecs const& specs,
                      Spreads const& spreads ) {
  int64_t total = 0;
  for( auto&& [spec, spread] :
       rv::zip( specs.specs, spreads.spreads ) )
    total += bounds_for_output( spec, spread );
  total += specs.group_spacing *
           std::max( spreads.spreads.size() - 1, 0ul );
  return total;
}

Spread* find_largest( SpreadSpecs const& specs,
                      Spreads& spreads ) {
  Spread* largest        = {};
  int64_t largest_bounds = 0;
  for( auto&& [spec, spread] :
       rv::zip( specs.specs, spreads.spreads ) ) {
    int64_t const bounds = bounds_for_output( spec, spread );
    if( bounds > largest_bounds ) {
      largest_bounds = bounds;
      largest        = &spread;
    }
  }
  return largest;
}

int64_t compute_total_count( SpreadSpecs const& specs ) {
  int64_t total = 0;
  for( SpreadSpec const& spec : specs.specs )
    total += spec.count;
  return total;
}

// This method is called when we can't fit all the icons within
// the bounds even when all of their spacings are reduced to one.
// We will force it to fit in the bounds by rewriting the counts
// of each spread, and it seems that a good way to do this would
// be to just rewrite all them to be a fraction of the whole in
// proportion to their original desired counts.
Spreads compute_compressed_proportionate(
    SpreadSpecs const& specs ) {
  auto const total_count = compute_total_count( specs );
  // Should have already handled this case. This can't be zero in
  // this method because we will shortly use this value as a de-
  // nominator.
  CHECK_GT( total_count, 0 );
  Spreads spreads;
  for( SpreadSpec const& spec : specs.specs ) {
    auto& spread        = spreads.spreads.emplace_back();
    spread.spec         = spec;
    spread.spacing      = 1;
    int const min_count = spec.count > 0 ? 1 : 0;
    spread.rendered_count =
        std::max( int( specs.bounds *
                       ( double( spec.count ) / total_count ) ),
                  min_count );
  }

  auto const decrement_count_for_largest = [&] {
    Spread* const largest = find_largest( specs, spreads );
    if( !largest ) return false;
    // Not sure if this could happen here since "largest" depends
    // on other parameters such as spacing and width, which the
    // user could pass in as zero, so good to be defensive but
    // not check-fail.
    if( largest->rendered_count <= 0 ) return false;
    --largest->rendered_count;
    return true;
  };

  // At this point we should have gotten things approximately
  // right, though we may have overshot just a bit due to the
  // width of the icons and spacing between them, so we'll just
  // decrement counts until we get under the bounds, which should
  // only be a handful of iterations as it only scales with the
  // width of the sprites and not any icon counts.
  while( total_bounds( specs, spreads ) > specs.bounds )
    if( !decrement_count_for_largest() )
      // We can't decrease the count of any of the spreads any
      // further, so just return what we have, which should have
      // all zero counts.
      break;

  return spreads;
}

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
Spreads compute_icon_spread( SpreadSpecs const& specs ) {
  Spreads spreads;

  // First compute the default spreads.
  for( SpreadSpec const& spec : specs.specs ) {
    auto& spread          = spreads.spreads.emplace_back();
    spread.spec           = spec;
    spread.spacing        = spec.trimmed.len + 1;
    spread.rendered_count = spec.count;
  }
  CHECK_EQ( specs.specs.size(), spreads.spreads.size() );

  auto const total_count = compute_total_count( specs );
  if( total_count == 0 ) return spreads;

  auto const decrement_spacing_for_largest = [&] {
    Spread* const largest = find_largest( specs, spreads );
    // Not sure if this could happen here since "largest" depends
    // on other parameters such as spacing and width, which the
    // user could pass in as zero, so good to be defensive but
    // not check-fail.
    if( !largest ) return false;
    if( largest->spacing <= 1 ) return false;
    --largest->spacing;
    return true;
  };

  while( total_bounds( specs, spreads ) > specs.bounds ) {
    if( !decrement_spacing_for_largest() ) {
      // We can't decrease the spacing of any of the spreads any
      // further, so we have to just change their counts.
      spreads = compute_compressed_proportionate( specs );
      break;
    }
  }

  return spreads;
}

bool requires_label( Spread const& spread ) {
  if( spread.spacing <= 1 ) return true;
  if( spread.rendered_count > 10 ) return true;
  if( spread.spec.trimmed.len < 8 ) return true;
  return false;
}

TileSpread rendered_tile_spread(
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
