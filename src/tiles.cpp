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
#include "../config/ucl/art.inl"

// base-util
#include "base-util/pp.hpp"

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

// sprite create_sprite_24( Texture const& texture, Coord coord )
// {
//  Rect rect{coord.x * 24_sx, coord.y * 24_sy, 24_w, 24_h};
//  return {&texture, rect, {24_sx, 24_sy}};
//}

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

#define SET_SPRITE_WORLD( name )              \
  sprites[g_tile::name] =                     \
      create_sprite_32( tile_set_world_32_32, \
                        config_art.tiles.world.coords.name )

#define SET_SPRITE_UNITS( name )              \
  sprites[g_tile::name] =                     \
      create_sprite_32( tile_set_units_32_32, \
                        config_art.tiles.units.coords.name )

#define SET_SPRITE_MENU( name )            \
  sprites[g_tile::name] = create_sprite_8( \
      tile_set_menu_8_8, config_art.tiles.menu.coords.name )

#define SET_SPRITE_MENU16( name )              \
  sprites[g_tile::name] =                      \
      create_sprite_16( tile_set_menu16_16_16, \
                        config_art.tiles.menu16.coords.name )

#define SET_SPRITE_WOOD( name )                 \
  sprites[g_tile::name] = create_sprite_128_64( \
      tile_set_wood_128_64, config_art.tiles.wood.coords.name )

#define SET_SPRITE_MENU_SEL( name )             \
  sprites[g_tile::name] = create_sprite_128_16( \
      tile_set_menu_sel_128_16,                 \
      config_art.tiles.menu_sel.coords.name )

#define SET_SPRITE_BUTTON( name )           \
  sprites[g_tile::name] =                   \
      create_sprite_8( tile_set_button_8_8, \
                       config_art.tiles.button.coords.name )

#define SET_SPRITE_COMMODITIES( name )      \
  sprites[g_tile::name] = create_sprite_16( \
      tile_set_commodities_16_16,           \
      config_art.tiles.commodities.coords.name )

#define SET_SPRITE_TESTING( name )              \
  sprites[g_tile::name] =                       \
      create_sprite_32( tile_set_testing_32_32, \
                        config_art.tiles.testing.coords.name )

