/****************************************************************
**pixel.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2021-12-11.
*
* Description: Representation of an image or texture pixel color.
*
*****************************************************************/
#pragma once

// rcl
#include "rcl/ext.hpp"

// base
#include "base/adl-tag.hpp"
#include "base/maybe.hpp"

// C++ standard library
#include <string>
#include <string_view>

namespace gfx {

/****************************************************************
** pixel
*****************************************************************/
// Standard RGBA form.  Can be freely passed by value.
struct pixel {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  uint8_t a = 0;

  pixel() = default;
  pixel( uint8_t r_, uint8_t g_, uint8_t b_ )
    : r( r_ ), g( g_ ), b( b_ ), a( 255 ) {}
  pixel( uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ )
    : r( r_ ), g( g_ ), b( b_ ), a( a_ ) {}

  auto operator<=>( pixel const& ) const = default;

  double luminosity() const;

  uint32_t to_uint32() const;

  // #NNNNNN with N in [0-9A-F] and optionally #NNNNNNNN
  // if with_alpha = true;
  std::string to_string( bool with_alpha = false ) const;

  friend void to_str( pixel const& o, std::string& out,
                      base::ADL_t );

  // Parses a string of the form 'NNNNNN[NN]' where N is:
  // [0-9a-fA-F]. The optional two digits at the end represent
  // alpha. If these are omitted then alpha will be set to 255.
  static base::maybe<pixel> parse_from_hex(
      std::string_view hex );

  // A random color.
  static pixel random();

  // Returns this color but with changed alpha.
  pixel with_alpha( uint8_t a_new ) const;

  // Will generate colors that can act as a "subtle" highlight
  // and shadow of this color. Shadow, from what knowledgable
  // people seem to say, means dark and less saturated, while
  // highlight means the opposite. One "iteration" will yield a
  // color with only a subtle difference. If more of a difference
  // is desired then a larger iteration number can be specified.
  // The iterations are such that, for example:
  //
  //   color.shaded( 3 ) == color.shaded().shaded().shaded()
  //
  // Calling with iterations == 0 returns *this.
  //
  // TODO: these need to cache results.
  pixel highlighted( int iterations = 1 ) const;
  pixel shaded( int iterations = 1 ) const;

  // This is for deserializing from Rcl config files.
  friend rcl::convert_err<pixel> convert_to( rcl::value const& v,
                                             rcl::tag<pixel> );

  static pixel red() { return { 255, 0, 0, 255 }; }
  static pixel yellow() { return { 255, 255, 0, 255 }; }
  static pixel green() { return { 0, 255, 0, 255 }; }
  static pixel blue() { return { 0, 0, 255, 255 }; }
  static pixel white() { return { 255, 255, 255, 255 }; }
  static pixel black() { return { 0, 0, 0, 255 }; }
  static pixel banana();
  static pixel wood();
};

// Takes the average of each component.
pixel mix( pixel first, pixel second );

// So that we can pass it by value.
static_assert( sizeof( pixel ) == 4 );

/****************************************************************
** Other stuff
*****************************************************************/
struct pixel_hsl {
  double  h = 0; // hue [0..360]
  double  s = 0; // saturation [0, 1]
  double  l = 0; // lightness [0, 1]
  uint8_t a = 0; // alpha

  auto operator<=>( pixel_hsl const& ) const = default;
};

pixel_hsl to_HSL( pixel const& rgb );
pixel     to_RGB( pixel_hsl const& hsl );

} // namespace gfx

/****************************************************************
** std::hash
*****************************************************************/
namespace std {

template<>
struct hash<::gfx::pixel> {
  auto operator()( ::gfx::pixel const& p ) const noexcept {
    return hash<uint32_t>{}( p.to_uint32() );
  }
};

} // namespace std
