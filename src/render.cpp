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
#include "logger.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "ustate.hpp"
#include "views.hpp"

// config
#include "config/gfx.rds.hpp"
#include "config/nation.hpp"
#include "config/natives.hpp"
#include "config/tile-enum.rds.hpp"
#include "config/unit-type.hpp"

// ss
#include "ss/colony.hpp"
#include "ss/dwelling.rds.hpp"

// base
#include "base/keyval.hpp"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {

constexpr Delta nationality_icon_size{ .w = 14, .h = 14 };

// Unit only, no flag.
void render_unit_no_icon( rr::Painter& painter, Coord where,
                          e_tile                   tile,
                          UnitRenderOptions const& options ) {
  if( options.shadow.has_value() )
    render_sprite_silhouette(
        painter, where + Delta{ .w = options.shadow->offset },
        tile, options.shadow->color );
  if( options.outline_right )
    render_sprite_silhouette( painter, where + Delta{ .w = 1 },
                              tile, options.outline_color );
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
                       e_unit_orders orders,
                       e_flag_count  flag ) {
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
  switch( orders ) {
    case e_unit_orders::none: c = '-'; break;
    case e_unit_orders::sentry: c = 'S'; break;
    case e_unit_orders::fortified: c = 'F'; break;
    case e_unit_orders::fortifying: c = 'F'; break;
    case e_unit_orders::road: c = 'R'; break;
    case e_unit_orders::plow: c = 'P'; break;
  };
  // We don't grey out the "fortifying" state to signal to the
  // player that the unit is not yet fully fortified.
  bool is_greyed = ( orders == e_unit_orders::fortified ||
                     orders == e_unit_orders::sentry );
  switch( flag ) {
    case e_flag_count::none: break;
    case e_flag_count::single: break;
    case e_flag_count::multiple:
      render_unit_flag_impl( renderer, where + delta_stacked,
                             color, c, is_greyed );
      break;
  }
  render_unit_flag_impl( renderer, where, color, c, is_greyed );
}

void depixelate_from_to(
    rr::Renderer& renderer, double stage, gfx::point hash_anchor,
    base::function_ref<void( rr::Painter& painter )> from,
    base::function_ref<void( rr::Painter& painter )> to ) {
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                           hash_anchor );
  SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.stage,
                           stage );
  // The ordering of these should actually not matter, because we
  // won't be rendering any pixels that contain both.
  {
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.inverted,
                             true );
    rr::Painter painter = renderer.painter();
    to( painter );
  }
  rr::Painter painter = renderer.painter();
  from( painter );
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
                       e_unit_orders orders ) {
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
                    flag_color, e_unit_orders::none, flag );
}

static void render_unit_impl(
    rr::Renderer& renderer, Coord where, e_tile tile,
    auto const& desc, gfx::pixel flag_color,
    e_unit_orders orders, UnitRenderOptions const& options ) {
  rr::Painter painter = renderer.painter();
  if( options.flag == e_flag_count::none ) {
    render_unit_no_icon( painter, where, tile, options );
    return;
  }

  // Should the icon be in front of the unit or in back.
  if( !desc.nat_icon_front ) {
    // This is a bit tricky if there's a shadow because we
    // don't want the shadow to be over the flag.
    if( options.shadow.has_value() )
      render_sprite_silhouette(
          painter, where + Delta{ .w = options.shadow->offset },
          desc.tile, options.shadow->color );
    render_unit_flag( renderer, where, desc, flag_color, orders,
                      options.flag );
    if( options.shadow.has_value() ) {
      // Draw a light shadow over the flag so that we can dif-
      // ferentiate the edge of the unit from the flag, but not
      // so dark that it will cover up the flag.
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .35 );
      rr::Painter painter = renderer.painter();
      render_sprite_silhouette(
          painter, where + Delta{ .w = options.shadow->offset },
          desc.tile, options.shadow->color );
    }
    render_sprite( painter, where, desc.tile );
  } else {
    render_unit_no_icon( painter, where, tile, options );
    render_unit_flag( renderer, where, desc, flag_color, orders,
                      options.flag );
  }
}

void render_unit( rr::Renderer& renderer, Coord where,
                  Unit const&              unit,
                  UnitRenderOptions const& options ) {
  render_unit_impl( renderer, where, unit.desc().tile,
                    unit.desc(),
                    nation_obj( unit.nation() ).flag_color,
                    unit.orders(), options );
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
                    e_unit_orders::none, options );
}

static void render_unit_type(
    rr::Painter& painter, Coord where, e_tile tile,
    UnitRenderOptions const& options ) {
  render_unit_no_icon( painter, where, tile, options );
}

void render_unit_type( rr::Painter& painter, Coord where,
                       e_unit_type              unit_type,
                       UnitRenderOptions const& options ) {
  render_unit_no_icon( painter, where,
                       unit_attr( unit_type ).tile, options );
}

void render_native_unit_type(
    rr::Painter& painter, Coord where,
    e_native_unit_type       unit_type,
    UnitRenderOptions const& options ) {
  render_unit_no_icon( painter, where,
                       unit_attr( unit_type ).tile, options );
}

void render_colony( rr::Painter& painter, Coord where,
                    Colony const& colony ) {
  auto tile = colony.buildings[e_colony_building::stockade]
                  ? e_tile::colony_stockade
                  : e_tile::colony_basic;
  render_sprite( painter, where, tile );
  auto const& nation = nation_obj( colony.nation );
  render_colony_flag( painter, where + Delta{ .w = 8, .h = 8 },
                      nation.flag_color );
}

void render_dwelling( rr::Painter& painter, Coord where,
                      Dwelling const& dwelling ) {
  auto& tribe_conf = config_natives.tribes[dwelling.tribe];
  e_tile const dwelling_tile = tribe_conf.dwelling_tile;
  render_sprite( painter, where, dwelling_tile );
  // Flags.
  e_native_level const native_level = tribe_conf.level;
  gfx::pixel const     flag_color   = tribe_conf.flag_color;
  for( gfx::rect flag : config_natives.flag_rects[native_level] )
    painter.draw_solid_rect( flag.origin_becomes_point( where ),
                             flag_color );
  // Yellow star to mark the capital.
  if( dwelling.is_capital )
    render_sprite( painter, where + Delta{ .w = 6, .h = 6 },
                   e_tile::capital_star );
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
        [&]( rr::Painter& painter ) {
          render_sprite_silhouette(
              painter,
              where + Delta{ .w = options.shadow->offset },
              desc.tile, options.shadow->color );
        },
        /*to=*/
        [&]( rr::Painter& painter ) {
          render_sprite_silhouette(
              painter,
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
      [&]( rr::Painter& painter ) {
        render_unit_type( painter, where, desc.tile, options );
      },
      /*to=*/
      [&]( rr::Painter& painter ) {
        render_unit_type( painter, where, target_tile, options );
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

} // namespace rn
