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

// Rds
#include "sprite-sheet.rds.hpp"

// gfx
#include "gfx/image.hpp"

// base
#include "base/valid.hpp"

// C++ standard library
#include <array>
#include <string>
#include <unordered_map>

namespace rr {

struct AtlasBuilder;

/****************************************************************
** AsciiFont
*****************************************************************/
struct AsciiFont {
  AsciiFont( std::unique_ptr<std::array<int, 256>> ids,
             gfx::size                             char_size )
    : atlas_ids_( std::move( ids ) ), char_size_{ char_size } {}

  gfx::size char_size() const { return char_size_; }

  int atlas_id_for_char( uint8_t c );

 private:
  std::unique_ptr<std::array<int, 256>> atlas_ids_;
  gfx::size                             char_size_;
};

/****************************************************************
** Loading from images.
*****************************************************************/
base::valid_or<std::string> load_sprite_sheet(
    AtlasBuilder& builder, gfx::image sheet,
    gfx::size sprite_size,
    std::unordered_map<std::string, gfx::point> const& names,
    std::unordered_map<std::string, int>& atlas_ids );

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, gfx::image sheet );

/****************************************************************
** Loading from config info.
*****************************************************************/
base::valid_or<std::string> load_sprite_sheet(
    AtlasBuilder& builder, SpriteSheetConfig const& sheet,
    std::unordered_map<std::string, int>& atlas_ids );

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, AsciiFontSheetConfig const& sheet );

} // namespace rr
