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
#include "cstate.hpp"
#include "error.hpp"
#include "logger.hpp"
#include "plane.hpp"
#include "screen.hpp"
#include "text.hpp"
#include "ustate.hpp"
#include "views.hpp"
#include "window.hpp"

// base
#include "base/keyval.hpp"

// C++ standard library
#include <vector>

using namespace std;

namespace rn {

namespace {

constexpr Delta nationality_icon_size( 13_h, 13_w );

// Unit only, no nationality icon.
void render_unit_no_icon( rr::Painter& painter, Coord where,
                          e_unit_type unit_type ) {
  auto const& desc = unit_attr( unit_type );
  render_sprite( painter, Rect::from( where, g_tile_delta ),
                 desc.tile );
}

void render_colony_flag( rr::Painter& painter, Coord coord,
                         gfx::pixel color ) {
  auto cloth_rect = Rect::from( coord, Delta{ 8_w, 6_h } );
  painter.draw_solid_rect( cloth_rect, color );
  painter.draw_vertical_line( cloth_rect.upper_right(), 12,
                              gfx::pixel::wood().shaded( 4 ) );
}

/****************************************************************
** Rendering Building Blocks
*****************************************************************/
void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, e_nation nation,
                              char c ) {
  Delta       delta    = nationality_icon_size;
  Rect        rect     = Rect::from( where, delta );
  auto const& nation_o = nation_obj( nation );

  auto color      = nation_o.flag_color;
  auto dark       = color.shaded( 4 );
  auto text_color = color.shaded( 7 );

  rr::Painter painter = renderer.painter();

  painter.draw_solid_rect( rect, color );
  painter.draw_empty_rect(
      rect, rr::Painter::e_border_mode::outside, dark );

  Delta char_size = Delta::from_gfx(
      rr::rendered_text_line_size_pixels( string( 1, c ) ) );
  render_text( renderer,
               centered( char_size, rect ) + Delta{ 1_w, 0_h },
               font::nat_icon(), text_color, string( 1, c ) );
}

void render_nationality_icon( rr::Renderer&             renderer,
                              Coord                     where,
                              UnitTypeAttributes const& desc,
                              e_nation                  nation,
                              e_unit_orders orders ) {
  // Now we will advance the pixel_coord to put the icon at the
  // location specified in the unit descriptor.
  auto  position = desc.nat_icon_position;
  Delta delta{};
  switch( position ) {
    case e_direction::nw: break;
    case e_direction::ne:
      delta +=
          ( ( 1_w * g_tile_width ) - nationality_icon_size.w );
      break;
    case e_direction::se:
      delta += ( ( Delta{ 1_w, 1_h } * g_tile_scale ) -
                 nationality_icon_size );
      break;
    case e_direction::sw:
      delta +=
          ( ( 1_h * g_tile_height ) - nationality_icon_size.h );
      break;
      // By default we keep it in the northwest corner.
    default: break;
  };
  where += delta;

  char c{ '-' }; // gcc seems to want us to initialize this
  switch( orders ) {
    case e_unit_orders::none: c = '-'; break;
    case e_unit_orders::sentry: c = 'S'; break;
    case e_unit_orders::fortified: c = 'F'; break;
  };
  render_nationality_icon( renderer, where, nation, c );
}

} // namespace

void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, e_unit_type type,
                              e_nation      nation,
                              e_unit_orders orders ) {
  render_nationality_icon( renderer, where, unit_attr( type ),
                           nation, orders );
}

void render_nationality_icon( rr::Renderer& renderer,
                              Coord where, UnitId id ) {
  auto const& unit = unit_from_id( id );
  render_nationality_icon( renderer, where, unit.desc(),
                           unit.nation(), unit.orders() );
}

void render_unit( rr::Renderer& renderer, Coord where, UnitId id,
                  bool with_icon ) {
  rr::Painter painter = renderer.painter();
  auto const& unit    = unit_from_id( id );
  if( with_icon ) {
    // Should the icon be in front of the unit or in back.
    if( !unit.desc().nat_icon_front ) {
      render_nationality_icon( renderer, where, id );
      render_unit_no_icon( painter, where, unit.desc().type );
    } else {
      render_unit_no_icon( painter, where, unit.desc().type );
      render_nationality_icon( renderer, where, id );
    }
  } else {
    render_unit_no_icon( painter, where, unit.desc().type );
  }
}

void render_unit( rr::Painter& painter, Coord where,
                  e_unit_type unit_type ) {
  render_unit_no_icon( painter, where, unit_type );
}

void render_colony( rr::Painter& painter, Coord where,
                    ColonyId id ) {
  auto const& colony = colony_from_id( id );
  auto        tile   = colony_from_id( id ).buildings().contains(
                           e_colony_building::stockade )
                           ? e_tile::colony_stockade
                           : e_tile::colony_basic;
  render_sprite( painter, Rect::from( where, g_tile_delta ),
                 tile );
  auto const& nation = nation_obj( colony.nation() );
  render_colony_flag( painter, where + Delta{ 8_w, 8_h },
                      nation.flag_color );
}

} // namespace rn
