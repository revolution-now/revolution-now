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
#include "text-layout.rds.hpp"
#include "textometer.hpp"

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
struct ITextometer;

/****************************************************************
** Typer
*****************************************************************/
// This is kind of like a type writer in that it holds a position
// and will advance by the correct number of pixels each time a
// character or newline is written.
struct Typer {
  Typer( Painter painter, AsciiFont const& ascii_font,
         TextLayout const& layout, gfx::point start,
         gfx::pixel color );

  // For this one you will need to manually set the coordinate
  // and color for it to be useful.
  Typer( Painter painter, AsciiFont const& ascii_font,
         TextLayout const& layout );

  // These are in pixels.
  gfx::point position() const { return pos_; }
  Typer& set_position( gfx::point const p ) {
    pos_ = p;
    return *this;
  }
  gfx::point line_start() const { return line_start_; }

  gfx::pixel color() const { return color_; }
  Typer& set_color( gfx::pixel color ) {
    color_ = color;
    return *this;
  }

  TextLayout& layout() { return layout_; }
  TextLayout const& layout() const { return layout_; }

  // Get an ITextometer object for the current typer.
  ITextometer const& textometer() const;

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

  // Same as above but puts a newline after.
  void writeln( std::string_view line );

  // For convenience.
  template<typename Arg, typename... Rest>
  void write(
      // The type_identity prevents the compiler from using the
      // first arg to try to infer Arg/Rest (which would fail);
      // it will defer that, then when it gets to the end it will
      // have inferred those parameters through other args.
      fmt::format_string<std::type_identity_t<Arg>, Rest...> fmt,
      Arg&& arg, Rest&&... rest ) {
    write( fmt::format( fmt, std::forward<Arg>( arg ),
                        std::forward<Rest>( rest )... ) );
  }

  // For convenience.
  template<typename Arg, typename... Rest>
  void writeln(
      // The type_identity prevents the compiler from using the
      // first arg to try to infer Arg/Rest (which would fail);
      // it will defer that, then when it gets to the end it will
      // have inferred those parameters through other args.
      fmt::format_string<std::type_identity_t<Arg>, Rest...> fmt,
      Arg&& arg, Rest&&... rest ) {
    writeln( fmt::format( fmt, std::forward<Arg>( arg ),
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
  Textometer textometer_;
  TextLayout layout_;
  // Pixel position of the upper-left of the start of the current
  // line.
  gfx::point line_start_;
  // Pixel position where next character will be written.
  gfx::point pos_;
  gfx::pixel color_;
};

} // namespace rr
