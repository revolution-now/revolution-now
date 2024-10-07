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
Typer::Typer( Painter painter, AsciiFont const& ascii_font,
              point start, pixel color )
  : painter_( std::move( painter ) ),
    ascii_font_( ascii_font ),
    line_start_( start ),
    pos_( start ),
    color_( color ),
    scale_( nothing ) {
  set_color( color_ );
}

gfx::size Typer::scale() const {
  return scale_.value_or( ascii_font_.char_size() );
}

void Typer::write_char_impl( Painter& painter, char c ) {
  if( c == '\n' ) {
    line_start_.y +=
        scale().h + rendered_text_line_spacing_pixels();
    pos_ = line_start_;
    return;
  }
  size delta = scale();
  painter.draw_sprite_scale(
      ascii_font_.atlas_id_for_char( c ),
      rect{ .origin = pos_, .size = delta } );
  pos_.x += delta.w;
}

void Typer::newline() { write_char_impl( painter_, '\n' ); }

void Typer::write( char const c ) {
  auto mods               = painter_.mods();
  mods.fixed_color        = color_;
  Painter colored_painter = painter_.with_mods( mods );
  write_char_impl( colored_painter, c );
}

void Typer::write( std::string_view line ) {
  // Precompute mods/painter for efficiency.
  auto mods               = painter_.mods();
  mods.fixed_color        = color_;
  Painter colored_painter = painter_.with_mods( mods );
  for( char c : line ) write_char_impl( colored_painter, c );
}

gfx::size Typer::dimensions_for_line(
    std::string_view msg ) const {
  gfx::size res = scale();
  res.w *= msg.length();
  return res;
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
