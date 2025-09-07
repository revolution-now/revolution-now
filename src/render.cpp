/****************************************************************
**render.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2018-08-31.
*
* Description: Performs all rendering for game.
*
*****************************************************************/
#include "render.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "colony-mgr.hpp"
#include "error.hpp"
#include "fog-conv.hpp"
#include "render-terrain.hpp"
#include "renderer.hpp"
#include "text.hpp"
#include "tiles.hpp"
#include "tribe-mgr.hpp"

// config
#include "config/land-view.rds.hpp"
#include "config/missionary.rds.hpp"
#include "config/nation.hpp"
#include "config/natives.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colony-enums.rds.hpp"
#include "ss/colony.hpp"
#include "ss/dwelling.rds.hpp"
#include "ss/nation.hpp"
#include "ss/natives.hpp"
#include "ss/players.rds.hpp"
#include "ss/ref.hpp"
#include "ss/revolution.rds.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

// Unit only, no flag.
void render_unit_no_flag( rr::Renderer& renderer, Coord where,
                          e_tile tile,
                          UnitRenderOptions const& options ) {
  if( options.shadow.has_value() )
    render_sprite_silhouette(
        renderer, where + Delta{ .w = options.shadow->offset },
        tile, options.shadow->color );
  if( options.outline_right )
    render_sprite_silhouette( renderer, where + Delta{ .w = 1 },
                              tile, options.outline_color );
  rr::Painter painter = renderer.painter();
  render_sprite( painter, Rect::from( where, g_tile_delta ),
                 tile );
}

void render_flag_pole( rr::Renderer& renderer,
                       point const where ) {
  renderer.painter().draw_vertical_line(
      where, 9, gfx::pixel::wood().shaded( 4 ) );
}

void render_colony_flag( rr::Renderer& renderer,
                         point const where, pixel const color ) {
  render_flag_pole( renderer, where );
  render_sprite_silhouette(
      renderer, where, e_tile::colony_colonial_flag, color );
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .2 );
    render_sprite( renderer, where,
                   e_tile::colony_colonial_flag_shading );
  }
}

void render_colony_america_flag( rr::Renderer& renderer,
                                 point const where ) {
  render_flag_pole( renderer, where );
  render_sprite( renderer, where, e_tile::colony_rebel_flag );
}

// Renders one flag in the stack.
void render_unit_flag_single(
    rr::Renderer& renderer, gfx::point where,
    UnitFlagRenderInfo const& flag_info ) {
  gfx::rect const rect = { .origin = where,
                           .size   = flag_info.size };

  rr::Painter painter = renderer.painter();

  painter.draw_solid_rect( rect, flag_info.background_color );
  painter.draw_empty_rect( rect,
                           rr::Painter::e_border_mode::inside,
                           flag_info.outline_color );

  SWITCH( flag_info.contents ) {
    CASE( character ) {
      string const text( 1, character.value );
      Delta char_size =
          renderer.typer().dimensions_for_line( text );
      render_text( renderer, centered( char_size, rect ),
                   font::nat_icon(), character.color, text );
      break;
    }
    CASE( icon ) {
      render_sprite( renderer, where.moved_down().moved_right(),
                     icon.tile );
      break;
    }
  }
}

void render_unit_flag( rr::Renderer& renderer, Coord where,
                       UnitFlagRenderInfo const& flag_info ) {
  if( flag_info.stacked )
    render_unit_flag_single(
        renderer,
        where.to_gfx() + flag_info.offsets.offset_stacked,
        flag_info );
  render_unit_flag_single(
      renderer, where.to_gfx() + flag_info.offsets.offset_first,
      flag_info );
}

