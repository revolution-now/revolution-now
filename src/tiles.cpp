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

// Revolution Now (config)
#include "../config/rcl/tile-sheet.inl"

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

vector<maybe<int>> cache;

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

} // namespace

REGISTER_INIT_ROUTINE( sprites );

Delta sprite_size( e_tile tile ) {
  return Delta::from_gfx(
      config_tile.sheets.sprite_size( tile ) );
}

gfx::size depixelation_offset( e_tile from_tile,
                               e_tile to_tile ) {
  // TODO
}

void render_sprite( rr::Painter& painter, Rect where,
                    e_tile tile ) {
  // TODO
}

void render_sprite_section( rr::Painter& painter, e_tile tile,
                            Coord pixel_coord, Rect source ) {
  // TODO
}

void render_sprite( rr::Painter& painter, e_tile tile,
                    Coord coord ) {
  Y    pixel_row = coord.y;
  X    pixel_col = coord.x;
  auto where     = g_sprites.find( tile );
  CHECK( where != g_sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  Sprite const& sp = where->second;

  Rect dst;
  dst.x = pixel_col;
  dst.y = pixel_row;
  dst.w = W{ 1 } * sp.scale.sx;
  dst.h = H{ 1 } * sp.scale.sy;

  constexpr double right_angle = 90.0; // degrees

  double angle = rot * right_angle;

  auto flip =
      ( flip_x != 0 ) ? e_flip::horizontal : e_flip::none;

  sp.texture->copy_to( tx, sp.source, dst, angle, flip );
}

void render_sprite_section( rr::Painter& painter, e_tile tile,
                            Coord pixel_coord, Rect clip ) {
  auto where = g_sprites.find( tile );
  CHECK( where != g_sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  Sprite const& sp = where->second;

  Rect dst;
  dst.x = pixel_coord.x;
  dst.y = pixel_coord.y;

  auto new_src = sp.source.clamp( clip.shifted_by(
      sp.source.upper_left().distance_from_origin() ) );

  dst.w = new_src.w;
  dst.h = new_src.h;

  sp.texture->copy_to( tx, new_src, dst, 0.0, e_flip::none );
}

void tile_sprite( rr::Painter& painter, e_tile tile,
                  Rect const& rect ) {
  auto& info = lookup_sprite( tile );
  auto  mod  = rect.delta() % info.scale;
  if( mod.w == 0_w && rect.delta().w != 0_w )
    mod.w = 1_w * info.scale.sx;
  if( mod.h == 0_h && rect.delta().h != 0_h )
    mod.h = 1_h * info.scale.sy;
  auto smaller_rect = Rect::from(
      rect.upper_left(), rect.delta() - Delta{ 1_w, 1_h } );
  for( auto coord :
       Rect::from( Coord{}, smaller_rect.delta() / info.scale ) )
    render_sprite(
        painter, tile,
        rect.upper_left() +
            coord.distance_from_origin() * info.scale );
  for( H h = 0_h; h < smaller_rect.h / info.scale.sy; ++h ) {
    auto pixel_coord =
        rect.upper_right() - mod.w + h * info.scale.sy;
    render_sprite_section(
        painter, tile, pixel_coord,
        Rect::from( Coord{},
                    mod.with_height( 1_h * info.scale.sy ) ) );
  }
  for( W w = 0_w; w < smaller_rect.w / info.scale.sx; ++w ) {
    auto pixel_coord =
        rect.lower_left() - mod.h + w * info.scale.sx;
    render_sprite_section(
        painter, tile, pixel_coord,
        Rect::from( Coord{},
                    mod.with_width( 1_w * info.scale.sx ) ) );
  }
  auto pixel_coord = rect.lower_right() - mod;
  render_sprite_section( painter, tile, pixel_coord,
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
  auto const& sprite_middle = lookup_sprite( middle );
  CHECK( sprite_middle.scale.sx._ == sprite_middle.scale.sy._ );
  for( auto tile : { middle, top, bottom, left, right, top_left,
                     top_right, bottom_left, bottom_right } ) {
    CHECK( lookup_sprite( tile ).scale == sprite_middle.scale );
  }

  auto scale = sprite_middle.scale;

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
