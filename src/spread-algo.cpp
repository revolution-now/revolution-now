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

// C++ standard library
#include <ranges>

using namespace std;

namespace rv = std::ranges::views;

namespace rn {

namespace {

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
  if( spread.spacing <= 2 ) return true;
  if( spread.rendered_count > 100 ) return true;
  if( spread.spec.trimmed.len < 8 ) return true;
  return false;
}

} // namespace rn
