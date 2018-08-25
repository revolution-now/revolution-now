/****************************************************************
* tiles.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-25.
*
* Description: Handles loading and retrieving tiles
*
*****************************************************************/

#include "tiles.hpp"

#include "base-util.hpp"
#include "globals.hpp"
#include "global-constants.hpp"
#include "macros.hpp"
#include "sdl-util.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::literals::string_literals;

namespace rn {

namespace {

struct tile_map {
  struct one_tile { int index, x, y, tile, rot, flip_x; };
  vector<one_tile> tiles;
};

unordered_map<g_tile, sprite, EnumClassHash> sprites;
unordered_map<std::string, tile_map> tile_maps;

} // namespace

sprite create_sprite_32( SDL_Texture* texture, int row, int col ) {
  SDL_Rect rect{ col*g_tile_width, row*g_tile_height,
                     g_tile_width, g_tile_height };
  return { texture, rect, 32, 32 };
}

void load_sprites() {
  auto land_tiles = load_texture( "../art/land-tiles.png" );

  sprites[g_tile::water]        = create_sprite_32( land_tiles, 0, 0 );
  sprites[g_tile::land]         = create_sprite_32( land_tiles, 0, 1 );
  sprites[g_tile::land_1_side]  = create_sprite_32( land_tiles, 0, 2 );
  sprites[g_tile::land_2_sides] = create_sprite_32( land_tiles, 0, 3 );
  sprites[g_tile::land_3_sides] = create_sprite_32( land_tiles, 0, 4 );
  sprites[g_tile::land_4_sides] = create_sprite_32( land_tiles, 0, 5 );
  sprites[g_tile::land_corner]  = create_sprite_32( land_tiles, 0, 6 );

  sprites[g_tile::fog]        = create_sprite_32( land_tiles, 1, 0 );
  sprites[g_tile::fog_1_side] = create_sprite_32( land_tiles, 1, 1 );
  sprites[g_tile::fog_corner] = create_sprite_32( land_tiles, 1, 2 );

  sprites[g_tile::terrain_grass] = create_sprite_32( land_tiles, 2, 0 );

  sprites[g_tile::panel]               = create_sprite_32( land_tiles, 3, 0 );
  sprites[g_tile::panel_edge_left]     = create_sprite_32( land_tiles, 3, 1 );
  sprites[g_tile::panel_slate]         = create_sprite_32( land_tiles, 3, 2 );
  sprites[g_tile::panel_slate_1_side]  = create_sprite_32( land_tiles, 3, 3 );
  sprites[g_tile::panel_slate_2_sides] = create_sprite_32( land_tiles, 3, 4 );
}

sprite const& lookup_sprite( g_tile tile ) {
  auto where = sprites.find( tile );
  if( where == sprites.end() )
    DIE( "failed to find sprite "s + std::to_string( static_cast<int>( tile ) ) );
  return where->second;
}

void render_sprite( g_tile tile, int pixel_row, int pixel_col, int rot, int flip_x  ) {
  auto where = sprites.find( tile );
  if( where == sprites.end() )
    DIE( "failed to find sprite "s + std::to_string( static_cast<int>( tile ) ) );
  sprite const& sp = where->second;

  int scale = g_display_scaled ? 2 : 1;

  SDL_Rect destination;
  destination.x = pixel_col*scale;
  destination.y = pixel_row*scale;
  destination.w = sp.w*scale;
  destination.h = sp.h*scale;

  double angle = rot*90.0;

  SDL_RendererFlip flip = flip_x ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;

  render_texture( sp.texture, sp.source, destination, angle, flip );
}

void render_sprite_grid( g_tile tile, int tile_row, int tile_col, int rot, int flip_x  ) {
  render_sprite( tile, tile_row*g_tile_height, tile_col*g_tile_width,
                 rot, flip_x );
}

g_tile index_to_tile( int index ) {
  return static_cast<g_tile>( index );
}

void render_tile_map( std::string_view name ) {
  auto where = tile_maps.find( string( name ) );
  if( where == tile_maps.end() )
    DIE( "failed to find tile_map "s + string( name ) );
  auto tm = where->second;
  for( auto const& tile : tm.tiles )
    render_sprite_grid( index_to_tile( tile.tile ), tile.y, tile.x, tile.rot, tile.flip_x );
}

tile_map load_tile_map( char const* path ) {

  ifstream in( path );
  if( !in.good() )
    DIE( "failed to open file "s + string( path ) );

  tile_map tm;

  string comments;
  getline( in, comments );

  while( true ) {
    int index, tile, x, y, rot, flip_x;
    in >> index >> x >> y >> tile >> rot >> flip_x;
    if( in.eof() || !in.good() )
      break;
    if( tile < 0 )
      continue;
    tm.tiles.push_back( {index, x, y, tile, rot, flip_x} );
  }
  return tm;
}

void load_tile_maps() {
  tile_maps["panel"] = load_tile_map( "../art/panel.tm" );
}

} // namespace rn