#define LOAD_SPRITES_IMPL( name, width, height, suffix, ... ) \
  auto& tile_set_##name##_##width##_##height =                \
      load_texture( config_art.tiles.name.img );              \
  PP_MAP_SEMI( SET_SPRITE_##suffix, __VA_ARGS__ )

#define LOAD_SPRITES( ... ) \
  EVAL( LOAD_SPRITES_IMPL( __VA_ARGS__ ) )

void init_sprites() {
  LOAD_SPRITES( world, 32, 32, WORLD,
                water,               //
                land,                //
                land_1_side,         //
                land_2_sides,        //
                land_3_sides,        //
                land_4_sides,        //
                land_corner,         //
                fog,                 //
                fog_1_side,          //
                fog_corner,          //
                terrain_grass,       //
                panel,               //
                panel_edge_left,     //
                panel_slate,         //
                panel_slate_1_side,  //
                panel_slate_2_sides, //
  );

  LOAD_SPRITES( wood, 128, 64, WOOD,
                wood_middle,    //
                wood_left_edge, //
  );

  LOAD_SPRITES( units, 32, 32, UNITS,
                free_colonist, //
                privateer,     //
                caravel,       //
                soldier,       //
  );

  LOAD_SPRITES( menu, 8, 8, MENU,
                menu_top_left,     //
                menu_body,         //
                menu_top,          //
                menu_left,         //
                menu_bottom,       //
                menu_bottom_left,  //
                menu_right,        //
                menu_top_right,    //
                menu_bottom_right, //
  );

  LOAD_SPRITES( menu16, 16, 16, MENU16,
                menu_bar_0, //
                menu_bar_1, //
                menu_bar_2, //
  );

  LOAD_SPRITES( menu_sel, 128, 16, MENU_SEL,
                menu_item_sel_back, //
                menu_hdr_sel_back,  //
  );

  LOAD_SPRITES( button, 8, 8, BUTTON,
                button_up_ul,   //
                button_up_um,   //
                button_up_ur,   //
                button_up_ml,   //
                button_up_mm,   //
                button_up_mr,   //
                button_up_ll,   //
                button_up_lm,   //
                button_up_lr,   //
                button_down_ul, //
                button_down_um, //
                button_down_ur, //
                button_down_ml, //
                button_down_mm, //
                button_down_mr, //
                button_down_ll, //
                button_down_lm, //
                button_down_lr, //
  );

  LOAD_SPRITES( commodities, 16, 16, COMMODITIES,
                commodity_food,        //
                commodity_sugar,       //
                commodity_tobacco,     //
                commodity_cotton,      //
                commodity_fur,         //
                commodity_lumber,      //
                commodity_ore,         //
                commodity_silver,      //
                commodity_horses,      //
                commodity_rum,         //
                commodity_cigars,      //
                commodity_cloth,       //
                commodity_coats,       //
                commodity_trade_goods, //
                commodity_tools,       //
                commodity_muskets,     //
  );

  LOAD_SPRITES( testing, 32, 32, TESTING,
                checkers,     //
                checkers_inv, //
  );
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

void render_sprite_clip( Texture const& tx, g_tile tile,
                         Coord pixel_coord, Rect const& clip ) {
  auto where = sprites.find( tile );
  CHECK( where != sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  sprite const& sp = where->second;

  Rect dst;
  dst.x = pixel_coord.x;
  dst.y = pixel_coord.y;

  auto new_src = sp.source.clamp( clip.shifted_by(
      sp.source.upper_left().distance_from_origin() ) );

  dst.w = new_src.w;
  dst.h = new_src.h;

  copy_texture( *sp.texture, tx, new_src, dst, 0.0,
                ::SDL_FLIP_NONE );
}

void render_sprite( Texture const& tx, g_tile tile,
                    Coord pixel_coord, int rot, int flip_x ) {
  render_sprite( tx, tile, pixel_coord.y, pixel_coord.x, rot,
                 flip_x );
}

void render_sprite( Texture const& tx, g_tile tile,
                    Coord pixel_coord ) {
  render_sprite( tx, tile, pixel_coord.y, pixel_coord.x, 0, 0 );
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

void tile_sprite( Texture const& tx, g_tile tile,
                  Rect const& rect ) {
  auto& info = lookup_sprite( tile );
  auto  mod  = rect.delta() % info.scale;
  if( mod.w == 0_w && rect.delta().w != 0_w )
    mod.w = 1_w * info.scale.sx;
  if( mod.h == 0_h && rect.delta().h != 0_h )
    mod.h = 1_h * info.scale.sy;
  auto smaller_rect = Rect::from(
      rect.upper_left(), rect.delta() - Delta{1_w, 1_h} );
  for( auto coord :
       Rect::from( Coord{}, smaller_rect.delta() / info.scale ) )
    render_sprite( tx, tile,
                   rect.upper_left() +
                       coord.distance_from_origin() * info.scale,
                   /*rot=*/0, /*flip=*/0 );
  for( H h = 0_h; h < smaller_rect.h / info.scale.sy; ++h ) {
    auto pixel_coord =
        rect.upper_right() - mod.w + h * info.scale.sy;
    render_sprite_clip(
        tx, tile, pixel_coord,
        Rect::from( Coord{},
                    mod.with_height( 1_h * info.scale.sy ) ) );
  }
  for( W w = 0_w; w < smaller_rect.w / info.scale.sx; ++w ) {
    auto pixel_coord =
        rect.lower_left() - mod.h + w * info.scale.sx;
    render_sprite_clip(
        tx, tile, pixel_coord,
        Rect::from( Coord{},
                    mod.with_width( 1_w * info.scale.sx ) ) );
  }
  auto pixel_coord = rect.lower_right() - mod;
  render_sprite_clip( tx, tile, pixel_coord,
                      Rect::from( Coord{}, mod ) );
}

g_tile index_to_tile( int index ) {
  return static_cast<g_tile>( index );
}

void render_rect_of_sprites_with_border(
    Texture const& dst,         // where to draw it
    Coord          dest_origin, // pixel coord of upper left
    Delta          size_tiles,  // tile coords, including border
    g_tile         middle,      //
    g_tile         top,         //
    g_tile         bottom,      //
    g_tile         left,        //
    g_tile         right,       //
    g_tile         top_left,    //
    g_tile         top_right,   //
    g_tile         bottom_left, //
    g_tile         bottom_right //
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
