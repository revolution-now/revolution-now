/****************************************************************
**text.hpp
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

/****************************************************************
** Typer
*****************************************************************/
// This is kind of like a type writer in that it holds a position
// and will advance by the correct number of pixels each time a
// character or newline is written.
struct Typer {
  Typer( Painter painter, AsciiFont const& ascii_font,
         gfx::point start, gfx::pixel color );

  gfx::point current_position_pixels() const { return pos_; }
  gfx::pixel color() const { return color_; }
  gfx::size  scale() const;

  void set_position_pixels( gfx::point where ) { pos_ = where; }
  void set_color( gfx::pixel color ) { color_ = color; }
  void set_scale( gfx::size scale ) { scale_ = scale; }

  void reset_scale() { scale_.reset(); }

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
  Painter          painter_;
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
