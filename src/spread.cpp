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

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

namespace {

using ::gfx::oriented_point;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::size;

int64_t bounds_for_output( IconSpreadSpec const& spec,
                           IconSpread const& spread ) {
  if( spread.count == 0 ) return 0;
  return ( spread.count - 1 ) * spread.spacing + spec.width;
}

int64_t total_bounds( IconSpreadSpecs const& specs,
                      IconSpreads const& spreads ) {
  int64_t total = 0;
  for( auto&& [spec, spread] :
       rv::zip( specs.specs, spreads.spreads ) )
    total += bounds_for_output( spec, spread );
  total += specs.group_spacing *
           std::max( spreads.spreads.size() - 1, 0ul );
  return total;
}

IconSpread* find_largest( IconSpreadSpecs const& specs,
                          IconSpreads& spreads ) {
  IconSpread* largest    = {};
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

int64_t compute_total_count( IconSpreadSpecs const& specs ) {
  int64_t total = 0;
  for( IconSpreadSpec const& spec : specs.specs )
    total += spec.count;
  return total;
}

// This method is called when we can't fit all the icons within
// the bounds even when all of their spacings are reduced to one.
// We will force it to fit in the bounds by rewriting the counts
// of each spread, and it seems that a good way to do this would
// be to just rewrite all them to be a fraction of the whole in
// proportion to their original desired counts.
IconSpreads compute_compressed_proportionate(
    IconSpreadSpecs const& specs ) {
  auto const total_count = compute_total_count( specs );
  // Should have already handled this case. This can't be zero in
  // this method because we will shortly use this value as a de-
  // nominator.
  CHECK_GT( total_count, 0 );
  IconSpreads spreads;
  spreads.group_spacing = specs.group_spacing;
  for( IconSpreadSpec const& spec : specs.specs ) {
    auto& spread        = spreads.spreads.emplace_back();
    spread.spacing      = 1;
    spread.width        = spec.width;
    int const min_count = spec.count > 0 ? 1 : 0;
    spread.count =
        std::max( int( specs.bounds *
                       ( double( spec.count ) / total_count ) ),
                  min_count );
  }

  auto const decrement_count_for_largest = [&] {
    IconSpread* const largest = find_largest( specs, spreads );
    if( !largest ) return false;
    // Not sure if this could happen here since "largest" depends
    // on other parameters such as spacing and width, which the
    // user could pass in as zero, so good to be defensive but
    // not check-fail.
    if( largest->count <= 0 ) return false;
    --largest->count;
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
IconSpreads compute_icon_spread( IconSpreadSpecs const& specs ) {
  IconSpreads spreads;

  // First compute the default spreads.
  spreads.group_spacing = specs.group_spacing;
  for( IconSpreadSpec const& spec : specs.specs ) {
    auto& spread   = spreads.spreads.emplace_back();
    spread.spacing = spec.width + 1;
    spread.width   = spec.width;
    spread.count   = spec.count;
  }
  CHECK_EQ( specs.specs.size(), spreads.spreads.size() );

  auto const total_count = compute_total_count( specs );
  if( total_count == 0 ) return spreads;

  auto const decrement_spacing_for_largest = [&] {
    IconSpread* const largest = find_largest( specs, spreads );
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

bool requires_label( IconSpread const& spread ) {
  if( spread.spacing <= 1 ) return true;
  if( spread.count > 10 ) return true;
  if( spread.width < 8 ) return true;
  return false;
}

TileSpreadRenderPlan rendered_tile_spread(
    TileSpreads const& tile_spreads ) {
  TileSpreadRenderPlan res;
  point p = {};
  for( TileSpread const& tile_spread : tile_spreads.spreads ) {
    if( tile_spread.icon_spread.count == 0 ) continue;
    int const tile_h = sprite_size( tile_spread.tile ).h;
    if( auto const& label_spec = tile_spread.label;
        label_spec.has_value() )
      res.labels.push_back( SpreadLabelRenderPlan{
        .options = *label_spec,
        .text    = to_string( tile_spread.icon_spread.count ),
        .p       = oriented_point{
                .anchor =
              p.moved_right( 2 ).moved_down( tile_h ).moved_up(
                  2 ),
                .placement = label_spec->placement.value_or(
              e_cdirection::sw ) } } );
    for( int i = 0; i < tile_spread.icon_spread.count; ++i ) {
      point const p_drawn =
          p.moved_left( tile_spread.opaque_start );
      res.tiles.push_back( { tile_spread.tile, p_drawn } );
      p.x += tile_spread.icon_spread.spacing;
    }
    if( tile_spread.icon_spread.count > 0 )
      p.x += std::max( ( tile_spread.icon_spread.width -
                         tile_spread.icon_spread.spacing ),
                       0 );
    p.x += tile_spreads.group_spacing;
  }
  return res;
}

void draw_rendered_icon_spread(
    rr::Renderer& renderer, point const origin,
    TileSpreadRenderPlan const& plan ) {
  for( auto const& [tile, p] : plan.tiles )
    render_sprite( renderer, p.origin_becomes_point( origin ),
                   tile );
  for( auto const& plan : plan.labels )
    render_text_line_with_background(
        renderer, plan.text,
        plan.p.origin_becomes_point( origin ),
        plan.options.color_fg.value_or( pixel::white() ),
        plan.options.color_bg.value_or( pixel::black() ),
        plan.options.text_padding.value_or( 1 ) );
}

} // namespace rn
