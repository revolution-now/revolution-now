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

// refl
#include "refl/to-str.hpp"

// base
#include "base/logger.hpp"
#include "base/to-str-ext-std.hpp"

// C++ standard library
#include <ranges>

using namespace std;

namespace rn {

namespace {

using ::base::maybe;
using ::base::nothing;
using ::std::ranges::views::enumerate;
using ::std::ranges::views::zip;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
bool requires_label( Spread const& spread ) {
  if( spread.rendered_count <= 1 ) return false;
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

// The idea with this algorithm is that we start off with `count`
// icons each one pixel spacing (total spacing; so they are
// mostly overlapping). Then if there is still space remaining in
// the bounds, we try to increase spacing so as to make the icons
// use up all of the space. This is generally not possible with
// uniform spacing, so we need to make the spacing non-uniform.
// We do this by walking upward through all of the possible "fre-
// quencies" at which we can add spacing, i.e. we start at every
// one icon and add as much space between them as possible until
// we would exceed the bounds, then (space permitting) we move to
// the next frequency which is every second icon and increase
// spacing on those until we can't go further, etc. Each time we
// bump the frequency, the total incremental space that gets used
// tends to decrease with each pixel of space added (because
// there are fewer icons whose spacing is being increased). This
// allows us to fine tune the amount of space used by the spread
// to be precisely equal to the input bounds.
//
// Example:
//
//   tile:   Xxxxx (length 5)
//   count:  8
//   bounds: ------------------------------ (30)
//
//   result: frequency 1: spacing 3
//           frequency 2: spacing 1
//           frequency 3: n/a
//           frequeycy 4: spacing 1
//           rest:        n/a
//
//           XxxXxxXxxxXxxXxxxxXxxXxxxXxxxx
//
//           The largest space is between X's #4 and #5, since 4
//           is a multiple of all three of the frequencies chosen
//           (1, 2, 4).
//
// That said, there are two cases where it is not possible to
// precisely cover the bounds, namely when there are two few
// icons (since they can't have more than one pixel of gap be-
// tween them) and when there are too many such that even low-
// ering the spacing to one does not allow them to fit. These two
// cases are handled first.
maybe<ProgressSpread> compute_icon_spread_progress_bar(
    ProgressSpreadSpec const& spec ) {
  maybe<ProgressSpread> res;
  int const bounds      = spec.bounds;
  int const tile_width  = spec.spread_spec.trimmed.len;
  int const max_spacing = tile_width + 1;
  int64_t const count   = spec.spread_spec.count;

  if( count <= 0 ) {
    res.emplace();
    return res;
  }
  CHECK_GE( count, 0 );

  // Find total space taken when there is uniform spacing.
  auto const uniform = [&]( int64_t const spacing ) {
    return std::max( ( count - 1 ) * spacing + tile_width,
                     int64_t{ 0 } );
  };

  if( int64_t const min_bounds_needed = uniform( 1 );
      min_bounds_needed > bounds )
    return res;

  // From here on we should always be able to return a value.
  auto& progress_spread = res.emplace();

  if( int64_t const max_bounds_possible = uniform( max_spacing );
      max_bounds_possible <= bounds ) {
    progress_spread.spacings = { { 1, max_spacing } };
    return res;
  }

  using P = ProgressSpreadSpacing;
  vector<P> spacings;

  auto const space_used_by_elem = [&]( P const& elem ) {
    CHECK_GT( elem.mod, 0 );
    int64_t const count_affected = ( count - 1 ) / elem.mod;
    return elem.spacing * count_affected;
  };

  auto const space_used = [&]( P const& proposed ) {
    int64_t total = 0;
    for( auto const& elem : spacings )
      total += space_used_by_elem( elem );
    total += space_used_by_elem( proposed );
    total += tile_width;
    return total;
  };

  auto const space_remaining_with = [&]( P const& proposed ) {
    return bounds - space_used( proposed );
  };

  auto const space_remaining = [&] {
    P const empty{ .mod = 1, .spacing = 0 };
    return space_remaining_with( empty );
  };

  P next{ .mod = 1, .spacing = 0 };
  while( true ) {
    // This expected number of iterations of this loop should be
    // on the order of the width of a typical tile, e.g. 10^1.
    while( true ) {
      auto proposed = next;
      ++proposed.spacing;
      if( space_remaining_with( proposed ) < 0 ) break;
      next = proposed;
    }
    if( next.spacing > 0 ) spacings.push_back( next );
    int64_t const remaining = space_remaining();
    CHECK_GE( remaining, 0 );
    if( remaining == 0 ) break;
    auto& mod = next.mod;
    // Add 1 to remaining because we want to to make `remaining`
    // splits in the count, and to do that we divide it into `re-
    // maining+1` segments. Add 1 to mod so that we guarantee
    // that we never repeat checking the same mod, since that'd
    // mess things up.
    mod = std::max( count / ( remaining + 1 ), mod + 1 );
    CHECK_GT( mod, 1 );
    next.spacing = 0;
    // Circuit breaker. Should never happen, but will prevent an
    // infinite loop in case something has gone wrong. Note that
    // if we're breaking out of the loop at this point, it is
    // likely that we haven't exhausted all of the available
    // bounds, and thus we will check fail below.
    if( mod > count ) {
      lg.error(
          "internal failure to exhaust bounds in tile spread "
          "algo for spec: {}",
          spec );
      break;
    }
  }

  // A spread does not have to use the full bounds if there are
  // too few elements such that at one pixel apart they cannot
  // fill the entire bounds. However, such a situation should
  // have been detected at the start of this function and handled
  // with an early return. Likewise, the situation where the min-
  // imum spacing still exceeds the bounds (which can also cause
  // this to trigger) should have been caught above as well.
  CHECK( space_remaining() == 0,
         "spec: {} resulted in a spread that did not precisely "
         "fit the full bounds.",
         spec );
  res->spacings = std::move( spacings );
  return res;
}

maybe<InhomogeneousSpread> compute_icon_spread_inhomogeneous(
    InhomogeneousSpreadSpec const& spec ) {
  maybe<InhomogeneousSpread> res;
  auto& spread = res.emplace();

  auto const occupied = [&] {
    int p           = 0;
    int right_most  = 0;
    int const count = ssize( spec.widths );
    for( auto const [idx, width] : enumerate( spec.widths ) ) {
      right_most = std::max( right_most, p + width );
      if( idx < count - 1 )
        p += std::min( width + spec.max_spacing,
                       spread.max_total_spacing );
      else
        p += width;
    }
    return right_most;
  };

  auto const fits = [&] { return occupied() <= spec.bounds; };

  spread.max_total_spacing = numeric_limits<int>::max();
  if( fits() ) return res;
  spread.max_total_spacing = 0;
  if( !fits() ) {
    res.reset();
    return res;
  }

  while( fits() ) ++spread.max_total_spacing;
  --spread.max_total_spacing;
  CHECK_GE( spread.max_total_spacing, 0 );
  CHECK( fits() );
  return res;
}

} // namespace rn
