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

using ::base::maybe;
using ::base::nothing;
using ::std::ranges::views::zip;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool requires_label( Spread const& spread ) {
  if( spread.spacing <= 1 ) return true;
  if( spread.rendered_count >= 50 ) return true;
  return false;
}

bool requires_label( ProgressSpread const& spread ) {
  if( spread.spacings.size() >= 1 &&
      spread.spacings[0].mod == 1 &&
      spread.spacings[0].spacing == 1 )
    return true;
  return false;
}

void adjust_rendered_count_for_progress_count(
    SpreadSpec const& spec, Spread& spread,
    int const progress_count_uncapped ) {
  int const total_count = spec.count;
  int const progress_count =
      std::min( progress_count_uncapped, total_count );
  int& rendered_count = spread.rendered_count;
  // This should theoretically not be necessary, but let's just
  // be defensive.
  rendered_count = std::min( rendered_count, total_count );
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
        Spread{ .rendered_count = spec.count,
                .spacing        = S( spec, final_spacing ) } );
  CHECK_EQ( res.spreads.size(), specs.specs.size() );
  // Sanity check.
  for( auto const [spec, spread] :
       zip( specs.specs, res.spreads ) )
    CHECK_EQ( spec.count, spread.rendered_count );
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

#define DEBUG_PRINT( ... )

maybe<ProgressSpread> compute_icon_spread_progress_bar(
    ProgressSpreadSpec const& spec ) {
  maybe<ProgressSpread> res;
  int const bounds      = spec.bounds;
  int const tile_width  = spec.spread_spec.trimmed.len;
  int const max_spacing = tile_width + 1;
  int const count       = spec.spread_spec.count;

  if( count <= 0 ) {
    res.emplace();
    return res;
  }
  CHECK_GE( count, 0 );

  auto const space_used_with_uniform_spacing =
      [&]( int const spacing ) {
        return std::max( ( count - 1 ) * spacing + tile_width,
                         0 );
      };

  if( int const min_bounds_needed =
          space_used_with_uniform_spacing( 1 );
      min_bounds_needed > bounds )
    return res;

  // From here on we should always be able to return a value.
  auto& progress_spread    = res.emplace();
  progress_spread.spacings = { { 1, 1 } };

  if( int const max_bounds_possible =
          space_used_with_uniform_spacing( max_spacing );
      max_bounds_possible <= bounds ) {
    progress_spread.spacings = { { 1, max_spacing } };
    return res;
  }

  vector<ProgressSpreadSpacing> spacings;

  auto const space_used_by_elem =
      [&]( ProgressSpreadSpacing const& elem ) {
        CHECK_GT( elem.mod, 0 );
        int const count_affected = ( count - 1 ) / elem.mod;
        return elem.spacing * count_affected;
      };

  auto const space_used =
      [&]( ProgressSpreadSpacing const& proposed ) {
        int total = 0;
        for( auto const& elem : spacings )
          total += space_used_by_elem( elem );
        total += space_used_by_elem( proposed );
        total += tile_width;
        return total;
      };

  auto const space_remaining_with_proposed =
      [&]( ProgressSpreadSpacing const& proposed ) {
        maybe<int> remaining;
        remaining = bounds - space_used( proposed );
        if( *remaining < 0 ) remaining.reset();
        return remaining;
      };

  auto const space_remaining = [&] {
    ProgressSpreadSpacing const empty{ .mod = 1, .spacing = 0 };
    return space_remaining_with_proposed( empty );
  };

  int const max_mod = count - 1;
  ProgressSpreadSpacing next{ .mod = 1, .spacing = 0 };
  int circuit_breaker = 10000; // TODO
  DEBUG_PRINT( "spec: {}", spec );
  // Makes sure we don't try a mod more than once.
  int last_mod = 1;
  while( true ) {
    DEBUG_PRINT( "--------------------------------------" );
    DEBUG_PRINT( "curcuit_breaker: {}", circuit_breaker );
    DEBUG_PRINT( "next: {}", next );
    while( true ) {
      auto const remaining =
          space_remaining_with_proposed( ProgressSpreadSpacing{
            .mod = next.mod, .spacing = next.spacing + 1 } );
      if( !remaining.has_value() ) break;
      ++next.spacing;
    }
    DEBUG_PRINT( "found: {}", next );
    if( next.spacing > 0 ) spacings.push_back( next );
    auto const remaining = space_remaining();
    DEBUG_PRINT( "remaining: {}", remaining );
    if( !remaining.has_value() )
      // TODO: needed?
      break;
    if( *remaining == 0 ) break;
    int const next_mod =
        std::max( 1 + count / ( *remaining + 1 ), last_mod + 1 );
    last_mod = next_mod;
    DEBUG_PRINT( "next_mod: {}", next_mod );
    if( next_mod > max_mod ) break;
    if( next_mod == 1 ) break;
    CHECK_GT( next_mod, 0 );
    next =
        ProgressSpreadSpacing{ .mod = next_mod, .spacing = 0 };
    CHECK_GT( circuit_breaker--, 0 );
  }

  res->spacings = std::move( spacings );
  return res;
}

} // namespace rn
