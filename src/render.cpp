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
#include "compositor.hpp"
#include "error.hpp"
#include "fog-conv.hpp"
#include "logger.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "unit-classes.hpp"
#include "unit-mgr.hpp"
#include "views.hpp"

// config
#include "config/gfx.rds.hpp"
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

// base
#include "base/keyval.hpp"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {

constexpr Delta nationality_icon_size{ .w = 14, .h = 14 };

// Unit only, no flag.
void render_unit_no_icon( rr::Renderer& renderer, Coord where,
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

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
void render_unit_flag_impl( rr::Renderer& renderer, Coord where,
                            gfx::pixel color, char c,
                            bool is_greyed ) {
  Delta delta = nationality_icon_size;
  Rect  rect  = Rect::from( where, delta );

  auto        dark       = color.shaded( 2 );
  auto        text_color = is_greyed
                               ? config_gfx.unit_flag_text_color_greyed
                               : config_gfx.unit_flag_text_color;
  rr::Painter painter    = renderer.painter();

  painter.draw_solid_rect( rect, color );
  painter.draw_empty_rect(
      rect, rr::Painter::e_border_mode::inside, dark );

  Delta char_size = Delta::from_gfx(
      rr::rendered_text_line_size_pixels( string( 1, c ) ) );
  render_text( renderer, centered( char_size, rect ),
               font::nat_icon(), text_color, string( 1, c ) );
}

void render_unit_flag( rr::Renderer& renderer, Coord where,
                       auto const& desc, gfx::pixel color,
                       unit_orders const& orders,
                       e_flag_count       flag ) {
  // Now we will advance the pixel_coord to put the icon at the
  // location specified in the unit descriptor.
  auto  position = desc.nat_icon_position;
  Delta delta{};
  // If we're going to draw a stacked flag (i.e., one behind the
  // front flag to indicated stacked units) then this will be its
  // offset from the front flag.
  Delta            delta_stacked;
  static int const S = 2;
  switch( position ) {
    case e_direction::nw:
      delta_stacked = { .w = S, .h = S };
      break;
    case e_direction::ne:
      delta.w +=
          ( ( 1 * g_tile_width ) - nationality_icon_size.w );
      delta_stacked = { .w = -S, .h = S };
      break;
    case e_direction::se:
      delta += ( ( Delta{ .w = 1, .h = 1 } * g_tile_delta ) -
                 nationality_icon_size );
      delta_stacked = { .w = -S, .h = -S };
      break;
    case e_direction::sw:
      delta.h +=
          ( ( 1 * g_tile_height ) - nationality_icon_size.h );
      delta_stacked = { .w = S, .h = -S };
      break;
    case e_direction::w:
      delta.h += ( g_tile_height - nationality_icon_size.h ) / 2;
      delta_stacked = { .w = S, .h = -S };
      break;
    case e_direction::n:
      delta.w += ( g_tile_width - nationality_icon_size.w ) / 2;
      delta_stacked = { .w = 0, .h = S };
      break;
    case e_direction::e:
      delta.h += ( g_tile_height - nationality_icon_size.h ) / 2;
      delta.w +=
          ( ( 1 * g_tile_width ) - nationality_icon_size.w );
      delta_stacked = { .w = -S, .h = -S };
      break;
    case e_direction::s:
      delta.w += ( g_tile_width - nationality_icon_size.w ) / 2;
      delta.h +=
          ( ( 1 * g_tile_height ) - nationality_icon_size.h );
      delta_stacked = { .w = 0, .h = -S };
      break;
  };
  where += delta;

  char c{ '-' }; // gcc seems to want us to initialize this
  switch( orders.to_enum() ) {
    using e = unit_orders::e;
    case e::none:
      c = '-';
      break;
    case e::sentry:
      c = 'S';
      break;
    case e::fortified:
      c = 'F';
      break;
    case e::fortifying:
      c = 'F';
      break;
    case e::road:
      c = 'R';
      break;
    case e::plow:
      c = 'P';
      break;
    case e::damaged: {
      auto&     o          = orders.get<unit_orders::damaged>();
      int const turns_left = o.turns_until_repair;
      // The number can be larger than 9, i.e. it can have more
      // than one digit which we cannot display on the flag. So
      // we will do what the OG does and display a + sign in that
      // case. TODO: Maybe we should find a better way to commu-
      // nicate that number (tool tips?).
      if( turns_left > 9 )
        c = '+';
      else
        c = '0' + turns_left;
      break;
    }
  };
  // We don't grey out the "fortifying" state to signal to the
  // player that the unit is not yet fully fortified.
  bool is_greyed = ( orders.holds<unit_orders::fortified>() ||
                     orders.holds<unit_orders::sentry>() );
  switch( flag ) {
    case e_flag_count::none:
      break;
    case e_flag_count::single:
      break;
    case e_flag_count::multiple:
      render_unit_flag_impl( renderer, where + delta_stacked,
                             color, c, is_greyed );
      break;
  }
  render_unit_flag_impl( renderer, where, color, c, is_greyed );
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
** Public API
*****************************************************************/
void render_unit_flag( rr::Renderer& renderer, Coord where,
                       e_unit_type type, e_nation nation,
                       unit_orders const& orders ) {
  render_unit_flag( renderer, where, unit_attr( type ),
                    nation_obj( nation ).flag_color, orders,
                    e_flag_count::single );
}

static void render_unit_flag( rr::Renderer& renderer,
                              Coord        where, SSConst const&,
                              Unit const&  unit,
                              e_flag_count flag ) {
  render_unit_flag( renderer, where, unit.desc(),
                    nation_obj( unit.nation() ).flag_color,
                    unit.orders(), flag );
}

static void render_unit_flag( rr::Renderer& renderer,
                              Coord where, SSConst const& ss,
                              NativeUnit const& unit,
                              e_flag_count      flag ) {
  e_tribe const    tribe = tribe_for_unit( ss, unit );
  gfx::pixel const flag_color =
      config_natives.tribes[tribe].flag_color;
  render_unit_flag( renderer, where, unit_attr( unit.type ),
                    flag_color, unit_orders::none{}, flag );
}

static void render_unit_impl(
    rr::Renderer& renderer, Coord where, e_tile tile,
    auto const& desc, gfx::pixel flag_color,
    unit_orders const& orders, bool damaged,
    UnitRenderOptions const& options ) {
  rr::Painter painter = renderer.painter();
  if( options.flag == e_flag_count::none ) {
    // No flag.
    render_unit_no_icon( renderer, where, tile, options );
  } else if( !desc.nat_icon_front ) {
    // Show the flag but in the back. This is a bit tricky if
    // there's a shadow because we don't want the shadow to be
    // over the flag.
    if( options.shadow.has_value() )
      render_sprite_silhouette(
          renderer, where + Delta{ .w = options.shadow->offset },
          desc.tile, options.shadow->color );
    render_unit_flag( renderer, where, desc, flag_color, orders,
                      options.flag );
    if( options.shadow.has_value() ) {
      // Draw a light shadow over the flag so that we can dif-
      // ferentiate the edge of the unit from the flag, but not
      // so dark that it will cover up the flag.
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .35 );
      render_sprite_silhouette(
          renderer, where + Delta{ .w = options.shadow->offset },
          desc.tile, options.shadow->color );
    }
    render_sprite( painter, where, desc.tile );
  } else {
    // Show the flag in the front.
    render_unit_no_icon( renderer, where, tile, options );
    render_unit_flag( renderer, where, desc, flag_color, orders,
                      options.flag );
  }

  if( damaged )
    // Reuse the red X from boycotted commodities for the damaged
    // icon (the OG seems to do this).
    render_sprite( painter, where + Delta{ .w = 8, .h = 8 },
                   e_tile::boycott );
}

void render_unit( rr::Renderer& renderer, Coord where,
                  Unit const&              unit,
                  UnitRenderOptions const& options ) {
  render_unit_impl(
      renderer, where, unit.desc().tile, unit.desc(),
      nation_obj( unit.nation() ).flag_color, unit.orders(),
      unit.orders().holds<unit_orders::damaged>(), options );
}

void render_native_unit( rr::Renderer& renderer, Coord where,
                         SSConst const&           ss,
                         NativeUnit const&        native_unit,
                         UnitRenderOptions const& options ) {
  auto const&      desc  = unit_attr( native_unit.type );
  e_tribe const    tribe = tribe_for_unit( ss, native_unit );
  gfx::pixel const flag_color =
      config_natives.tribes[tribe].flag_color;
  render_unit_impl( renderer, where, desc.tile, desc, flag_color,
                    unit_orders::none{}, /*damaged=*/false,
                    options );
}

static void render_unit_type(
    rr::Renderer& renderer, Coord where, e_tile tile,
    UnitRenderOptions const& options ) {
  render_unit_no_icon( renderer, where, tile, options );
}

void render_unit_type( rr::Renderer& renderer, Coord where,
                       e_unit_type              unit_type,
                       UnitRenderOptions const& options ) {
  render_unit_no_icon( renderer, where,
                       unit_attr( unit_type ).tile, options );
}

void render_native_unit_type(
    rr::Renderer& renderer, Coord where,
    e_native_unit_type       unit_type,
    UnitRenderOptions const& options ) {
  render_unit_no_icon( renderer, where,
                       unit_attr( unit_type ).tile, options );
}

void render_fog_colony( rr::Renderer& renderer, Coord where,
                        FogColony const&           fog_colony,
                        ColonyRenderOptions const& options ) {
  e_tile const tile = [&]() {
    if( !fog_colony.barricade_type.has_value() )
      return e_tile::colony_basic;
    switch( *fog_colony.barricade_type ) {
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
  rr::Painter painter = renderer.painter();
  render_sprite( painter, where, tile );
  auto const& nation = nation_obj( fog_colony.nation );
  if( options.render_flag )
    render_colony_flag( painter, where + Delta{ .w = 8, .h = 8 },
                        nation.flag_color );
  if( options.render_population ) {
    Coord const population_coord =
        where + Delta{ .w = 44 / 2 - 3, .h = 44 / 2 - 4 };
    gfx::pixel const color =
        ( fog_colony.sons_of_liberty_integral_percent == 100 )
            ? gfx::pixel{ .r = 0, .g = 255, .b = 255, .a = 255 }
        : ( fog_colony.sons_of_liberty_integral_percent >= 50 )
            ? gfx::pixel{ .r = 0, .g = 255, .b = 0, .a = 255 }
            : gfx::pixel::white();
    render_text_markup(
        renderer, population_coord, e_font{},
        TextMarkupInfo{ .normal = color },
        fmt::to_string( fog_colony.population ) );
  }
  if( options.render_name ) {
    Coord const name_coord =
        where + config_land_view.colony_name_offset;
    render_text_markup(
        renderer, name_coord, config_land_view.colony_name_font,
        TextMarkupInfo{ .highlight = gfx::pixel::white(),
                        .shadow    = gfx::pixel::black() },
        fmt::format( "[{}]", fog_colony.name ) );
  }
}

void render_real_colony( rr::Renderer& renderer, Coord where,
                         SSConst const& ss, Colony const& colony,
                         ColonyRenderOptions const& options ) {
  render_fog_colony( renderer, where,
                     colony_to_fog_colony( ss, colony ),
                     options );
}

void render_fog_dwelling( rr::Renderer& renderer, Coord where,
                          FogDwelling const& fog_dwelling ) {
  rr::Painter   painter    = renderer.painter();
  e_tribe const tribe_type = fog_dwelling.tribe;
  auto&         tribe_conf = config_natives.tribes[tribe_type];
  e_tile const  dwelling_tile = tribe_conf.dwelling_tile;
  render_sprite( painter, where, dwelling_tile );
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
  if( fog_dwelling.capital )
    render_sprite( painter, where + offset_32x32,
                   e_tile::capital_star );

  // If there is a missionary in this dwelling then render a
  // cross on top of it with the color of the nation's flag. It
  // will be dulled if the missionary is a non-jesuit.
  maybe<FogMission> const mission = fog_dwelling.mission;
  if( mission.has_value() ) {
    bool const is_jesuit =
        ( mission->level == e_missionary_type::jesuit );
    gfx::pixel const cross_color =
        missionary_cross_color( mission->nation, is_jesuit );
    // This is the inner part that is colored according to the
    // nation's flag color.
    render_sprite_silhouette( renderer, where + offset_32x32,
                              e_tile::missionary_cross_inner,
                              cross_color );
    render_sprite( painter, where + offset_32x32,
                   e_tile::missionary_cross_outter );
    // This part adds some shadows/highlights to the colored part
    // of the cross.
    {
      double const alpha = is_jesuit ? .10 : .07;
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, alpha );
      rr::Painter painter = renderer.painter();
      render_sprite( painter, where + offset_32x32,
                     e_tile::missionary_cross_accent );
    }
  }
}

void render_real_dwelling( rr::Renderer& renderer, Coord where,
                           SSConst const&  ss,
                           Dwelling const& dwelling ) {
  render_fog_dwelling(
      renderer, where,
      dwelling_to_fog_dwelling( ss, dwelling.id ) );
}

void render_unit_depixelate( rr::Renderer& renderer, Coord where,
                             Unit const& unit, double stage,
                             UnitRenderOptions const& options ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  render_unit( renderer, where, unit, options );
}

// This is a bit tricky because we need to render the shadow, the
// flag, and the unit, but 1) the flag has to go between the unit
// and the shadow if the flag is to be behind the unit, and 2) we
// don't want to depixelate the flag since we have a target unit,
// so it would look strange if the flag depixelated.
static void render_unit_depixelate_to_impl(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    auto const& desc, auto const& unit, e_tile target_tile,
    double stage, UnitRenderOptions options ) {
  // The shadow always goes in back of the flag, so if there is
  // one we can get that out of the way.
  if( options.shadow.has_value() )
    depixelate_from_to(
        renderer, stage, /*anchor=*/where, /*from=*/
        [&] {
          render_sprite_silhouette(
              renderer,
              where + Delta{ .w = options.shadow->offset },
              desc.tile, options.shadow->color );
        },
        /*to=*/
        [&] {
          render_sprite_silhouette(
              renderer,
              where + Delta{ .w = options.shadow->offset },
              target_tile, options.shadow->color );
        } );
  options.shadow.reset();

  // If the flag is on then it goes in between the shadow and
  // unit.
  if( options.flag != e_flag_count::none &&
      !desc.nat_icon_front )
    render_unit_flag( renderer, where, ss, unit, options.flag );

  // Now the unit.
  depixelate_from_to(
      renderer, stage, /*anchor=*/where, /*from=*/
      [&] {
        render_unit_type( renderer, where, desc.tile, options );
      },
      /*to=*/
      [&] {
        render_unit_type( renderer, where, target_tile,
                          options );
      } );

  if( options.flag != e_flag_count::none && desc.nat_icon_front )
    render_unit_flag( renderer, where, ss, unit, options.flag );
}

void render_unit_depixelate_to( rr::Renderer& renderer,
                                Coord where, SSConst const& ss,
                                Unit const& unit, e_tile target,
                                double            stage,
                                UnitRenderOptions options ) {
  render_unit_depixelate_to_impl( renderer, where, ss,
                                  unit.desc(), unit, target,
                                  stage, options );
}

void render_native_unit_depixelate(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    NativeUnit const& unit, double stage,
    UnitRenderOptions const& options ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  render_native_unit( renderer, where, ss, unit, options );
}

void render_native_unit_depixelate_to(
    rr::Renderer& renderer, Coord where, SSConst const& ss,
    NativeUnit const& unit, e_tile target, double stage,
    UnitRenderOptions options ) {
  render_unit_depixelate_to_impl( renderer, where, ss,
                                  unit_attr( unit.type ), unit,
                                  target, stage, options );
}

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
