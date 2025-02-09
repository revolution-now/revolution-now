/****************************************************************
**tiles.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Handles loading and retrieving tiles
*
*****************************************************************/
#include "tiles.hpp"

// Revolution Now
#include "error.hpp"

// config
#include "config/tile-enum.rds.hpp"
#include "config/tile-sheet.rds.hpp"

// render
#include "render/atlas.hpp"
#include "render/renderer.hpp" // FIXME: remove

// gfx
#include "gfx/iter.hpp"

// refl
#include "refl/enum-map.hpp"
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

// C++ standard library
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::literals::string_literals;

namespace rn {

namespace {

using ::base::valid;
using ::base::valid_or;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_count;

vector<int> cache;
vector<rect> trimmed_cache;

int atlas_lookup( e_tile const tile ) {
  int const idx = static_cast<int>( tile );
  CHECK( idx < int( cache.size() ) );
  return cache[idx];
}

rect atlas_lookup_trimmed( e_tile const tile ) {
  int const idx = static_cast<int>( tile );
  CHECK( idx < int( trimmed_cache.size() ) );
  auto const& res = trimmed_cache[idx];
  // This is only expected to ever fail in unit tests when we
  // forget to initialize a tile in the trimmed cache.
  CHECK( res.origin.x >= 0,
         "trimmed cache not initialized properly or has invalid "
         "value for tile {}.  If you are in a unit test then "
         "you must call testing_set_trimmed_cache to set a "
         "trimmed rect for the tile.",
         tile );
  return res;
}

} // namespace

void testing_set_trimmed_cache( e_tile const tile,
                                rect const trimmed ) {
  if( trimmed_cache.empty() ) {
    trimmed_cache.resize( enum_count<e_tile> );
    // Use this to mark the values as uninitialized (a negative
    // value is guaranteed to never come from the trimming algo)
    // so that we can easily cache uninitialized entries in unit
    // tests.
    for( auto& e : trimmed_cache ) e.origin.x = -1;
  }
  CHECK( !trimmed_cache.empty() );
  int const idx = static_cast<int>( tile );
  CHECK_LT( idx, ssize( trimmed_cache ) );
  trimmed_cache[idx] = trimmed;
}

void init_sprites( rr::Renderer& renderer ) {
  cache.resize( refl::enum_count<e_tile> );
  trimmed_cache.resize( refl::enum_count<e_tile> );
  auto const& atlas_ids = renderer.atlas_ids();
  auto const& atlas_trimmed_rects =
      renderer.atlas_trimmed_rects();
  int i = 0;
  for( e_tile const tile : refl::enum_values<e_tile> ) {
    string const name( refl::enum_value_name( tile ) );
    UNWRAP_CHECK( atlas_id, base::lookup( atlas_ids, name ) );
    UNWRAP_CHECK( atlas_trimmed,
                  base::lookup( atlas_trimmed_rects, name ) );
    cache[i]         = atlas_id;
    trimmed_cache[i] = atlas_trimmed;
    ++i;
  }
}

valid_or<string> validate_sprites( rr::Renderer& ) {
  // Check that food and fish tiles have the same trimmed rect.
  // This is necessary because they sometimes need to be rendered
  // within the same tile spread (i.e. in the colony population
  // view).
  {
    rect const food_20 =
        atlas_lookup_trimmed( e_tile::commodity_food_20 );
    rect const fish_20 =
        atlas_lookup_trimmed( e_tile::product_fish_20 );
    rect const food_16 =
        atlas_lookup_trimmed( e_tile::commodity_food_16 );
    rect const fish_16 =
        atlas_lookup_trimmed( e_tile::product_fish_16 );
    if( food_20 != fish_20 )
      return fmt::format(
          "The 20x20 food and fish tiles must have the same "
          "bounding box within their sprites, but instead the "
          "food tile has {} and the fish tile has {}.",
          food_20, fish_20 );
    if( food_16 != fish_16 )
      return fmt::format(
          "The 16x16 food and fish tiles must have the same "
          "bounding box within their sprites, but instead the "
          "food tile has {} and the fish tile has {}.",
          food_16, fish_16 );
  }
  return valid;
}

void deinit_sprites() {
  cache.clear();
  trimmed_cache.clear();
}

gfx::size sprite_size( e_tile tile ) {
  // FIXME: find a better way to do this. Maybe store it in the
  // renderer object.
  static auto const sizes = [] {
    refl::enum_map<e_tile, gfx::size> res;
    for( rr::SpriteSheetConfig const& sheet :
         config_tile_sheet.sheets.sprite_sheets ) {
      for( auto const& [sprite_name, pos] : sheet.sprites ) {
        // This should have already been validated.
        UNWRAP_CHECK( tile, refl::enum_from_string<e_tile>(
                                sprite_name ) );
        res[tile] = sheet.sprite_size;
      }
    }
    return res;
  }();
  return sizes[tile];
}

rect trimmed_area_for( e_tile const tile ) {
  return atlas_lookup_trimmed( tile );
}

void render_sprite( rr::Painter& painter, Rect where,
                    e_tile tile ) {
  painter.draw_sprite_scale( atlas_lookup( tile ), where );
}

void render_sprite( rr::Renderer& renderer, gfx::point where,
                    e_tile tile ) {
  renderer.painter().draw_sprite( atlas_lookup( tile ), where );
}

void render_sprite( rr::Renderer& renderer, Coord where,
                    e_tile tile ) {
  render_sprite( renderer, where.to_gfx(), tile );
}

void render_sprite_section( rr::Painter& painter, e_tile tile,
                            Coord where, Rect source ) {
  painter.draw_sprite_section( atlas_lookup( tile ), where,
                               source );
}

void render_sprite_outline( rr::Renderer& renderer,
                            gfx::point const where,
                            e_tile const tile,
                            gfx::pixel const outline_color ) {
  render_sprite_silhouette( renderer, where + size{ .w = 1 },
                            tile, outline_color );
  render_sprite_silhouette( renderer, where + size{ .w = -1 },
                            tile, outline_color );
  render_sprite_silhouette( renderer, where + size{ .h = 1 },
                            tile, outline_color );
  render_sprite_silhouette( renderer, where + size{ .h = -1 },
                            tile, outline_color );
  render_sprite( renderer, where, tile );
}

void render_sprite_silhouette( rr::Renderer& renderer,
                               Coord where, e_tile tile,
                               gfx::pixel color ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.fixed_color, color );
  rr::Painter painter = renderer.painter();
  painter.draw_sprite( atlas_lookup( tile ), where );
}