void render_unit_with_tile( rr::Renderer& renderer, Coord where,
                            e_tile tile, bool damaged,
                            UnitRenderOptions const& options ) {
  if( !options.flag.has_value() ) {
    // No flag.
    render_unit_no_flag( renderer, where, tile, options );
  } else if( !options.flag->in_front ) {
    // Show the flag but in the back. This is a bit tricky if
    // there's a shadow because we don't want the shadow to be
    // over the flag.
    if( options.shadow.has_value() )
      render_sprite_silhouette(
          renderer, where + Delta{ .w = options.shadow->offset },
          tile, options.shadow->color );
    render_unit_flag( renderer, where, *options.flag );
    if( options.shadow.has_value() ) {
      // Draw a light shadow over the flag so that we can dif-
      // ferentiate the edge of the unit from the flag, but not
      // so dark that it will cover up the flag.
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .35 );
      render_sprite_silhouette(
          renderer, where + Delta{ .w = options.shadow->offset },
          tile, options.shadow->color );
    }
    render_sprite( renderer, where, tile );
  } else {
    // Show the flag in the front.
    //
    // The idea here is that we need it to be in front to be vis-
    // ible, but we don't want to fully hide what is behind it,
    // in an effort to make it look better. So this tries to get
    // the best of both worlds by rendering it behind at full
    // opacity, then again over the sprite at only partial
    // opacity so that both the flag contents can be seen as well
    // as the part of the sprite that would have been hidden. At
    // the time of writing this is only used for the Man-o-War,
    // but seems to make it look a bit better. That said, this
    // effect only looks good when there is a single flag, i.e.
    // not a stack of units.
    if( !options.flag->stacked ) {
      render_unit_flag( renderer, where, *options.flag );
      render_unit_no_flag( renderer, where, tile, options );
      {
        SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
        render_unit_flag( renderer, where, *options.flag );
      }
    } else {
      render_unit_no_flag( renderer, where, tile, options );
      render_unit_flag( renderer, where, *options.flag );
    }
  }

  if( damaged )
    // Reuse the red X from boycotted commodities for the damaged
    // icon (the OG seems to do this).
    render_sprite( renderer, where + Delta{ .w = 8, .h = 8 },
                   e_tile::red_x_12 );
}

void depixelate_from_to( rr::Renderer& renderer, double stage,
                         gfx::point hash_anchor,
                         base::function_ref<void()> from,
                         base::function_ref<void()> to ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                           hash_anchor );
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  // The ordering of these should actually not matter, because we
  // won't be rendering any pixels that contain both.
  {
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.inverted,
                             true );
    to();
  }
  from();
}

void render_unit_depixelate_to_impl(
    rr::Renderer& renderer, Coord where, auto const& from_desc,
    auto const& target_desc, double stage,
    UnitRenderOptions from_options,
    UnitRenderOptions target_options ) {
  depixelate_from_to(
      renderer, stage, /*anchor=*/where,
      /*from=*/
      [&] {
        render_unit_type( renderer, where, from_desc.type,
                          from_options );
      },
      /*to=*/
      [&] {
        render_unit_type( renderer, where, target_desc.type,
                          target_options );
      } );
}

// We really don't want to compute the dulled colors everytime we
// render a dwelling, so we will cache it.
gfx::pixel missionary_cross_color( e_player player,
                                   bool is_jesuit ) {
  static auto dulled = [] {
    refl::enum_map<e_player, gfx::pixel> m;
    for( e_player player : refl::enum_values<e_player> ) {
      gfx::pixel const bright_color =
          config_nation.players[player].flag_color;
      gfx::pixel_hsl hsl = to_HSL( bright_color );
      hsl.s *= config_missionary
                   .saturation_reduction_for_non_jesuit_cross;
      m[player] = to_RGB( hsl );
    }
    return m;
  }();
  if( is_jesuit )
    return config_nation.players[player].flag_color;
  return dulled[player];
}

