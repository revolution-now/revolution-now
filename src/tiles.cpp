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

// Revolution Now (config)
#include "../config/ucl/art.inl"

// base-util
#include "base-util/fs.hpp"
#include "base-util/keyval.hpp"
#include "base-util/pp.hpp"

// magic_enum
#include "magic_enum.hpp"

// C++ standard library
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
using namespace std::literals::string_literals;

namespace rn {

static_assert(
    magic_enum::enum_count<e_tile>() < MAGIC_ENUM_RANGE_MAX,
    "The e_tile enum has gotten larger than magic-enum can "
    "handle.  Either increase the max size that magic-enum can "
    "handle or switch to a generated enum." );

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
NOTHROW_MOVE( tile_map );

// Need pointer stability for these.
unordered_map<fs::path, Texture>             g_images;
unordered_map<e_tile, sprite, EnumClassHash> g_sprites;
unordered_map<std::string, tile_map>         tile_maps;

Texture const& load_image( fs::path const& p ) {
  if( !bu::has_key( g_images, p ) )
    g_images.emplace( p, Texture::load_image( p ) );
  return g_images[p];
}

sprite create_sprite( Texture const& texture, Coord coord,
                      Delta size ) {
  auto scale = size.to_scale();
  Rect rect{ coord.x * scale.sx, coord.y * scale.sy, size.w,
             size.h };
  return { &texture, rect, scale };
}

#define SET_SPRITE( category, tile_name )         \
  g_sprites[e_tile::tile_name] = create_sprite(   \
      tile_set_texture,                           \
      config_art.tiles.category.coords.tile_name, \
      config_art.tiles.category.size );

#define LOAD_SPRITES( name, ... )                            \
  {                                                          \
    auto& tile_set_texture =                                 \
        load_image( config_art.tiles.name.img );             \
    PP_MAP_TUPLE( SET_SPRITE,                                \
                  PP_MAP_PREPEND_TUPLE(                      \
                      name, PP_MAP_COMMAS( SINGLETON_TUPLE,  \
                                           __VA_ARGS__ ) ) ) \
  }

void init_sprites() {
  // clang-format off
  EVAL( PP_MAP_TUPLE(LOAD_SPRITES,
  (  /*-------------------*/world,/*-------------------*/
       water,
       land,
       land_1_side,
       land_2_sides,
       land_3_sides,
       land_4_sides,
       land_corner,
       fog,
       fog_1_side,
       fog_corner,
       terrain_grass,
       panel,
       panel_edge_left,
       panel_slate,
       panel_slate_1_side,
       panel_slate_2_sides,
  ),
  (  /*-------------------*/wood,/*-------------------*/
       wood_middle,
       wood_left_edge,
  ),
  (  /*-------------------*/units,/*-------------------*/
       free_colonist,
       privateer,
       merchantman,
       soldier,
       large_treasure,
       small_treasure,
  ),
  (  /*-------------------*/menu,/*-------------------*/
       menu_top_left,
       menu_body,
       menu_top,
       menu_left,
       menu_bottom,
       menu_bottom_left,
       menu_right,
       menu_top_right,
       menu_bottom_right,
  ),
  (  /*-------------------*/menu16,/*-------------------*/
       menu_bar_0,
       menu_bar_1,
       menu_bar_2,
  ),
  (  /*-------------------*/menu_sel,/*-------------------*/
       menu_item_sel_back,
       menu_hdr_sel_back,
  ),
  (  /*-------------------*/button,/*-------------------*/
       button_up_ul,
       button_up_um,
       button_up_ur,
       button_up_ml,
       button_up_mm,
       button_up_mr,
       button_up_ll,
       button_up_lm,
       button_up_lr,
       button_down_ul,
       button_down_um,
       button_down_ur,
       button_down_ml,
       button_down_mm,
       button_down_mr,
       button_down_ll,
       button_down_lm,
       button_down_lr,
  ),
  (  /*-------------------*/commodities,/*-------------------*/
       commodity_food,
       commodity_sugar,
       commodity_tobacco,
       commodity_cotton,
       commodity_fur,
       commodity_lumber,
       commodity_ore,
       commodity_silver,
       commodity_horses,
       commodity_rum,
       commodity_cigars,
       commodity_cloth,
       commodity_coats,
       commodity_trade_goods,
       commodity_tools,
       commodity_muskets,
  ),
  (  /*-------------------*/colony,/*-------------------*/
       colony_basic,
       colony_stockade,
  ),
  (  /*-------------------*/mouse,/*-------------------*/
       mouse_planks,
       mouse_musket,
       mouse_arrow1,
  ),
  (  /*-------------------*/testing,/*-------------------*/
       checkers,
       checkers_inv,
  )
  ));
  // clang-format on

  for( e_tile e : magic_enum::enum_values<e_tile>() ) {
    CHECK( bu::has_key( g_sprites, e ),
           "Tile `{}` has not been loaded.", e );
  }
  auto num_tiles        = magic_enum::enum_count<e_tile>();
  auto num_tiles_loaded = g_sprites.size();
  CHECK( num_tiles_loaded == num_tiles,
         "There are {} tiles but {} were loaded.", num_tiles,
         num_tiles_loaded );
}

void cleanup_sprites() {
  for( auto& p : g_images ) p.second.free();
}

} // namespace

