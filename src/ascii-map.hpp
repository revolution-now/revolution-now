/****************************************************************
**ascii-map.hpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2026-01-04.
*
* Description: Draws maps to the terminal for tools/testing.
*
*****************************************************************/
#pragma once

// base
#include "base/maybe.hpp"

// C++ standard library
#include <iosfwd>

namespace rn {

/****************************************************************
** Fwd. Decls.
*****************************************************************/
struct MapSquare;
struct RealTerrain;

/****************************************************************
** IAsciiMapColorizer
*****************************************************************/
struct IAsciiMapFormatter {
  virtual ~IAsciiMapFormatter() = default;

  struct Format {
    std::string_view chars;
    base::maybe<std::string_view> color_fg;
    base::maybe<std::string_view> color_bg;
  };

  [[nodiscard]] virtual Format format(
      MapSquare const& above, MapSquare const& below ) const = 0;
};

/****************************************************************
** Public API.
*****************************************************************/
IAsciiMapFormatter const& ascii_map_biome_formatter();

IAsciiMapFormatter const& ascii_map_rivers_formatter();

void print_ascii_map( RealTerrain const& terrain,
                      IAsciiMapFormatter const& formatter,
                      std::ostream& out );

} // namespace rn
