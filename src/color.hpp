/****************************************************************
**color.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-12-17.
*
* Description: All things color.
*
*****************************************************************/
#pragma once

#include "core-config.hpp"

// Revolution Now
#include "aliases.hpp"
#include "geo-types.hpp"

// c++ standard library
#include <cstdint>
#include <string_view>
#include <tuple>
#include <vector>

namespace rn {

// Standard RGBA form.  Can be freely passed by value.
struct Color {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;

  Color() {}
  Color( uint8_t r_, uint8_t g_, uint8_t b_ )
    : r( r_ ), g( g_ ), b( b_ ), a( 255 ) {}
  Color( uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ )
    : r( r_ ), g( g_ ), b( b_ ), a( a_ ) {}

  double luminance() const;

  auto to_tuple() const { return std::tuple( r, g, b, a ); }

  // #NNNNNN with N in [0-9A-F] and optionally #NNNNNNNN
  // if with_alpha = true;
  std::string to_string( bool with_alpha = false ) const;

  // Parses a string of the form 'NNNNNN[NN]' where N is:
  // [0-9a-fA-F]. The optional two digits at the end represent
  // alpha. If these are omitted then alpha will be set to 255.
  static Opt<Color> parse_from_hex( std::string_view hex );

  // A random color.
  static Color random();

  // Abseil hashing API.
  template<typename H>
  friend H AbslHashValue( H h, Color const& c ) {
    return H::combine( std::move( h ), c.to_tuple() );
  }

  bool operator<( Color const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
  bool operator==( Color const& rhs ) const {
    return to_tuple() == rhs.to_tuple();
  }

  static Color red() { return {255, 0, 0, 255}; }
  static Color green() { return {0, 255, 0, 255}; }
  static Color blue() { return {0, 0, 255, 255}; }
  static Color white() { return {255, 255, 255, 255}; }
  static Color black() { return {0, 0, 0, 255}; }
};

// So that we can pass it by value.
static_assert( sizeof( Color ) == 4 );

// Sorts colors in a quasi-human way.
void hsl_bucketed_sort( Vec<Color>& colors );

// Removes colors with a saturation below a threshold.
void remove_greys( Vec<Color>& colors );

// Bucket colors first by hue, then by saturation. Note that the
// colors will not be sorted in any way by this function; it will
// only partition them as they are ordered. NOTE: Although
// sorting is not a precondition for this function, the caller
// will likely want to first sort the colors using the
// hsl_bucketed_sort before calling this function in order to get
// meaningful results.
Vec<Vec<Vec<Color>>> hsl_partition( Vec<Color> const& colors );

// Will remove colors that are redundant or approximately
// redunant in order to meet the target count. It will always
// return a number of colors that is >= min_count so long as
// there are at least that many to begin with.
Vec<Color> coursen( Vec<Color> const& colors, int min_count );

// Load the image file(s) and scan every pixel and compile a list
// of unique colors. Then, if a target number of colors is
// specified, try to reduce the number of colors to achieve
// approximately the target number. The algorithm will try its
// best to achieve this number, but typically the set of returned
// colors may have a bit more or less. Also, the order of colors
// returned is unspecified.
std::vector<Color> extract_palette(
    std::string const& glob,
    Opt<int>           target_num_colors = std::nullopt );

// Just for testing. Show each color in a small square in a grid
// on screen.
void show_palette( Vec<Color> const& colors );
void show_palette( Vec<Vec<Color>> const& colors );
void show_palette( Vec<Vec<Vec<Color>>> const& colors );

} // namespace rn
