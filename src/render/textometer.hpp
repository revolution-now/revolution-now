/****************************************************************
**textometer.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-03-08.
*
* Description: Implementation of ITextometer.
*
*****************************************************************/
#pragma once

// render
#include "itextometer.hpp"

namespace rr {

struct AsciiFont;
struct AtlasMap;

/****************************************************************
** Textometer
*****************************************************************/
// NOTE: the logic in this class must be kept in sync with the
// logic in Typer which does the rendering whose layout (e.g.
// spacing) should be the same as is assumed by this class.
struct Textometer final : public ITextometer {
  Textometer( AtlasMap const& atlas,
              AsciiFont const& ascii_font );

  gfx::size dimensions_for_line(
      TextLayout const& layout,
      std::string const& text ) const override;

  int spacing_between_chars(
      TextLayout const& layout ) const override;

  int spacing_between_lines(
      TextLayout const& layout ) const override;

  gfx::interval trimmed_horizontally(
      TextLayout const& layout, char const c ) const override;

  int font_height() const override;

 private:
  gfx::rect trimmed_area_for( char const c ) const;

  AtlasMap const& atlas_;
  AsciiFont const& ascii_font_;
};

} // namespace rr
