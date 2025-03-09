/****************************************************************
**textometer.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-08.
*
* Description: Implementation of ITextometer.
*
*****************************************************************/
#include "textometer.hpp"

// render
#include "ascii-font.hpp"
#include "atlas.hpp"

// C++ standard library
#include <numeric>

using namespace std;

namespace rr {

namespace {

using ::gfx::interval;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** Public API.
*****************************************************************/
Textometer::Textometer( AtlasMap const& atlas,
                        AsciiFont const& ascii_font )
  : atlas_( atlas ), ascii_font_( ascii_font ) {}

rect Textometer::trimmed_area_for( char const c ) const {
  int const atlas_id = ascii_font_.atlas_id_for_char( c );
  return atlas_.trimmed_bounds( atlas_id );
}

interval Textometer::trimmed_horizontally(
    TextLayout const& layout, char const c ) const {
  size const csize    = ascii_font_.char_size();
  interval const mono = { .start = 0, .len = csize.w };
  if( layout.monospace ) return mono;
  if( c == ' ' )
    return interval{ .len = std::max( csize.w / 2, 1 ) };
  return trimmed_area_for( c ).horizontal_slice();
}

int Textometer::spacing_between_chars(
    TextLayout const& layout ) const {
  return layout.spacing.value_or( layout.monospace ? 0 : 1 );
}

int Textometer::spacing_between_lines(
    TextLayout const& layout ) const {
  return layout.line_spacing.value_or( 1 );
}

size Textometer::dimensions_for_line(
    TextLayout const& layout, string const& text ) const {
  int const chars_total = accumulate(
      text.begin(), text.end(), 0,
      [&]( int const total, char const c ) {
        return total + trimmed_horizontally( layout, c ).len;
      } );
  int const spacings_total = spacing_between_chars( layout ) *
                             std::max( ssize( text ) - 1, 0l );
  return { .w = chars_total + spacings_total,
           .h = ascii_font_.char_size().h };
}

int Textometer::font_height() const {
  return ascii_font_.char_size().h;
}

} // namespace rr
