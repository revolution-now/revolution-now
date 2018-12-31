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

#include <fstream>
#include <iostream>
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
  Rect rect{coord.x * g_tile_width, coord.y * g_tile_height,
            W{1} * g_tile_width, H{1} * g_tile_height};
  return {&texture, rect, W{1} * g_tile_width,
          H{1} * g_tile_height};
}

#define SET_SPRITE( name ) \
  sprites[g_tile::name] =  \
      create_sprite_32( tile_set, config_art.tiles.name )

void load_sprites() {
  auto& tile_set = load_texture( config_art.tiles_png.c_str() );

  SET_SPRITE( water );
  SET_SPRITE( land );
  SET_SPRITE( land_1_side );
  SET_SPRITE( land_2_sides );
  SET_SPRITE( land_3_sides );
  SET_SPRITE( land_4_sides );
  SET_SPRITE( land_corner );

  SET_SPRITE( fog );
  SET_SPRITE( fog_1_side );
  SET_SPRITE( fog_corner );

  SET_SPRITE( terrain_grass );

  SET_SPRITE( panel );
  SET_SPRITE( panel_edge_left );
  SET_SPRITE( panel_slate );
  SET_SPRITE( panel_slate_1_side );
  SET_SPRITE( panel_slate_2_sides );

  SET_SPRITE( free_colonist );
  SET_SPRITE( caravel );
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
  dst.w = sp.w;
  dst.h = sp.h;

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
  render_sprite( tx, tile, tile_row * g_tile_height,
                 tile_col * g_tile_width, rot, flip_x );
}

g_tile index_to_tile( int index ) {
  return static_cast<g_tile>( index );
}

// void render_tile_map( std::string_view name ) {
//  auto where = tile_maps.find( string( name ) );
//  if( where == tile_maps.end() )
//    DIE( "failed to find tile_map "s + string( name ) );
//  auto tm = where->second;
//  for( auto const& tile : tm.tiles )
//    render_sprite_grid( index_to_tile( tile.tile ), tile.y,
//                        tile.x, tile.rot, tile.flip_x );
//}

// tile_map load_tile_map( char const* path ) {
//  ifstream in( path );
//  if( !in.good() )
//    DIE( "failed to open file "s + string( path ) );
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
