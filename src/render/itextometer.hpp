/****************************************************************
**itextometer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-08.
*
* Description: Interface for font character placement and
*              bounding boxes of rendered text.
*
*****************************************************************/
#pragma once

// render
#include "text-layout.rds.hpp"

// gfx
#include "gfx/cartesian.hpp"

// C++ standard library
#include <string>

namespace rr {

/****************************************************************
** ITextometer
*****************************************************************/
// TODO: add font name into each method.
struct ITextometer {
  virtual ~ITextometer() = default;

  // Get the size in pixels of a line of text when rendered.
  virtual gfx::size dimensions_for_line(
      TextLayout const& layout,
      std::string const& text ) const = 0;

  // Note for monospace layouts the value of this may not be so
  // meaningful because for monospace fonts the whitespace that
  // already exists in the character sprites is not stripped
  // away. But you can still call it in that case.
  virtual int spacing_between_chars(
      TextLayout const& layout ) const = 0;

  virtual int spacing_between_lines(
      TextLayout const& layout ) const = 0;

  virtual gfx::interval trimmed_horizontally(
      TextLayout const& layout, char const c ) const = 0;

  virtual int font_height() const = 0;
};

} // namespace rr
