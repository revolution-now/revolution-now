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

using ::base::maybe;
using ::base::nothing;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool requires_label( Spread const& spread ) {
  if( spread.spacing <= 1 ) return true;
  if( spread.rendered_count >= 50 ) return true;
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
    rendered_count = std::max( progress_count, 0 );
    return;
  }
  // Rendered count is less than total count here, so therefore
  // we need to also recompute progress_count to be proportional.
  double const progress_percent =
      double( progress_count ) / total_count;
  rendered_count =
      clamp( 0, int( rendered_count * progress_percent ),
             rendered_count );
  if( progress_count > 0 && rendered_count == 0 )
    rendered_count = 1;
}

maybe<Spreads> compute_icon_spread( SpreadSpecs const& specs ) {
  auto const total_count = [&] {
    int64_t total = 0;
    for( SpreadSpec const& spec : specs.specs )
      total += spec.count;
    return total;
  }();
  int64_t const total_trimmed = [&] {
    int64_t res = 0;
    for( SpreadSpec const& spec : specs.specs )
      if( spec.count > 0 ) res += spec.trimmed.len;
    return res;
  }();
  // This will be the number of non-empty spreads.
  int64_t const num_spreads = [&] {
    int64_t res = 0;
    for( SpreadSpec const& spec : specs.specs )
      if( spec.count > 0 ) ++res;
    return res;
  }();

  // One can derive this formula for the total space used given a
  // proposed (max) value of spacing:
  //
  //   total_space = (count_1-1)*S_spec_1
  //               + (count_2-1)*S_spec_2
  //               + (count_3-1)*S_spec_3
  //               + (count_4-1)*S_spec_4
  //               + ...
  //               + total_trimmed (for non-empty groups)
  //               + group_spacing*(num_nonempty_spreads-1)
  //
  // Where S is a function that gives the spacing for a given
  // spread given the max uniform spacing being proposed.
  //
  // We then keep incrementing the proposed spacing until we
  // cover the maximum area possible in the allowed bounds.
  //
  auto const S = [&]( SpreadSpec const& spec,
                      int64_t const max_spacing ) -> int {
    return std::min( max_spacing,
                     int64_t{ spec.trimmed.len + 1 } );
  };
  auto const total_space = [&]( int64_t const spacing ) {
    int64_t res = total_trimmed +
                  specs.group_spacing * ( num_spreads - 1 );
    for( SpreadSpec const& spec : specs.specs )
      if( spec.count > 0 )
        res += ( spec.count - 1 ) * S( spec, spacing );
    return res;
  };
  int64_t const final_spacing = [&] {
    int64_t proposed_spacing = 0;
    int64_t most_space_used  = {};
    while( true ) {
      int64_t const space_used =
          total_space( proposed_spacing + 1 );
      if( space_used > specs.bounds ) break;
      // Eventually we may get to a point where we haven't yet
      // covered all of the allowed bounds but where we can't
      // space out any of the spreads any more (since the empty
      // space between them can be at most one). In that case
      // we're done.
      if( space_used == most_space_used ) break;
      most_space_used = space_used;
      ++proposed_spacing;
      // Circuit breaker, just in case something funny happens
      // with overflow and we go into an infinite loop.
      if( proposed_spacing > specs.bounds * 2 ) {
        proposed_spacing = 0;
        break;
      }
    }
    return proposed_spacing;
  }();
  if( final_spacing == 0 && total_count > 0 )
    // This means that we couldn't fit everything in the allowed
    // bounds even with a spacing of 1. So switch to a different
    // algorithm that is made to handle that situation.
    return nothing;
  Spreads res;
  res.spreads.reserve( specs.specs.size() );
  for( SpreadSpec const& spec : specs.specs )
    res.spreads.push_back(
        Spread{ .spec           = spec,
                .rendered_count = spec.count,
                .spacing        = S( spec, final_spacing ) } );
  CHECK_EQ( res.spreads.size(), specs.specs.size() );
  // Sanity check.
  for( Spread const& spread : res.spreads )
    CHECK_EQ( spread.spec.count, spread.rendered_count );
  return res;
}

Spreads compute_icon_spread_proportionate(
    SpreadSpecs const& specs ) {
  Spreads spreads;

  auto const total_count = [&] {
    int64_t total = 0;
    for( SpreadSpec const& spec : specs.specs )
      total += spec.count;
    return total;
  }();

  int const num_nonempty_spreads = [&] {
    int non_empty_count = 0;
    for( SpreadSpec const& spec : specs.specs )
      if( spec.count > 0 ) ++non_empty_count;
    return non_empty_count;
  }();

  // We already know that we will be spacing the tiles one pixel
  // apart. Given that, find the total number of slots that we
  // have available after subtracting overhead space from the
  // total bounds (e.g. spaces between groups, and spaces occu-
  // pied by the last tile in a spread).
  int const available_slots = [&] {
    int res = specs.bounds;
    for( SpreadSpec const& spec : specs.specs )
      if( spec.count > 0 )
        res -= std::max( spec.trimmed.len - 1, 0 );
    if( num_nonempty_spreads > 0 )
      res -= specs.group_spacing * ( num_nonempty_spreads - 1 );
    return std::max( res, 0 );
  }();

  for( SpreadSpec const& spec : specs.specs ) {
    auto& spread   = spreads.spreads.emplace_back();
    spread.spec    = spec;
    spread.spacing = 1;
    if( spec.count == 0 ) continue;
    int const max_count = spec.count;
    CHECK_GT( total_count, 0 );
    double const percent_occupied =
        double( spec.count ) / total_count;
    spread.rendered_count =
        std::clamp( int( available_slots * percent_occupied ), 1,
                    max_count );
  }

  CHECK_EQ( spreads.spreads.size(), specs.specs.size() );
  return spreads;
}

} // namespace rn