//
REGISTER_INIT_ROUTINE( sprites );

sprite const& lookup_sprite( e_tile tile ) {
  auto where = g_sprites.find( tile );
  CHECK( where != g_sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  return where->second;
}

void render_sprite( Texture& tx, e_tile tile, Y pixel_row,
                    X pixel_col, int rot, int flip_x ) {
  auto where = g_sprites.find( tile );
  CHECK( where != g_sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  sprite const& sp = where->second;

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

void render_sprite_clip( Texture& tx, e_tile tile,
                         Coord pixel_coord, Rect const& clip ) {
  auto where = g_sprites.find( tile );
  CHECK( where != g_sprites.end(), "failed to find sprite {}",
         std::to_string( static_cast<int>( tile ) ) );
  sprite const& sp = where->second;

  Rect dst;
  dst.x = pixel_coord.x;
  dst.y = pixel_coord.y;

  auto new_src = sp.source.clamp( clip.shifted_by(
      sp.source.upper_left().distance_from_origin() ) );

  dst.w = new_src.w;
  dst.h = new_src.h;

  sp.texture->copy_to( tx, new_src, dst, 0.0, e_flip::none );
}

void render_sprite( Texture& tx, e_tile tile, Coord pixel_coord,
                    int rot, int flip_x ) {
  render_sprite( tx, tile, pixel_coord.y, pixel_coord.x, rot,
                 flip_x );
}

void render_sprite( Texture& tx, e_tile tile,
                    Coord pixel_coord ) {
  render_sprite( tx, tile, pixel_coord.y, pixel_coord.x, 0, 0 );
}

void render_sprite_grid( Texture& tx, e_tile tile, Y tile_row,
                         X tile_col, int rot, int flip_x ) {
  auto const& sprite = lookup_sprite( tile );
  render_sprite( tx, tile, tile_row * sprite.scale.sy,
                 tile_col * sprite.scale.sx, rot, flip_x );
}

void render_sprite_grid( Texture& tx, e_tile tile, Coord coord,
                         int rot, int flip_x ) {
  render_sprite_grid( tx, tile, coord.y, coord.x, rot, flip_x );
}

void tile_sprite( Texture& tx, e_tile tile, Rect const& rect ) {
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

e_tile index_to_tile( int index ) {
  return static_cast<e_tile>( index );
}

void render_rect_of_sprites_with_border(
    Texture& dst,         // where to draw it
    Coord    dest_origin, // pixel coord of upper left
    Delta    size_tiles,  // tile coords, including border
    e_tile   middle,      //
    e_tile   top,         //
    e_tile   bottom,      //
    e_tile   left,        //
    e_tile   right,       //
    e_tile   top_left,    //
    e_tile   top_right,   //
    e_tile   bottom_left, //
    e_tile   bottom_right //
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
    render_sprite( dst, middle, to_pixels( coord ), 0, 0 );

  for( X x = dst_tile_rect.x + 1_w;
       x < dst_tile_rect.right_edge() - 1_w; ++x )
    render_sprite( dst, top, to_pixels( { 0_y, x } ), 0, 0 );
  for( X x = dst_tile_rect.x + 1_w;
       x < dst_tile_rect.right_edge() - 1_w; ++x )
    render_sprite(
        dst, bottom,
        to_pixels(
            { 0_y + ( dst_tile_rect.bottom_edge() - 1_h ), x } ),
        0, 0 );
  for( Y y = dst_tile_rect.y + 1_h;
       y < dst_tile_rect.bottom_edge() - 1_h; ++y )
    render_sprite( dst, left, to_pixels( { y, 0_x } ), 0, 0 );
  for( Y y = dst_tile_rect.y + 1_h;
       y < dst_tile_rect.bottom_edge() - 1_h; ++y )
    render_sprite(
        dst, right,
        to_pixels(
            { y, 0_x + ( dst_tile_rect.right_edge() - 1_w ) } ),
        0, 0 );

  render_sprite( dst, top_left, to_pixels( { 0_y, 0_x } ), 0,
                 0 );
  render_sprite(
      dst, top_right,
      to_pixels(
          { 0_y, 0_x + ( dst_tile_rect.right_edge() - 1_w ) } ),
      0, 0 );
  render_sprite(
      dst, bottom_left,
      to_pixels(
          { 0_y + ( dst_tile_rect.bottom_edge() - 1_h ), 0_x } ),
      0, 0 );
  render_sprite(
      dst, bottom_right,
      to_pixels(
          { 0_y + ( dst_tile_rect.bottom_edge() - 1_h ),
            0_x + ( dst_tile_rect.right_edge() - 1_w ) } ),
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
