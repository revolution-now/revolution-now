/****************************************************************
**typer.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-11.
*
* Description: Rendering ASCII text from sprites.
*
*****************************************************************/
#pragma once

// render
#include "painter.hpp"

// gfx
#include "gfx/cartesian.hpp"
#include "gfx/pixel.hpp"

// base
#include "base/fmt.hpp"

// C++ standard library
#include <string_view>

namespace rr {

struct Painter;
struct AsciiFont;

// FIXME: need to find a better way of doing this. We should be
// using the dimensions_for_line method on the rr::Typer, but we
// can't get access to that in all of the places where we need
// it.
inline gfx::size rendered_text_line_size_pixels(
    std::string_view text ) {
  return gfx::size{ .w = 6 * int( text.size() ), .h = 8 };
}
inline int rendered_text_line_spacing_pixels() { return 1; }

/****************************************************************
** Typer
*****************************************************************/
// This is kind of like a type writer in that it holds a position
// and will advance by the correct number of pixels each time a
// character or newline is written.
struct Typer {
  Typer( Painter painter, AsciiFont const& ascii_font,
         gfx::point start, gfx::pixel color );

  // These are in pixels.
  gfx::point position() const { return pos_; }
  gfx::point line_start() const { return line_start_; }

  gfx::pixel color() const { return color_; }
  gfx::size scale() const;

  void set_color( gfx::pixel color ) { color_ = color; }
  void set_scale( gfx::size scale ) { scale_ = scale; }
  void multiply_scale( int factor );

  void reset_scale() { scale_.reset(); }

  // This will offset the current frame by how_much. That means
  // that both the current text position (in pixels) as well as
  // the start of line will be shifted.
  void move_frame_by( gfx::size how_much );

  // Return a new typer that is the same as this one except with
  // the current frame position offset by `how_much`.
  Typer with_frame_offset( gfx::size how_much );

  // The pixel dimensions of the resulting rectangle were the
  // single-line message to be rendered. If there are newlines in
  // the string they will be treated as any other character.
  gfx::size dimensions_for_line( std::string_view msg ) const;

  // Writes the character to the current position and advances
  // the cursor. When c is '\n' then this is equivalent to
  // calling newline().
  void write( char c );

  // Just loops through the characters and writes them.
  void write( std::string_view line );

  // For convenience.
  template<typename Arg, typename... Rest>
  void write( std::string_view line, Arg&& arg,
              Rest&&... rest ) {
    write( fmt::format( fmt::runtime( line ),
                        std::forward<Arg>( arg ),
                        std::forward<Rest>( rest )... ) );
  }

  // Moves the cursor down to the start of the next line, where
  // the line height is taken as the height of the current text
  // scale.
  void newline();

 private:
  // Writes the character to the current position and advances
  // the cursor. When c is '\n' then this is equivalent to
  // calling newline().
  void write_char_impl( Painter& painter, char c );

  Painter painter_;
  AsciiFont const& ascii_font_;
  // Pixel position of the upper-left of the start of the current
  // line.
  gfx::point line_start_;
  // Pixel position where next character will be written.
  gfx::point pos_;
  gfx::pixel color_;
  // If this has a value then it will be used to override the
  // size of the characters.
  base::maybe<gfx::size> scale_;
};

} // namespace rr
