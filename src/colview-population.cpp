/****************************************************************
**colview-population.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-08-05.
*
* Description: Population view UI within the colony view.
*
*****************************************************************/
#include "colview-population.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "render.hpp"
#include "sons-of-liberty.hpp"
#include "tiles.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** PopulationView
*****************************************************************/
void PopulationView::draw_sons_of_liberty(
    rr::Renderer& renderer, Coord coord ) const {
  rr::Painter painter          = renderer.painter();
  Coord       pos              = coord;
  int constexpr kVerticalStart = 3;
  pos.y += kVerticalStart;
  // This is the number of pixels of padding to add on each side
  // of the flag and crown icons.
  int constexpr kIconPadding = 4;
  pos.x += kIconPadding;
  render_sprite( painter, pos, e_tile::rebel_flag );
  pos.x += sprite_size( e_tile::rebel_flag ).w + kIconPadding;
  ColonySonsOfLiberty const info =
      compute_colony_sons_of_liberty( player_, colony_ );
  gfx::pixel text_color         = gfx::pixel::white();
  int const  tory_penalty_level = compute_tory_penalty_level(
       ss_.settings.difficulty, info.tories );
  switch( tory_penalty_level ) {
    case 0: {
      text_color = gfx::pixel::white();
      break;
    }
    case 1: {
      text_color = gfx::pixel::red();
      break;
    }
    case 2: {
      static auto c = gfx::pixel::red().highlighted( 2 );
      text_color    = c;
      break;
    }
    case 3: {
      static auto c = gfx::pixel::red().highlighted( 4 );
      text_color    = c;
      break;
    }
    default: {
      static auto c = gfx::pixel::red().highlighted( 4 );
      text_color    = c;
      break;
    }
  }
  int const kTextVerticalOffset = 4;
  {
    rr::Typer typer = renderer.typer(
        pos + gfx::size{ .h = kTextVerticalOffset },
        text_color );
    typer.write( fmt::format(
        "{}% ({})", info.sol_integral_percent, info.rebels ) );
  }
  string const tories_str = fmt::format(
      "{}% ({})", info.tory_integral_percent, info.tories );
  gfx::size const tories_text_width =
      rr::rendered_text_line_size_pixels( tories_str );
  pos.x = rect( coord ).right_edge() - kIconPadding -
          sprite_size( e_tile::crown ).w - kIconPadding -
          tories_text_width.w;
  rr::Typer typer = renderer.typer(
      pos + gfx::size{ .h = kTextVerticalOffset }, text_color );
  typer.write( tories_str );
  pos.x = typer.position().x;
  pos.x += kIconPadding;
  render_sprite( painter, pos, e_tile::crown );
}

void PopulationView::draw( rr::Renderer& renderer,
                           Coord         coord ) const {
  draw_sons_of_liberty( renderer, coord );
  // Draw colonists.
  rr::Painter painter = renderer.painter();
  painter.draw_empty_rect( rect( coord ).with_inc_size(),
                           rr::Painter::e_border_mode::inside,
                           gfx::pixel::black() );
  vector<UnitId> units    = colony_units_all( colony_ );
  auto           unit_pos = coord + Delta{ .h = 16 };
  unit_pos.x -= 3;
  for( UnitId unit_id : units ) {
    render_unit( renderer, unit_pos,
                 ss_.units.unit_for( unit_id ),
                 UnitRenderOptions{ .flag = false } );
    unit_pos.x += 15;
  }
}

} // namespace rn
