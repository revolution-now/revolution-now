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

#include "config-files.hpp"
#include "errors.hpp"
#include "globals.hpp"
#include "sdl-util.hpp"
#include "util.hpp"

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

} // namespace

sprite create_sprite_32( Texture const& texture, Coord coord ) {
  Rect rect{coord.x * 32_sx, coord.y * 32_sy, 32_w, 32_h};
  return {&texture, rect, 32_sx, 32_sy};
}

sprite create_sprite_8( Texture const& texture, Coord coord ) {
  Rect rect{coord.x * 8_sx, coord.y * 8_sy, 8_w, 8_h};
  return {&texture, rect, 8_sx, 8_sy};
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

void load_sprites() {
  auto& tile_set_world =
      load_texture( config_art.tiles.world.img );
  auto& tile_set_units =
      load_texture( config_art.tiles.units.img );
  auto& tile_set_menu =
      load_texture( config_art.tiles.menu.img );

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
  SET_SPRITE_MENU( menu_sel_body );
  SET_SPRITE_MENU( menu_sel_left );
  SET_SPRITE_MENU( menu_sel_right );

  SET_SPRITE_MENU( menu_bar_bottom );
  SET_SPRITE_MENU( menu_bar_top );
}

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
  dst.w = W{1} * sp.sx;
  dst.h = H{1} * sp.sy;

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
  render_sprite( tx, tile, tile_row * sprite.sy,
                 tile_col * sprite.sx, rot, flip_x );
}

g_tile index_to_tile( int index ) {
  return static_cast<g_tile>( index );
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
