/****************************************************************
**sprite-sheet.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-03-10.
*
* Description: Describes and loads a sprite sheet into the atlas.
*
*****************************************************************/
#include "sprite-sheet.hpp"

// render
#include "atlas.hpp"

// refl
#include "refl/to-str.hpp"

// stb
#include "stb/image.hpp"

using namespace std;

namespace rr {

/****************************************************************
** AsciiFont
*****************************************************************/
int AsciiFont::atlas_id_for_char( uint8_t c ) {
  return ( *atlas_ids_ )[c];
}

/****************************************************************
** Loading from images.
*****************************************************************/
base::valid_or<string> load_sprite_sheet(
    AtlasBuilder& builder, gfx::image sheet,
    gfx::size                                sprite_size,
    unordered_map<string, gfx::point> const& names,
    unordered_map<string, int>&              atlas_ids ) {
  gfx::rect const            img_pixels = sheet.rect_pixels();
  AtlasBuilder::ImageBuilder img_builder =
      builder.add_image( std::move( sheet ) );
  gfx::rect r{ .origin = {}, .size = sprite_size };
  vector<pair<string, gfx::point>> ordered_names( names.begin(),
                                                  names.end() );
  sort( ordered_names.begin(), ordered_names.end(),
        []( auto const& l, auto const& r ) {
          return l.first < r.first;
        } );
  for( auto const& [name, p] : ordered_names ) {
    if( atlas_ids.contains( name ) )
      return fmt::format(
          "atlas ID map already contains a sprite named `{}'.",
          name );
    r.origin = p * sprite_size;
    if( !r.is_inside( img_pixels ) )
      return fmt::format(
          "sprite `{}' has rect {} which is outside of the img "
          "of size {}.",
          name, r, img_pixels );
    int id          = img_builder.add_sprite( r );
    atlas_ids[name] = id;
  }
  return base::valid;
}

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, gfx::image sheet ) {
  AtlasBuilder::ImageBuilder img_builder =
      builder.add_image( std::move( sheet ) );
  gfx::size size = sheet.size_pixels();
  if( size.w % 16 != 0 || size.h % 16 != 0 )
    return fmt::format(
        "ascii font sheet must have dimensions that are a "
        "multiple of 16 (instead found {}).",
        size );
  if( size.w <= 16 || size.h <= 16 )
    return fmt::format(
        "ascii font sheet must have at least one pixel per "
        "character (image size is {}).",
        size );
  gfx::size const char_size =
      gfx::size{ .w = size.w / 16, .h = size.h / 16 };
  gfx::rect r{ .origin = {}, .size = char_size };
  auto      arr     = make_unique<array<int, 256>>();
  int       arr_idx = 0;
  for( int y = 0; y < 16; ++y ) {
    for( int x = 0; x < 16; ++x ) {
      r.origin            = gfx::point{ .x = x, .y = y };
      int id              = img_builder.add_sprite( r );
      ( *arr )[arr_idx++] = id;
    }
  }
  return AsciiFont( std::move( arr ), char_size );
}

/****************************************************************
** Loading from config info.
*****************************************************************/
base::valid_or<string> load_sprite_sheet(
    AtlasBuilder& builder, SpriteSheetConfig const& sheet,
    unordered_map<string, int>& atlas_ids ) {
  UNWRAP_RETURN( img, stb::load_image( sheet.img_path ) );
  return load_sprite_sheet( builder, std::move( img ),
                            sheet.sprite_size, sheet.sprites,
                            atlas_ids );
}

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, AsciiFontSheetConfig const& sheet ) {
  UNWRAP_RETURN( img, stb::load_image( sheet.img_path ) );
  return load_ascii_font_sheet( builder, std::move( img ) );
}

} // namespace rr
