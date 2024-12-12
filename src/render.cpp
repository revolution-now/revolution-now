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
#include "text.hpp"
#include "tiles.hpp"

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
#include "ss/natives.hpp"
#include "ss/units.hpp"

// rds
#include "rds/switch-macro.hpp"

using namespace std;

namespace rn {

namespace {

// Unit only, no flag.
void render_unit_no_flag( rr::Renderer& renderer, Coord where,
                          e_tile                   tile,
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

void render_colony_flag( rr::Painter& painter, Coord coord,
                         gfx::pixel color ) {
  auto cloth_rect = Rect::from( coord, Delta{ .w = 8, .h = 6 } );
  painter.draw_solid_rect( cloth_rect, color );
  painter.draw_vertical_line( cloth_rect.upper_right(), 12,
                              gfx::pixel::wood().shaded( 4 ) );
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
      Delta        char_size = Delta::from_gfx(
          rr::rendered_text_line_size_pixels( text ) );
      render_text( renderer,
                   centered( char_size, Rect::from_gfx( rect ) ),
                   font::nat_icon(), character.color, text );
      break;
    }
    CASE( icon ) {
      render_sprite(
          renderer,
          Coord::from_gfx( where.moved_down().moved_right() ),
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
    render_unit_no_flag( renderer, where, tile, options );
    render_unit_flag( renderer, where, *options.flag );
  }

  if( damaged )
    // Reuse the red X from boycotted commodities for the damaged
    // icon (the OG seems to do this).
    render_sprite( renderer, where + Delta{ .w = 8, .h = 8 },
                   e_tile::boycott );
}

void depixelate_from_to( rr::Renderer& renderer, double stage,
                         gfx::point                 hash_anchor,
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
gfx::pixel missionary_cross_color( e_nation nation,
                                   bool     is_jesuit ) {
  static auto dulled = [] {
    refl::enum_map<e_nation, gfx::pixel> m;
    for( e_nation nation : refl::enum_values<e_nation> ) {
      gfx::pixel const bright_color =
          config_nation.nations[nation].flag_color;
      gfx::pixel_hsl hsl = to_HSL( bright_color );
      hsl.s *= config_missionary
                   .saturation_reduction_for_non_jesuit_cross;
      m[nation] = to_RGB( hsl );
    }
    return m;
  }();
  if( is_jesuit )
    return config_nation.nations[nation].flag_color;
  return dulled[nation];
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
void render_unit( rr::Renderer& renderer, Coord where,
                  Unit const&              unit,
                  UnitRenderOptions const& options ) {
  render_unit_with_tile(
      renderer, where, unit.desc().tile,
      unit.orders().holds<unit_orders::damaged>(), options );
}

void render_native_unit( rr::Renderer& renderer, Coord where,
                         NativeUnit const&        native_unit,
                         UnitRenderOptions const& options ) {
  render_unit_with_tile( renderer, where,
                         unit_attr( native_unit.type ).tile,
                         /*damaged=*/false, options );
}

void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_unit_type              unit_type,
                       UnitRenderOptions const& options ) {
  render_unit_with_tile( renderer, where,
                         unit_attr( unit_type ).tile,
                         /*damaged=*/false, options );
}

void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_native_unit_type       unit_type,
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
void render_colony( rr::Renderer& renderer, Coord where,
                    SSConst const& ss, Colony const& colony,
                    ColonyRenderOptions const& options ) {
  e_tile const tile = [&]() {
    maybe<e_colony_barricade_type> barricade_type =
        barricade_for_colony( colony );
    if( !barricade_type.has_value() )
      return e_tile::colony_basic;
    switch( *barricade_type ) {
      case e_colony_barricade_type::stockade:
        return e_tile::colony_stockade;
      case e_colony_barricade_type::fort:
        // FIXME
        return e_tile::colony_stockade;
      case e_colony_barricade_type::fortress:
        // FIXME
        return e_tile::colony_stockade;
    }
  }();
  FrozenColony const frozen_colony =
      colony_to_frozen_colony( ss, colony );
  int const   population = colony_population( colony );
  rr::Painter painter    = renderer.painter();
  render_sprite( renderer, where, tile );
  auto const& nation = nation_obj( colony.nation );
  if( options.render_flag )
    render_colony_flag( painter, where + Delta{ .w = 8, .h = 8 },
                        nation.flag_color );
  if( options.render_population ) {
    Coord const population_coord =
        where + Delta{ .w = 44 / 2 - 3, .h = 44 / 2 - 4 };
    gfx::pixel const color =
        ( frozen_colony.sons_of_liberty_integral_percent == 100 )
            ? gfx::pixel{ .r = 0, .g = 255, .b = 255, .a = 255 }
        : ( frozen_colony.sons_of_liberty_integral_percent >=
            50 )
            ? gfx::pixel{ .r = 0, .g = 255, .b = 0, .a = 255 }
            : gfx::pixel::white();
    render_text_markup( renderer, population_coord, e_font{},
                        TextMarkupInfo{ .normal = color },
                        fmt::to_string( population ) );
  }
  if( options.render_name ) {
    Coord const name_coord =
        where + config_land_view.colony_name_offset;
    render_text_markup(
        renderer, name_coord, config_land_view.colony_name_font,
        TextMarkupInfo{ .highlight = gfx::pixel::white(),
                        .shadow    = gfx::pixel::black() },
        fmt::format( "[{}]", colony.name ) );
  }
}

/****************************************************************
** Dwelling Rendering.
*****************************************************************/
void render_dwelling( rr::Renderer& renderer, Coord where,
                      SSConst const&  ss,
                      Dwelling const& dwelling ) {
  rr::Painter          painter = renderer.painter();
  FrozenDwelling const frozen_dwelling =
      dwelling_to_frozen_dwelling( ss, dwelling );
  e_tribe const tribe_type = frozen_dwelling.tribe;
  auto&         tribe_conf = config_natives.tribes[tribe_type];
  e_tile const  dwelling_tile = tribe_conf.dwelling_tile;
  render_sprite( renderer, where, dwelling_tile );
  // Flags.
  e_native_level const native_level = tribe_conf.level;
  gfx::pixel const     flag_color   = tribe_conf.flag_color;
  for( gfx::rect flag : config_natives.flag_rects[native_level] )
    painter.draw_solid_rect( flag.origin_becomes_point( where ),
                             flag_color );
  // The offset that we need to go to get to the upper left
  // corner of a 32x32 sprite centered on the (48x48) dwelling
  // tile.
  Delta const offset_32x32{ .w = 6, .h = 6 };
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
        frozen_mission->nation, is_jesuit );
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

/****************************************************************
** Misc. Rendering.
*****************************************************************/
void render_shadow_hightlight_border(
    rr::Renderer& renderer, gfx::rect rect,
    gfx::pixel left_and_bottom, gfx::pixel top_and_right ) {
  rr::Painter painter = renderer.painter();
  painter.draw_horizontal_line( rect.nw().moved_up(),
                                rect.size.w + 1, top_and_right );
  painter.draw_horizontal_line( rect.sw(), rect.size.w,
                                left_and_bottom );
  painter.draw_vertical_line( rect.nw().moved_left(),
                              rect.size.h + 1, left_and_bottom );
  painter.draw_vertical_line( rect.ne(), rect.size.h,
                              top_and_right );
  gfx::pixel const mixed = mix( left_and_bottom, top_and_right );
  painter.draw_point( rect.nw().moved_up().moved_left(), mixed );
  painter.draw_point( rect.se(), mixed );
}

} // namespace rn
