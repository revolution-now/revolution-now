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

// Cdr
#include "cdr/ext.hpp"

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

  // Implement cdr::ToCanonical.
  friend cdr::value to_canonical( cdr::converter& conv,
                                  pixel const&    o,
                                  cdr::tag_t<pixel> );

  // Implement cdr::FromCanonical.
  friend cdr::result<pixel> from_canonical( cdr::converter& conv,
                                            cdr::value const& v,
                                            cdr::tag_t<pixel> );

  static pixel red() {
    return { .r = 255, .g = 0, .b = 0, .a = 255 };
  }
  static pixel yellow() {
    return { .r = 255, .g = 255, .b = 0, .a = 255 };
  }
  static pixel green() {
    return { .r = 0, .g = 255, .b = 0, .a = 255 };
  }
  static pixel blue() {
    return { .r = 0, .g = 0, .b = 255, .a = 255 };
  }
  static pixel white() {
    return { .r = 255, .g = 255, .b = 255, .a = 255 };
  }
  static pixel black() {
    return { .r = 0, .g = 0, .b = 0, .a = 255 };
  }
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