// Burrowing refers to the mechanism that allows e.g. the native
// dwellings and colonies to appear like they are nested in the
// forest instead of just appearing to hover over the forest
// tiles. For each sprite that supports burrowing, a second
// sprite is precomputed that marks the bottom edge of the opaque
// region of the sprite with some depixelation stages so that the
// forest can then be drawn over that depixelation guide to make
// it appear that the forest grow is encroaching on the bottom
// each of the dwelling/colony sprite.
//
// NOTE: Here "large" means larger than a standard 32x32 tile.
void render_large_sprite_with_forest_burrowing(
    IVisibility const& viz, rr::Renderer& renderer,
    point const pixel_coord, point const center_tile,
    e_tile const sprite_tile ) {
  render_sprite( renderer, pixel_coord, sprite_tile );

  static size const kMapTileSz = size{ .w = 32, .h = 32 };
  size const sprite_sz         = sprite_size( sprite_tile );
  size const sprite_origin_pixel_delta =
      ( sprite_sz - kMapTileSz ) / 2;
  point const center_map_tile_pixel_coord =
      pixel_coord + sprite_origin_pixel_delta;

  auto const burrow_tile = [&]( e_cdirection const d ) {
    point const map_tile = center_tile.moved( d );
    size const tile_delta_from_center =
        ( map_tile - center_tile );
    auto const section =
        rect{ .size = kMapTileSz }.clipped_by( rect{
          .origin = point{} - sprite_origin_pixel_delta -
                    size( tile_delta_from_center * kMapTileSz ),
          .size = sprite_sz } );
    if( !section.has_value() ) return;
    auto const forest_tile = forest_tile_for( viz, map_tile );
    if( !forest_tile.has_value() ) return;
    point const moved_pixel_coord =
        center_map_tile_pixel_coord +
        size( kMapTileSz * tile_delta_from_center );
    point const hash_anchor = [&] {
      point const anchor_tile =
          ( map_tile / size{ .w = 10, .h = 10 } ) *
          size{ .w = 10, .h = 10 };
      size const pixel_delta =
          ( map_tile - anchor_tile ) * kMapTileSz;
      point const hash_anchor = moved_pixel_coord - pixel_delta;
      return hash_anchor;
    }();
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.inverted,
                             true );
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                             hash_anchor );
    SCOPED_RENDERER_MOD_SET(
        painter_mods.depixelate.textured,
        burrowed_sprite_plan_for( *forest_tile, sprite_tile,
                                  tile_delta_from_center ) )
    render_sprite_section(
        renderer, *forest_tile,
        moved_pixel_coord +
            section->origin.distance_from_origin(),
        *section );
  };

  // For burrow only need to cover the middle and bottom portion.
  burrow_tile( e_cdirection::w );
  burrow_tile( e_cdirection::c );
  burrow_tile( e_cdirection::e );
  burrow_tile( e_cdirection::sw );
  burrow_tile( e_cdirection::s );
  burrow_tile( e_cdirection::se );
}

} // namespace

/****************************************************************
** UnitShadow
*****************************************************************/
gfx::pixel UnitShadow::default_color() {
  return gfx::pixel{ .r = 10, .g = 20, .b = 10, .a = 255 };
}

W UnitShadow::default_offset() { return W{ -3 }; }

/****************************************************************
** Unit Rendering.
*****************************************************************/
e_tile tile_for_unit_type( e_unit_type const unit_type ) {
  return unit_attr( unit_type ).tile;
}

rect trimmed_area_for_unit_type( e_unit_type const unit_type ) {
  return trimmed_area_for( tile_for_unit_type( unit_type ) );
}

void render_unit( rr::Renderer& renderer, Coord where,
                  Unit const& unit,
                  UnitRenderOptions const& options ) {
  render_unit_with_tile(
      renderer, where, unit.desc().tile,
      unit.orders().holds<unit_orders::damaged>(), options );
}

void render_native_unit( rr::Renderer& renderer, Coord where,
                         NativeUnit const& native_unit,
                         UnitRenderOptions const& options ) {
  render_unit_with_tile( renderer, where,
                         unit_attr( native_unit.type ).tile,
                         /*damaged=*/false, options );
}

void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_unit_type unit_type,
                       UnitRenderOptions const& options ) {
  render_unit_with_tile( renderer, where,
                         unit_attr( unit_type ).tile,
                         /*damaged=*/false, options );
}

void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_native_unit_type unit_type,
                       UnitRenderOptions const& options ) {
  render_unit_with_tile( renderer, where,
                         unit_attr( unit_type ).tile,
                         /*damaged=*/false, options );
}

void render_unit_depixelate( rr::Renderer& renderer, Coord where,
                             Unit const& unit, double stage,
                             UnitRenderOptions const& options ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  render_unit( renderer, where, unit, options );
}

void render_native_unit_depixelate(
    rr::Renderer& renderer, Coord where, NativeUnit const& unit,
    double stage, UnitRenderOptions const& options ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  render_native_unit( renderer, where, unit, options );
}

void render_unit_depixelate_to(
    rr::Renderer& renderer, Coord where, Unit const& unit,
    e_unit_type target, double stage,
    UnitRenderOptions const& from_options,
    UnitRenderOptions const& target_options ) {
  render_unit_depixelate_to_impl( renderer, where, unit.desc(),
                                  unit_attr( target ), stage,
                                  from_options, target_options );
}

