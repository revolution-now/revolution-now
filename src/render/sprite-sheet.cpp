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

// gfx
#include "gfx/image-analysis.hpp"
#include "gfx/image.hpp"

// refl
#include "refl/to-str.hpp"

// stb
#include "stb/image.hpp"

// C++ standard library
#include <ranges>

namespace rg = std::ranges;

using namespace std;

namespace rr {

using ::base::maybe;
using ::base::valid;
using ::base::valid_or;
using ::gfx::image;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

/****************************************************************
** Loading from images.
*****************************************************************/
namespace detail {

valid_or<string> load_sprite_sheet(
    AtlasBuilder& builder, image&& sheet, size sprite_size,
    unordered_map<string, point> const& names,
    SpriteSheetOptions const& sheet_options,
    AtlasLoadOutput& output ) {
  rect const img_pixels    = sheet.rect_pixels();
  auto const ordered_names = [&] {
    vector<pair<string, point>> res( names.begin(),
                                     names.end() );
    rg::sort( res, []( auto const& l, auto const& r ) {
      return l.first < r.first;
    } );
    return res;
  }();
  maybe<image> burrowed;
  // NOTE: Do this before moving out of `sheet`.
  if( sheet_options.compute_burrow ) {
    vector<rect> sprite_rects;
    sprite_rects.reserve( names.size() );
    for( auto const& [name, p] : names ) {
      rect const r{ .origin = p * sprite_size,
                    .size   = sprite_size };
      sprite_rects.push_back( r );
    }
    burrowed = compute_burrowed_sprites( sheet, sprite_rects );
  }
  AtlasBuilder::ImageBuilder img_builder =
      builder.add_image( std::move( sheet ) );
  vector<pair<int /*id*/, rect>> rects;
  rects.reserve( ordered_names.size() );
  for( auto const& [name, p] : ordered_names ) {
    if( output.atlas_ids.contains( name ) )
      return fmt::format(
          "atlas ID map already contains a sprite named `{}'.",
          name );
    rect const r{ .origin = p * sprite_size,
                  .size   = sprite_size };
    if( !r.is_inside( img_pixels ) )
      return fmt::format(
          "sprite `{}' has rect {} which is outside of the img "
          "of size {}.",
          name, r, img_pixels );
    int const sprite_id    = img_builder.add_sprite( r );
    output.atlas_ids[name] = sprite_id;
    rects.push_back( { sprite_id, r } );
  }
  if( burrowed.has_value() ) {
    AtlasBuilder::ImageBuilder burrow_img_builder =
        builder.add_image( std::move( *burrowed ) );
    for( auto const& [sprite_id, r] : rects ) {
      int const burrowed_sprite_id =
          burrow_img_builder.add_sprite( r );
      output.atlas_burrow_ids[sprite_id] = burrowed_sprite_id;
    }
  }
  return valid;
}

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, image&& sheet ) {
  AtlasBuilder::ImageBuilder img_builder =
      builder.add_image( std::move( sheet ) );
  size const sz = sheet.size_pixels();
  if( sz.w % 16 != 0 || sz.h % 16 != 0 )
    return fmt::format(
        "ascii font sheet must have dimensions that are a "
        "multiple of 16 (instead found {}).",
        sz );
  if( sz.w <= 16 || sz.h <= 16 )
    return fmt::format(
        "ascii font sheet must have at least one pixel per "
        "character (image size is {}).",
        sz );
  size const char_size = size{ .w = sz.w / 16, .h = sz.h / 16 };
  rect r{ .origin = {}, .size = char_size };
  auto arr    = make_unique<array<int, 256>>();
  int arr_idx = 0;
  for( int y = 0; y < 16; ++y ) {
    for( int x = 0; x < 16; ++x ) {
      r.origin            = point{ .x = x, .y = y } * char_size;
      int const id        = img_builder.add_sprite( r );
      ( *arr )[arr_idx++] = id;
    }
  }
  return AsciiFont( std::move( arr ), char_size );
}

} // namespace detail

/****************************************************************
** Loading from config info.
*****************************************************************/
valid_or<string> load_sprite_sheet(
    AtlasBuilder& builder, SpriteSheetConfig const& sheet,
    AtlasLoadOutput& output ) {
  UNWRAP_RETURN( img, stb::load_image( sheet.img_path ) );
  return detail::load_sprite_sheet(
      builder, std::move( img ), sheet.sprite_size,
      sheet.sprites, sheet.options, output );
}

base::expect<AsciiFont> load_ascii_font_sheet(
    AtlasBuilder& builder, AsciiFontSheetConfig const& sheet ) {
  UNWRAP_RETURN( img, stb::load_image( sheet.img_path ) );
  return detail::load_ascii_font_sheet( builder,
                                        std::move( img ) );
}

} // namespace rr
