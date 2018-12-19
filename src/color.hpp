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

struct ColorHSL {
  double  h = 0; // hue [0..360]
  double  s = 0; // saturation [0, 1]
  double  l = 0; // lightness [0, 1]
  uint8_t a = 0; // alpha

  auto to_tuple() const { return std::tuple( h, s, l, a ); }

  bool operator<( ColorHSL const& rhs ) const {
    return to_tuple() < rhs.to_tuple();
  }
};

ColorHSL to_HSL( Color const& rgb );
Color    to_RGB( ColorHSL const& hsl );

// Sorts colors in a quasi-human way.
void hsl_bucketed_sort( Vec<Color>& colors );

// Load the image file and scan every pixel and compile a list
// of unique colors in RGB form.  The order of colors returned
// is unspecified.
std::vector<Color> extract_palette( std::string const& image );

// Partition colors by taking the full range of hue (0-360) and
// partitioning it into segments of size `hue_segment_size` and
// then gathering all colors in each segment.  Result will be
// independent of the ordering of the input colors.
Vec<Vec<Color>> partition_by_hue( Vec<Color> const& colors );

Vec<Color> coursen( Vec<Color> const& colors, int min_count );

Vec<Vec<Color>> coursen( Vec<Vec<Color>> const& colors,
                         int                    min_count );

// Just for testing. Show each color in a small square in a grid
// on screen.
void show_palette( Vec<Color> const& colors );
void show_palette( Vec<Vec<Color>> const& colors );

} // namespace rn
