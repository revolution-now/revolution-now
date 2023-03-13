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
#include "config-files.hpp"
#include "error.hpp"
#include "init.hpp"
#include "renderer.hpp"

// config
#include "config/tile-enum.rds.hpp"
#include "config/tile-sheet.rds.hpp"

// render
#include "render/atlas.hpp"

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

vector<int> cache;

void init_sprites() {
  // FIXME: need to find a better way to get the renderer to gen-
  // erate the atlas ID cache.
  rr::Renderer& renderer =
      global_renderer_use_only_when_needed();
  cache.resize( refl::enum_count<e_tile> );
  auto& atlas_ids = renderer.atlas_ids();
  int   i         = 0;
  for( e_tile tile : refl::enum_values<e_tile> ) {
    UNWRAP_CHECK( atlas_id,
                  base::lookup( atlas_ids, refl::enum_value_name(
                                               tile ) ) );
    cache[i++] = atlas_id;
  }
}

void cleanup_sprites() { cache.clear(); }

int atlas_lookup( e_tile tile ) {
  int idx = static_cast<int>( tile );
  DCHECK( idx < int( cache.size() ) );
  return cache[idx];
}

REGISTER_INIT_ROUTINE( sprites );

} // namespace

Delta sprite_size( e_tile tile ) {
  // FIXME: find a better way to do this. Maybe store it in the
  // renderer object.
  static auto const sizes = [] {
    CHECK( configs_loaded() );
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
  return Delta::from_gfx( sizes[tile] );
}

void render_sprite( rr::Painter& painter, Rect where,
                    e_tile tile ) {
  painter.draw_sprite_scale( atlas_lookup( tile ), where );
}

void render_sprite( rr::Painter& painter, e_tile tile,
                    Coord where ) {
  painter.draw_sprite( atlas_lookup( tile ), where );
}

void render_sprite( rr::Painter& painter, Coord where,
                    e_tile tile ) {
  painter.draw_sprite( atlas_lookup( tile ), where );
}

void render_sprite_section( rr::Painter& painter, e_tile tile,
                            Coord where, Rect source ) {
  painter.draw_sprite_section( atlas_lookup( tile ), where,
                               source );
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
    rr::Painter painter = renderer.painter();
    render_sprite( painter, tile, where );
    return;
  }
  // First draw the sprite desaturated, then draw it again but
  // with a transparent black silhouette.
  {
    SCOPED_RENDERER_MOD_OR( painter_mods.desaturate, true );
    rr::Painter painter = renderer.painter();
    render_sprite( painter, tile, where );
  }
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .3 );
    render_sprite_silhouette( renderer, where, tile,
                              gfx::pixel::black() );
  }
}

void render_sprite_stencil( rr::Painter& painter, Coord where,
                            e_tile tile, e_tile replacement_tile,
                            gfx::pixel key_color ) {
  painter.draw_stencil( atlas_lookup( tile ),
                        atlas_lookup( replacement_tile ), where,
                        key_color );
}

void tile_sprite( rr::Painter& painter, e_tile tile,
                  Rect const& rect ) {
  Delta info = sprite_size( tile );
  auto  mod  = rect.delta() % info;
  if( mod.w == 0 && rect.delta().w != 0 ) mod.w = 1 * info.w;
  if( mod.h == 0 && rect.delta().h != 0 ) mod.h = 1 * info.h;
  auto smaller_rect =
      Rect::from( rect.upper_left(),
                  rect.delta() - Delta{ .w = 1, .h = 1 } );
  for( Rect const r : gfx::subrects(
           Rect::from( Coord{}, smaller_rect.delta() / info ) ) )
    render_sprite(
        painter, tile,
        rect.upper_left() +
            r.upper_left().distance_from_origin() * info );
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
    rr::Painter& painter,     // where to draw it
    Coord        dest_origin, // pixel coord of upper left
    Delta        size_tiles,  // tile coords, including border
    e_tile       middle,      //
    e_tile       top,         //
    e_tile       bottom,      //
    e_tile       left,        //
    e_tile       right,       //
    e_tile       top_left,    //
    e_tile       top_right,   //
    e_tile       bottom_left, //
    e_tile       bottom_right //
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
    render_sprite( painter, middle,
                   to_pixels( r.upper_left() ) );

  for( X x = dst_tile_rect.x + 1;
       x < dst_tile_rect.right_edge() - 1; ++x )
    render_sprite( painter, top,
                   to_pixels( { .x = x, .y = 0 } ) );
  for( X x = dst_tile_rect.x + 1;
       x < dst_tile_rect.right_edge() - 1; ++x )
    render_sprite(
        painter, bottom,
        to_pixels(
            { .x = x,
              .y = 0 + ( dst_tile_rect.bottom_edge() - 1 ) } ) );
  for( Y y = dst_tile_rect.y + 1;
       y < dst_tile_rect.bottom_edge() - 1; ++y )
    render_sprite( painter, left,
                   to_pixels( { .x = 0, .y = y } ) );
  for( Y y = dst_tile_rect.y + 1;
       y < dst_tile_rect.bottom_edge() - 1; ++y )
    render_sprite(
        painter, right,
        to_pixels( { .x = 0 + ( dst_tile_rect.right_edge() - 1 ),
                     .y = y } ) );

  render_sprite( painter, top_left,
                 to_pixels( { .x = 0, .y = 0 } ) );
  render_sprite(
      painter, top_right,
      to_pixels( { .x = 0 + ( dst_tile_rect.right_edge() - 1 ),
                   .y = 0 } ) );
  render_sprite(
      painter, bottom_left,
      to_pixels(
          { .x = 0,
            .y = 0 + ( dst_tile_rect.bottom_edge() - 1 ) } ) );
  render_sprite(
      painter, bottom_right,
      to_pixels(
          { .x = 0 + ( dst_tile_rect.right_edge() - 1 ),
            .y = 0 + ( dst_tile_rect.bottom_edge() - 1 ) } ) );
}

} // namespace rn