void render_sprite_dulled( rr::Renderer& renderer, e_tile tile,
                           Coord where, bool dulled ) {
  if( !dulled ) {
    render_sprite( renderer, where, tile );
    return;
  }
  // First draw the sprite desaturated, then draw it again but
  // with a transparent black silhouette.
  {
    SCOPED_RENDERER_MOD_OR( painter_mods.desaturate, true );
    render_sprite( renderer, where, tile );
  }
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .3 );
    render_sprite_silhouette( renderer, where, tile,
                              gfx::pixel::black() );
  }
}

rr::StencilPlan stencil_plan_for( e_tile replacement_tile,
                                  gfx::pixel key_color ) {
  return {
    .replacement_atlas_id = atlas_lookup( replacement_tile ),
    .key_color            = key_color };
}

void render_sprite_stencil( rr::Renderer& renderer, Coord where,
                            e_tile tile, e_tile replacement_tile,
                            gfx::pixel key_color ) {
  SCOPED_RENDERER_MOD_SET(
      painter_mods.stencil,
      stencil_plan_for( replacement_tile, key_color ) );
  renderer.painter().draw_sprite( atlas_lookup( tile ), where );
}

void tile_sprite( rr::Renderer& renderer, e_tile tile,
                  Rect const& rect ) {
  Delta info = sprite_size( tile );
  auto mod   = rect.delta() % info;
  if( mod.w == 0 && rect.delta().w != 0 ) mod.w = 1 * info.w;
  if( mod.h == 0 && rect.delta().h != 0 ) mod.h = 1 * info.h;
  auto smaller_rect =
      Rect::from( rect.upper_left(),
                  rect.delta() - Delta{ .w = 1, .h = 1 } );
  for( Rect const r : gfx::subrects(
           Rect::from( Coord{}, smaller_rect.delta() / info ) ) )
    render_sprite(
        renderer,
        rect.upper_left() +
            r.upper_left().distance_from_origin() * info,
        tile );
  rr::Painter painter = renderer.painter();
  for( H h = 0; h < smaller_rect.h / info.h; ++h ) {
    auto where = rect.upper_right() - Delta{ .w = mod.w } +
                 Delta{ .h = h } * info.h;
    render_sprite_section(
        painter, tile, where,
        Rect::from( Coord{}, mod.with_height( 1 * info.h ) ) );
  }
  for( W w = 0; w < smaller_rect.w / info.w; ++w ) {
    auto where = rect.lower_left() - Delta{ .h = mod.h } +
                 Delta{ .w = w } * info.w;
    render_sprite_section(
        painter, tile, where,
        Rect::from( Coord{}, mod.with_width( 1 * info.w ) ) );
  }
  auto where = rect.lower_right() - mod;
  render_sprite_section( painter, tile, where,
                         Rect::from( Coord{}, mod ) );
}

