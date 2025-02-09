/****************************************************************
**spread-algo.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-01-12.
*
* Description: Algorithm for doing the icon spread.
*
*****************************************************************/
#include "spread-algo.hpp"

using namespace std;

namespace rn {

namespace {

int64_t bounds_for_output( Spread const& spread ) {
  if( spread.rendered_count == 0 ) return 0;
  return ( spread.rendered_count - 1 ) * spread.spacing +
         spread.spec.trimmed.len;
}

int64_t total_bounds( SpreadSpecs const& specs,
                      Spreads const& spreads ) {
  int64_t total = 0;
  for( auto const& spread : spreads.spreads )
    total += bounds_for_output( spread );
  total += specs.group_spacing *
           std::max( spreads.spreads.size() - 1, 0ul );
  return total;
}

Spread* find_largest( Spreads& spreads ) {
  Spread* largest        = {};
  int64_t largest_bounds = 0;
  for( Spread& spread : spreads.spreads ) {
    int64_t const bounds = bounds_for_output( spread );
    if( bounds > largest_bounds ) {
      largest_bounds = bounds;
      largest        = &spread;
    }
  }
  return largest;
}

Spread* find_largest_with_non_adjacent_spacing(
    Spreads& spreads ) {
  Spread* largest        = {};
  int64_t largest_bounds = 0;
  for( Spread& spread : spreads.spreads ) {
    if( spread.spacing <= 1 ) continue;
    int64_t const bounds = bounds_for_output( spread );
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
    auto& spread          = spreads.spreads.emplace_back();
    spread.spec           = spec;
    spread.spacing        = 1;
    int const min_count   = spec.count > 0 ? 1 : 0;
    int const max_count   = spec.count;
    spread.rendered_count = std::clamp(
        int( specs.bounds * double( spec.count ) / total_count ),
        min_count, max_count );
  }

  auto const decrement_count_for_largest = [&] {
    Spread* const largest = find_largest( spreads );
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
    CHECK_GE( spec.count, 0 );
  }
  CHECK_EQ( specs.specs.size(), spreads.spreads.size() );

  auto const total_count = compute_total_count( specs );
  if( total_count == 0 ) return spreads;

  auto const decrement_spacing_for_largest = [&] {
    Spread* const largest =
        find_largest_with_non_adjacent_spacing( spreads );
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

  for( auto const& spread : spreads.spreads ) {
    CHECK_LE( spread.rendered_count, spread.spec.count );
  }
  return spreads;
}

bool requires_label( Spread const& spread ) {
  if( spread.spacing <= 2 ) return true;
  if( spread.rendered_count > 100 ) return true;
  return false;
}

void adjust_rendered_count_for_progress_count(
    Spread& spread, int const progress_count ) {
  int const total_count = spread.spec.count;
  CHECK_LE( progress_count, total_count );
  int& rendered_count = spread.rendered_count;
  CHECK_LE( rendered_count, total_count );
  if( rendered_count == 0 ) return;
  if( rendered_count == total_count ) {
    CHECK_LE( progress_count, rendered_count );
    rendered_count = progress_count;
    return;
  }
  // Rendered count is less than total count here, so therefore
  // we need to also recompute progress_count to be proportional.
  rendered_count = clamp( 0,
                          int( double( progress_count ) /
                               total_count * rendered_count ),
                          rendered_count );
  if( progress_count > 0 && rendered_count == 0 )
    rendered_count = 1;
}

} // namespace rn
