/****************************************************************
**sprite-sheet.hpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Describes and loads a sprite sheet into the atlas.
*
*****************************************************************/
#pragma once

// render
#include "ascii-font.hpp"

// Rds
#include "sprite-sheet.rds.hpp"

// base
#include "base/valid.hpp"

// C++ standard library
#include <array>
#include <string>
#include <unordered_map>

namespace gfx {
struct image;
}

namespace rr {

struct AtlasBuilder;

/****************************************************************
** Loading from images.
*****************************************************************/
struct AtlasLoadOutput {
  std::unordered_map<std::string, int> atlas_ids;
  std::unordered_map<int, int> atlas_burrow_ids;
};

namespace detail {

base::valid_or<std::string> load_sprite_sheet(
    AtlasBuilder& builder, gfx::image&& sheet,
    gfx::size sprite_size,
    std::unordered_map<std::string, gfx::point> const& names,
    SpriteSheetOptions const& sheet_options,
    AtlasLoadOutput& output );

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, gfx::image&& sheet );

}

/****************************************************************
** Loading from config info.
*****************************************************************/
base::valid_or<std::string> load_sprite_sheet(
    AtlasBuilder& builder, SpriteSheetConfig const& sheet,
    AtlasLoadOutput& output );

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, AsciiFontSheetConfig const& sheet );

} // namespace rr