void render_rect_of_sprites_with_border(
    rr::Renderer& renderer, // where to draw it
    Coord dest_origin,      // pixel coord of upper left
    size size_tiles,        // tile coords, including border
    e_tile middle,          //
    e_tile top,             //
    e_tile bottom,          //
    e_tile left,            //
    e_tile right,           //
    e_tile top_left,        //
    e_tile top_right,       //
    e_tile bottom_left,     //
    e_tile bottom_right     //
) {
  Delta sprite_middle = sprite_size( middle );
  CHECK( sprite_middle.w == sprite_middle.h );
  for( auto tile : { middle, top, bottom, left, right, top_left,
                     top_right, bottom_left, bottom_right } ) {
    CHECK( sprite_size( tile ) == sprite_middle );
  }

  auto scale = sprite_middle;

  auto to_pixels = [&]( Coord coord ) {
    coord = scale * coord;
    // coord is now in pixels.
    auto delta = coord - Coord{};
    return dest_origin + delta;
  };

  Rect dst_tile_rect = Rect::from( Coord{}, size_tiles );
  for( Rect const r :
       gfx::subrects( dst_tile_rect.edges_removed() ) )
    render_sprite( renderer, to_pixels( r.upper_left() ),
                   middle );

  for( X x = dst_tile_rect.x + 1;
       x < dst_tile_rect.right_edge() - 1; ++x )
    render_sprite( renderer, to_pixels( { .x = x, .y = 0 } ),
                   top );
  for( X x = dst_tile_rect.x + 1;
       x < dst_tile_rect.right_edge() - 1; ++x )
    render_sprite(
        renderer,
        to_pixels(
            { .x = x,
              .y = 0 + ( dst_tile_rect.bottom_edge() - 1 ) } ),
        bottom );
  for( Y y = dst_tile_rect.y + 1;
       y < dst_tile_rect.bottom_edge() - 1; ++y )
    render_sprite( renderer, to_pixels( { .x = 0, .y = y } ),
                   left );
  for( Y y = dst_tile_rect.y + 1;
       y < dst_tile_rect.bottom_edge() - 1; ++y )
    render_sprite(
        renderer,
        to_pixels( { .x = 0 + ( dst_tile_rect.right_edge() - 1 ),
                     .y = y } ),
        right );

  render_sprite( renderer, to_pixels( { .x = 0, .y = 0 } ),
                 top_left );
  render_sprite(
      renderer,
      to_pixels( { .x = 0 + ( dst_tile_rect.right_edge() - 1 ),
                   .y = 0 } ),
      top_right );
  render_sprite(
      renderer,
      to_pixels(
          { .x = 0,
            .y = 0 + ( dst_tile_rect.bottom_edge() - 1 ) } ),
      bottom_left );
  render_sprite(
      renderer,
      to_pixels(
          { .x = 0 + ( dst_tile_rect.right_edge() - 1 ),
            .y = 0 + ( dst_tile_rect.bottom_edge() - 1 ) } ),
      bottom_right );
}

} // namespace rn