void render_native_unit_depixelate_to(
    rr::Renderer& renderer, Coord where, NativeUnit const& unit,
    e_native_unit_type target, double stage,
    UnitRenderOptions const& from_options,
    UnitRenderOptions const& target_options ) {
  render_unit_depixelate_to_impl(
      renderer, where, unit_attr( unit.type ),
      unit_attr( target ), stage, from_options, target_options );
}

/****************************************************************
** Colony Rendering.
*****************************************************************/
static maybe<e_tile> back_walls_tile_for_colony(
    Colony const& colony ) {
  e_colony_barricade_type const barricade_type =
      barricade_for_colony( colony );
  switch( barricade_type ) {
    case e_colony_barricade_type::none:
      return nothing;
    case e_colony_barricade_type::stockade:
      return e_tile::colony_stockade_walls_back;
    case e_colony_barricade_type::fort:
      return e_tile::colony_fort_walls_back;
    case e_colony_barricade_type::fortress:
      return e_tile::colony_fortress_walls_back;
  }
}

static maybe<e_tile> front_walls_tile_for_colony(
    Colony const& colony ) {
  e_colony_barricade_type const barricade_type =
      barricade_for_colony( colony );
  switch( barricade_type ) {
    case e_colony_barricade_type::none:
      return nothing;
    case e_colony_barricade_type::stockade:
      return e_tile::colony_stockade_walls_front;
    case e_colony_barricade_type::fort:
      return e_tile::colony_fort_walls_front;
    case e_colony_barricade_type::fortress:
      return e_tile::colony_fortress_walls_front;
  }
}

e_tile houses_tile_for_colony( Colony const& colony ) {
  e_colony_barricade_type const barricade_type =
      barricade_for_colony( colony );
  switch( barricade_type ) {
    case e_colony_barricade_type::none:
      return e_tile::colony_basic_houses;
    case e_colony_barricade_type::stockade:
      return e_tile::colony_stockade_houses;
    case e_colony_barricade_type::fort:
      return e_tile::colony_fort_houses;
    case e_colony_barricade_type::fortress:
      return e_tile::colony_fortress_houses;
  }
}

void render_colony( rr::Renderer& renderer, Coord const where,
                    IVisibility const& viz, point const map_tile,
                    SSConst const& ss, Colony const& colony,
                    ColonyRenderOptions const& options ) {
  e_tile const tile      = houses_tile_for_colony( colony );
  auto const walls_back  = back_walls_tile_for_colony( colony );
  auto const walls_front = front_walls_tile_for_colony( colony );
  bool const has_walls =
      walls_back.has_value() && walls_front.has_value();
  if( has_walls ) {
    render_sprite( renderer, where, *walls_back );
    render_sprite( renderer, where, tile );
    render_large_sprite_with_forest_burrowing(
        viz, renderer, where, map_tile, *walls_front );
  } else {
    render_large_sprite_with_forest_burrowing(
        viz, renderer, where, map_tile, tile );
  }
  auto const& player_conf = player_obj( colony.player );
  if( options.render_flag ) {
    UNWRAP_CHECK_T( Player const& player,
                    ss.players.players[colony.player] );
    point const flag_pos =
        where.to_gfx().moved_right( 9 ).moved_down( 2 );
    if( is_ref( colony.player ) ) {
      e_player const colonial_player_type =
          colonial_player_for( nation_for( colony.player ) );
      auto const& colonial_player_conf =
          player_obj( colonial_player_type );
      render_colony_flag( renderer, flag_pos,
                          colonial_player_conf.flag_color );
    } else {
      if( player.revolution.status >=
          e_revolution_status::declared )
        render_colony_america_flag( renderer, flag_pos );
      else
        render_colony_flag( renderer, flag_pos,
                            player_conf.flag_color );
    }
  }
  if( options.render_population ) {
    FrozenColony const frozen_colony =
        colony_to_frozen_colony( ss, colony );
    int const population = colony_population( colony );
    // TODO: needs to be centered.
    Coord const population_coord =
        where + Delta{ .w = 44 / 2 - 1, .h = 44 / 2 - 4 };
    gfx::pixel const color =
        ( frozen_colony.sons_of_liberty_integral_percent == 100 )
            ? gfx::pixel{ .r = 0, .g = 255, .b = 255, .a = 255 }
        : ( frozen_colony.sons_of_liberty_integral_percent >=
            50 )
            ? gfx::pixel{ .r = 0, .g = 255, .b = 0, .a = 255 }
            : gfx::pixel::white();
    render_text_markup( renderer, population_coord, e_font{},
                        rr::TextLayout{},
                        TextMarkupInfo{ .normal = color },
                        fmt::to_string( population ) );
  }
  if( options.render_name ) {
    Coord const name_coord =
        where + config_land_view.colony_name_offset;
    render_text_markup(
        renderer, name_coord, config_land_view.colony_name_font,
        rr::TextLayout{},
        TextMarkupInfo{ .highlight = gfx::pixel::white(),
                        .shadow    = gfx::pixel::black() },
        fmt::format( "[{}]", colony.name ) );
  }
}

