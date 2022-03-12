/****************************************************************
**text.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-11.
*
* Description: Rendering ASCII text from sprites.
*
*****************************************************************/
#include "text.hpp"

// render
#include "ascii-font.hpp"
#include "atlas.hpp"
#include "painter.hpp"

using namespace std;

namespace rr {

namespace {

using ::base::nothing;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** Typer
*****************************************************************/
Typer::Typer( Painter& painter, AsciiFont const& ascii_font,
              point start, pixel color )
  : painter_( painter ),
    ascii_font_( ascii_font ),
    line_start_( start ),
    pos_( start ),
    color_( color ),
    scale_( nothing ) {}

gfx::size Typer::scale() const {
  return scale_.value_or( ascii_font_.char_size() );
}

void Typer::write( char c ) {
  if( c == '\n' ) {
    line_start_.y += scale().h;
    pos_ = line_start_;
    return;
  }
  size delta = scale();
  painter_.draw_silhouette_scale(
      ascii_font_.atlas_id_for_char( c ),
      rect{ .origin = pos_, .size = delta }, color_ );
  pos_.x += delta.w;
}

void Typer::newline() { write( '\n' ); }

void Typer::write( std::string_view line ) {
  for( char c : line ) write( c );
}

} // namespace rr
