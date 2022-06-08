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
#include "init.hpp"
#include "renderer.hpp"

// config
#include "config/tile-sheet.rds.hpp"

// render
#include "render/atlas.hpp"

// refl
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

} // namespace

REGISTER_INIT_ROUTINE( sprites );

Delta sprite_size( e_tile tile ) {
  return Delta::from_gfx(
      config_tile_sheet.sheets.sprite_size( tile ) );
}

gfx::size depixelation_offset( rr::Painter& painter,
                               e_tile       from_tile,
                               e_tile       to_tile ) {
  gfx::size from_tile_size = sprite_size( from_tile );
  gfx::size to_tile_size   = sprite_size( to_tile );
  CHECK( sprite_size( from_tile ) == sprite_size( to_tile ),
         "depixelation offsets can only be computed between "
         "sprites of the same size; sprite {} has size {} and "
         "sprite {} has size {}.",
         from_tile, from_tile_size, to_tile, to_tile_size );
  int                 from_id = atlas_lookup( from_tile );
  int                 to_id   = atlas_lookup( to_tile );
  rr::AtlasMap const& atlas   = painter.atlas();
  return atlas.lookup( to_id ).origin -
         atlas.lookup( from_id ).origin;
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

void render_sprite_silhouette( rr::Painter& painter, Coord where,
                               e_tile tile, gfx::pixel color ) {
  painter.draw_silhouette( atlas_lookup( tile ), where, color );
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
  Delta info       = sprite_size( tile );
  Scale info_scale = info.to_scale();
  auto  mod        = rect.delta() % info_scale;
  if( mod.w == 0_w && rect.delta().w != 0_w )
    mod.w = 1_w * info_scale.sx;
  if( mod.h == 0_h && rect.delta().h != 0_h )
    mod.h = 1_h * info_scale.sy;
  auto smaller_rect = Rect::from(
      rect.upper_left(), rect.delta() - Delta{ 1_w, 1_h } );
  for( auto coord :
       Rect::from( Coord{}, smaller_rect.delta() / info_scale ) )
    render_sprite(
        painter, tile,
        rect.upper_left() +
            coord.distance_from_origin() * info_scale );
  for( H h = 0_h; h < smaller_rect.h / info_scale.sy; ++h ) {
    auto where = rect.upper_right() - mod.w + h * info_scale.sy;
    render_sprite_section(
        painter, tile, where,
        Rect::from( Coord{},
                    mod.with_height( 1_h * info_scale.sy ) ) );
  }
  for( W w = 0_w; w < smaller_rect.w / info_scale.sx; ++w ) {
    auto where = rect.lower_left() - mod.h + w * info_scale.sx;
    render_sprite_section(
        painter, tile, where,
        Rect::from( Coord{},
                    mod.with_width( 1_w * info_scale.sx ) ) );
  }
  auto where = rect.lower_right() - mod;
  render_sprite_section( painter, tile, where,
                         Rect::from( Coord{}, mod ) );
}

e_tile index_to_tile( int index ) {
  return static_cast<e_tile>( index );
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
  CHECK( sprite_middle.w._ == sprite_middle.h._ );
  for( auto tile : { middle, top, bottom, left, right, top_left,
                     top_right, bottom_left, bottom_right } ) {
    CHECK( sprite_size( tile ) == sprite_middle );
  }

  auto scale = sprite_middle.to_scale();

  auto to_pixels = [&]( Coord coord ) {
    coord = scale * coord;
    // coord is now in pixels.
    auto delta = coord - Coord{};
    return dest_origin + delta;
  };

  Rect dst_tile_rect = Rect::from( Coord{}, size_tiles );
  for( auto coord : dst_tile_rect.edges_removed() )
    render_sprite( painter, middle, to_pixels( coord ) );

  for( X x = dst_tile_rect.x + 1_w;
       x < dst_tile_rect.right_edge() - 1_w; ++x )
    render_sprite( painter, top, to_pixels( { 0_y, x } ) );
  for( X x = dst_tile_rect.x + 1_w;
       x < dst_tile_rect.right_edge() - 1_w; ++x )
    render_sprite(
        painter, bottom,
        to_pixels( { 0_y + ( dst_tile_rect.bottom_edge() - 1_h ),
                     x } ) );
  for( Y y = dst_tile_rect.y + 1_h;
       y < dst_tile_rect.bottom_edge() - 1_h; ++y )
    render_sprite( painter, left, to_pixels( { y, 0_x } ) );
  for( Y y = dst_tile_rect.y + 1_h;
       y < dst_tile_rect.bottom_edge() - 1_h; ++y )
    render_sprite(
        painter, right,
        to_pixels( { y, 0_x + ( dst_tile_rect.right_edge() -
                                1_w ) } ) );

  render_sprite( painter, top_left, to_pixels( { 0_y, 0_x } ) );
  render_sprite(
      painter, top_right,
      to_pixels( { 0_y, 0_x + ( dst_tile_rect.right_edge() -
                                1_w ) } ) );
  render_sprite(
      painter, bottom_left,
      to_pixels( { 0_y + ( dst_tile_rect.bottom_edge() - 1_h ),
                   0_x } ) );
  render_sprite(
      painter, bottom_right,
      to_pixels(
          { 0_y + ( dst_tile_rect.bottom_edge() - 1_h ),
            0_x + ( dst_tile_rect.right_edge() - 1_w ) } ) );
}

} // namespace rn
