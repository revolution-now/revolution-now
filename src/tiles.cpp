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
#include "errors.hpp"
#include "hash.hpp"
#include "init.hpp"
#include "sdl-util.hpp"

// Revolution Now (config)
#include "config/ucl/art.inl"

// C++ standard library
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::literals::string_literals;

namespace rn {

namespace {

struct tile_map {
  struct one_tile {
    int index;
    X   x;
    Y   y;
    int tile, rot, flip_x;
  };
  vector<one_tile> tiles;
};

unordered_map<g_tile, sprite, EnumClassHash> sprites;
unordered_map<std::string, tile_map>         tile_maps;

sprite create_sprite_32( Texture const& texture, Coord coord ) {
  Rect rect{coord.x * 32_sx, coord.y * 32_sy, 32_w, 32_h};
  return {&texture, rect, {32_sx, 32_sy}};
}

sprite create_sprite_8( Texture const& texture, Coord coord ) {
  Rect rect{coord.x * 8_sx, coord.y * 8_sy, 8_w, 8_h};
  return {&texture, rect, {8_sx, 8_sy}};
}

sprite create_sprite_16( Texture const& texture, Coord coord ) {
  Rect rect{coord.x * 16_sx, coord.y * 16_sy, 16_w, 16_h};
  return {&texture, rect, {16_sx, 16_sy}};
}

sprite create_sprite_128_64( Texture const& texture,
                             Coord          coord ) {
  Rect rect{coord.x * 128_sx, coord.y * 64_sy, 128_w, 64_h};
  return {&texture, rect, {128_sx, 64_sy}};
}

sprite create_sprite_128_16( Texture const& texture,
                             Coord          coord ) {
  Rect rect{coord.x * 128_sx, coord.y * 16_sy, 128_w, 16_h};
  return {&texture, rect, {128_sx, 16_sy}};
}

#define SET_SPRITE_WORLD( name )            \
  sprites[g_tile::name] = create_sprite_32( \
      tile_set_world, config_art.tiles.world.coords.name )

#define SET_SPRITE_UNIT( name )             \
  sprites[g_tile::name] = create_sprite_32( \
      tile_set_units, config_art.tiles.units.coords.name )

#define SET_SPRITE_MENU( name )            \
  sprites[g_tile::name] = create_sprite_8( \
      tile_set_menu, config_art.tiles.menu.coords.name )

#define SET_SPRITE_MENU16( name )           \
  sprites[g_tile::name] = create_sprite_16( \
      tile_set_menu16, config_art.tiles.menu16.coords.name )

#define SET_SPRITE_WOOD( name )                 \
  sprites[g_tile::name] = create_sprite_128_64( \
      tile_set_wood_128_64, config_art.tiles.wood.coords.name )

#define SET_SPRITE_MENU_SEL( name )             \
  sprites[g_tile::name] = create_sprite_128_16( \
      tile_set_menu_sel,                        \
      config_art.tiles.menu_sel.coords.name )

#define SET_SPRITE_BUTTON( name )          \
  sprites[g_tile::name] = create_sprite_8( \
      tile_set_button, config_art.tiles.button.coords.name )

void init_sprites() {
  auto& tile_set_world =
      load_texture( config_art.tiles.world.img );
  auto& tile_set_units =
      load_texture( config_art.tiles.units.img );
  auto& tile_set_menu =
      load_texture( config_art.tiles.menu.img );
  auto& tile_set_menu16 =
      load_texture( config_art.tiles.menu16.img );
  auto& tile_set_wood_128_64 =
      load_texture( config_art.tiles.wood.img );
  auto& tile_set_menu_sel =
      load_texture( config_art.tiles.menu_sel.img );
  auto& tile_set_button =
      load_texture( config_art.tiles.button.img );

  SET_SPRITE_WORLD( water );
  SET_SPRITE_WORLD( land );
  SET_SPRITE_WORLD( land_1_side );
  SET_SPRITE_WORLD( land_2_sides );
  SET_SPRITE_WORLD( land_3_sides );
  SET_SPRITE_WORLD( land_4_sides );
  SET_SPRITE_WORLD( land_corner );

  SET_SPRITE_WORLD( fog );
  SET_SPRITE_WORLD( fog_1_side );
  SET_SPRITE_WORLD( fog_corner );

  SET_SPRITE_WORLD( terrain_grass );

  SET_SPRITE_WORLD( panel );
  SET_SPRITE_WORLD( panel_edge_left );
  SET_SPRITE_WORLD( panel_slate );
  SET_SPRITE_WORLD( panel_slate_1_side );
  SET_SPRITE_WORLD( panel_slate_2_sides );

  SET_SPRITE_WOOD( wood_middle );
  SET_SPRITE_WOOD( wood_left_edge );

  SET_SPRITE_UNIT( free_colonist );
  SET_SPRITE_UNIT( privateer );
  SET_SPRITE_UNIT( caravel );
  SET_SPRITE_UNIT( soldier );

  SET_SPRITE_MENU( menu_top_left );
  SET_SPRITE_MENU( menu_body );
  SET_SPRITE_MENU( menu_top );
  SET_SPRITE_MENU( menu_left );
  SET_SPRITE_MENU( menu_bottom );
  SET_SPRITE_MENU( menu_bottom_left );
  SET_SPRITE_MENU( menu_right );
  SET_SPRITE_MENU( menu_top_right );
  SET_SPRITE_MENU( menu_bottom_right );

  SET_SPRITE_MENU16( menu_bar_0 );
  SET_SPRITE_MENU16( menu_bar_1 );
  SET_SPRITE_MENU16( menu_bar_2 );

  SET_SPRITE_MENU_SEL( menu_item_sel_back );
  SET_SPRITE_MENU_SEL( menu_hdr_sel_back );

  SET_SPRITE_BUTTON( button_up_ul );
  SET_SPRITE_BUTTON( button_up_um );
  SET_SPRITE_BUTTON( button_up_ur );
  SET_SPRITE_BUTTON( button_up_ml );
  SET_SPRITE_BUTTON( button_up_mm );
  SET_SPRITE_BUTTON( button_up_mr );
  SET_SPRITE_BUTTON( button_up_ll );
  SET_SPRITE_BUTTON( button_up_lm );
  SET_SPRITE_BUTTON( button_up_lr );
  SET_SPRITE_BUTTON( button_down_ul );
  SET_SPRITE_BUTTON( button_down_um );
  SET_SPRITE_BUTTON( button_down_ur );
  SET_SPRITE_BUTTON( button_down_ml );
  SET_SPRITE_BUTTON( button_down_mm );
  SET_SPRITE_BUTTON( button_down_mr );
  SET_SPRITE_BUTTON( button_down_ll );
  SET_SPRITE_BUTTON( button_down_lm );
  SET_SPRITE_BUTTON( button_down_lr );
}

void cleanup_sprites() {}

} // namespace

//
REGISTER_INIT_ROUTINE( sprites );

sprite const& lookup_sprite( g_tile tile ) {
  auto where = sprites.find( tile );
  CHECK( where != sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  return where->second;
}

void render_sprite( Texture const& tx, g_tile tile, Y pixel_row,
                    X pixel_col, int rot, int flip_x ) {
  auto where = sprites.find( tile );
  CHECK( where != sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  sprite const& sp = where->second;

  Rect dst;
  dst.x = pixel_col;
  dst.y = pixel_row;
  dst.w = W{1} * sp.scale.sx;
  dst.h = H{1} * sp.scale.sy;

  constexpr double right_angle = 90.0; // degrees

  double angle = rot * right_angle;

  SDL_RendererFlip flip =
      ( flip_x != 0 ) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

  copy_texture( *sp.texture, tx, sp.source, dst, angle, flip );
}

void render_sprite( Texture const& tx, g_tile tile,
                    Coord pixel_coord, int rot, int flip_x ) {
  render_sprite( tx, tile, pixel_coord.y, pixel_coord.x, rot,
                 flip_x );
}

void render_sprite_grid( Texture const& tx, g_tile tile,
                         Y tile_row, X tile_col, int rot,
                         int flip_x ) {
  auto const& sprite = lookup_sprite( tile );
  render_sprite( tx, tile, tile_row * sprite.scale.sy,
                 tile_col * sprite.scale.sx, rot, flip_x );
}

void render_sprite_grid( Texture const& tx, g_tile tile,
                         Coord coord, int rot, int flip_x ) {
  render_sprite_grid( tx, tile, coord.y, coord.x, rot, flip_x );
}

g_tile index_to_tile( int index ) {
  return static_cast<g_tile>( index );
}

void render_rect_of_sprites_with_border(
    Texture& dst,         // where to draw it
    Coord    dest_origin, // pixel coord of upper left
    Delta    size_tiles,  // tile coords, including border
    g_tile   middle,      //
    g_tile   top,         //
    g_tile   bottom,      //
    g_tile   left,        //
    g_tile   right,       //
    g_tile   top_left,    //
    g_tile   top_right,   //
    g_tile   bottom_left, //
    g_tile   bottom_right //
) {
  auto const& sprite_middle = lookup_sprite( middle );
  CHECK( sprite_middle.scale.sx._ == sprite_middle.scale.sy._ );
  for( auto tile : {middle, top, bottom, left, right, top_left,
                    top_right, bottom_left, bottom_right} ) {
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
    render_sprite( dst, middle, to_pixels( coord ), 0, 0 );

  for( X x = dst_tile_rect.x + 1_w;
       x < dst_tile_rect.right_edge() - 1_w; ++x )
    render_sprite( dst, top, to_pixels( {0_y, x} ), 0, 0 );
  for( X x = dst_tile_rect.x + 1_w;
       x < dst_tile_rect.right_edge() - 1_w; ++x )
    render_sprite(
        dst, bottom,
        to_pixels(
            {0_y + ( dst_tile_rect.bottom_edge() - 1_h ), x} ),
        0, 0 );
  for( Y y = dst_tile_rect.y + 1_h;
       y < dst_tile_rect.bottom_edge() - 1_h; ++y )
    render_sprite( dst, left, to_pixels( {y, 0_x} ), 0, 0 );
  for( Y y = dst_tile_rect.y + 1_h;
       y < dst_tile_rect.bottom_edge() - 1_h; ++y )
    render_sprite(
        dst, right,
        to_pixels(
            {y, 0_x + ( dst_tile_rect.right_edge() - 1_w )} ),
        0, 0 );

  render_sprite( dst, top_left, to_pixels( {0_y, 0_x} ), 0, 0 );
  render_sprite(
      dst, top_right,
      to_pixels(
          {0_y, 0_x + ( dst_tile_rect.right_edge() - 1_w )} ),
      0, 0 );
  render_sprite(
      dst, bottom_left,
      to_pixels(
          {0_y + ( dst_tile_rect.bottom_edge() - 1_h ), 0_x} ),
      0, 0 );
  render_sprite(
      dst, bottom_right,
      to_pixels( {0_y + ( dst_tile_rect.bottom_edge() - 1_h ),
                  0_x + ( dst_tile_rect.right_edge() - 1_w )} ),
      0, 0 );
}

// void render_tile_map( std::string_view name ) {
//  auto where = tile_maps.find( string( name ) );
//  CHECK( where != tile_maps.end(),
//    "failed to find tile_map "s + string( name ) );
//  auto tm = where->second;
//  for( auto const& tile : tm.tiles )
//    render_sprite_grid( index_to_tile( tile.tile ), tile.y,
//                        tile.x, tile.rot, tile.flip_x );
//}

// tile_map load_tile_map( char const* path ) {
//  ifstream in( path );
//  CHECK( in.good(),
//    "failed to open file "s + string( path ) );
//
//  tile_map tm;
//
//  string comments;
//  getline( in, comments );
//
//  while( true ) {
//    int index, tile, rot, flip_x;
//    X   x;
//    Y   y;
//    in >> index >> x >> y >> tile >> rot >> flip_x;
//    if( in.eof() || !in.good() ) break;
//    if( tile < 0 ) continue;
//    tm.tiles.push_back( {index, x, y, tile, rot, flip_x} );
//  }
//  return tm;
//}

void load_tile_maps() {
  // tile_maps["panel"] = load_tile_map( "assets/art/panel.tm" );
}

} // namespace rn
