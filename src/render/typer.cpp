/****************************************************************
**typer.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-11.
*
* Description: Rendering ASCII text from sprites.
*
*****************************************************************/
#include "typer.hpp"

// render
#include "ascii-font.hpp"
#include "atlas.hpp"
#include "painter.hpp"
#include "textometer.hpp"

using namespace std;

namespace rr {

namespace {

using ::base::nothing;
using ::gfx::interval;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** Typer
*****************************************************************/
Typer::Typer( Painter painter, AsciiFont const& ascii_font,
              TextLayout const& layout, point start,
              pixel color )
  : painter_( std::move( painter ) ),
    ascii_font_( ascii_font ),
    textometer_( painter_.atlas(), ascii_font_ ),
    layout_( layout ),
    line_start_( start ),
    pos_( start ),
    color_( color ) {
  set_color( color_ );
}

Typer::Typer( Painter painter, AsciiFont const& ascii_font,
              TextLayout const& layout )
  : Typer( std::move( painter ), ascii_font, layout, point{},
           pixel::black() ) {}

ITextometer const& Typer::textometer() const {
  return textometer_;
}

// NOTE: the logic in this function must be kept in sync with the
// logic in the Textometer class which does the layout calcula-
// tions (e.g. spacing) of rendered text.
void Typer::write_char_impl( Painter& painter, char const c ) {
  if( c == '\n' ) {
    size const csize = ascii_font_.char_size();
    line_start_.y +=
        csize.h + textometer_.spacing_between_lines( layout_ );
    pos_ = line_start_;
    return;
  }
  interval const spacing =
      textometer_.trimmed_horizontally( layout_, c );
  painter.draw_sprite( ascii_font_.atlas_id_for_char( c ),
                       pos_.moved_left( spacing.start ) );
  pos_.x +=
      spacing.len + textometer_.spacing_between_chars( layout_ );
}

void Typer::newline() { write_char_impl( painter_, '\n' ); }

void Typer::write( char const c ) {
  auto mods               = painter_.mods();
  mods.fixed_color        = color_;
  Painter colored_painter = painter_.with_mods( mods );
  write_char_impl( colored_painter, c );
}

void Typer::write( string_view line ) {
  // Precompute mods/painter for efficiency.
  auto mods               = painter_.mods();
  mods.fixed_color        = color_;
  Painter colored_painter = painter_.with_mods( mods );
  for( char c : line ) write_char_impl( colored_painter, c );
}

void Typer::writeln( string_view const line ) {
  write( line );
  newline();
}

gfx::size Typer::dimensions_for_line(
    string_view const text ) const {
  return Textometer( painter_.atlas(), ascii_font_ )
      .dimensions_for_line( layout_, string( text ) );
}

void Typer::move_frame_by( gfx::size how_much ) {
  pos_ += how_much;
  line_start_ += how_much;
}

Typer Typer::with_frame_offset( gfx::size how_much ) {
  Typer res = *this;
  res.move_frame_by( how_much );
  return res;
}

} // namespace rr