/****************************************************************
** Dwelling Rendering.
*****************************************************************/
e_tile dwelling_tile_for_tribe( e_tribe const tribe_type ) {
  return config_natives.tribes[tribe_type].dwelling_tile;
}

e_tile tile_for_dwelling( SSConst const& ss,
                          Dwelling const& dwelling ) {
  // Need to use the tribe_for_dwelling method because this may
  // be a non-existent (fogged) dwelling in which case a lookup
  // for its tribe would fail.
  e_tribe const tribe_type =
      tribe_type_for_dwelling( ss, dwelling );
  return dwelling_tile_for_tribe( tribe_type );
}

void render_dwelling( rr::Renderer& renderer, point const where,
                      IVisibility const& viz,
                      point const map_tile, SSConst const& ss,
                      Dwelling const& dwelling ) {
  rr::Painter painter = renderer.painter();
  FrozenDwelling const frozen_dwelling =
      dwelling_to_frozen_dwelling( ss, dwelling );
  // NOTE: !! From here on we must not use ss to look up anything
  // that would fail if the dwelling no longer exists, since we
  // may in fact be rendering a fogged dwelling that does not ex-
  // ist. All info needed should be available either in `d-
  // welling` or `frozen_dwelling`.
  e_tribe const tribe_type = frozen_dwelling.tribe;
  auto& tribe_conf         = config_natives.tribes[tribe_type];
  e_tile const dwelling_tile =
      dwelling_tile_for_tribe( tribe_type );
  render_large_sprite_with_forest_burrowing(
      viz, renderer, where, map_tile, dwelling_tile );
  // Flags.
  e_native_level const native_level = tribe_conf.level;
  gfx::pixel const flag_color       = tribe_conf.flag_color;
  // FIXME: when the dwelling is depixelating, it seems to show
  // the underlying red for its flag.
  for( gfx::rect flag : config_natives.flag_rects[native_level] )
    painter.draw_solid_rect( flag.origin_becomes_point( where ),
                             flag_color );
  // The offset that we need to go to get to the upper left
  // corner of a 32x32 sprite centered on the (48x48) dwelling
  // tile.
  size const offset_32x32{ .w = 6, .h = 6 };
  // Yellow star to mark the capital.
  if( dwelling.is_capital )
    render_sprite( renderer, where + offset_32x32,
                   e_tile::capital_star );

  // If there is a missionary in this dwelling then render a
  // cross on top of it with the color of the nation's flag. It
  // will be dulled if the missionary is a non-jesuit.
  maybe<FrozenMission> const& frozen_mission =
      frozen_dwelling.mission;
  if( frozen_mission.has_value() ) {
    bool const is_jesuit =
        ( frozen_mission->level == e_missionary_type::jesuit );
    gfx::pixel const cross_color = missionary_cross_color(
        frozen_mission->player, is_jesuit );
    // This is the inner part that is colored according to the
    // nation's flag color.
    render_sprite_silhouette( renderer, where + offset_32x32,
                              e_tile::missionary_cross_inner,
                              cross_color );
    render_sprite( renderer, where + offset_32x32,
                   e_tile::missionary_cross_outter );
    // This part adds some shadows/highlights to the colored part
    // of the cross.
    {
      double const alpha = is_jesuit ? .10 : .07;
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
      render_sprite( renderer, where + offset_32x32,
                     e_tile::missionary_cross_accent );
    }
  }
}

} // namespace rn
