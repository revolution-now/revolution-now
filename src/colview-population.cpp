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
#include "production.rds.hpp"
#include "render.hpp"
#include "sons-of-liberty.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/settings.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

TileSpreadRenderPlans create_production_spreads(
    ColonyProduction const& production, int const width ) {
  int const food_deficit_without_stores =
      std::max( production.food_horses
                        .food_consumed_by_colonists_theoretical -
                    production.food_horses.food_produced,
                0 );
  TileSpreadConfigMulti const config{
    .tiles{
      { .tile  = e_tile::product_fish_20,
        .count = production.food_horses.fish_produced },
      { .tile  = e_tile::commodity_food_20,
        .count = production.food_horses.corn_produced },
      { .tile  = e_tile::commodity_food_20,
        .count = food_deficit_without_stores,
        .has_x = e_red_x_size::large },
      { .tile  = e_tile::product_crosses,
        .count = production.crosses },
      { .tile  = e_tile::product_bells,
        .count = production.bells },
    },
    .options =
        {
          .bounds = width,
          .label_policy = SpreadLabels::auto_decide{},
          .label_opts   = {},
        },
    .group_spacing = 4,
  };
  return build_tile_spread_multi( config );
}

} // namespace

/****************************************************************
** PopulationView
*****************************************************************/
void PopulationView::draw_production_spreads(
    rr::Renderer& renderer, Coord coord ) const {
  point const origin =
      layout_.spread_origin.origin_becomes_point( coord );
  draw_rendered_icon_spread( renderer, origin,
                             layout_.production_spreads );
}

void PopulationView::draw_sons_of_liberty(
    rr::Renderer& renderer, Coord coord ) const {
  Coord pos                    = coord;
  int constexpr kVerticalStart = 3;
  pos.y += kVerticalStart;
  // This is the number of pixels of padding to add on each side
  // of the flag and crown icons.
  int constexpr kIconPadding = 4;
  ColonySonsOfLiberty const info =
      compute_colony_sons_of_liberty( player_, colony_ );
  gfx::pixel text_color        = gfx::pixel::white();
  int const tory_penalty_level = compute_tory_penalty_level(
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
  if( info.sol_integral_percent > 0 ) {
    pos.x += kIconPadding;
    render_sprite( renderer, pos, e_tile::rebel_flag );
    pos.x += sprite_size( e_tile::rebel_flag ).w + kIconPadding;
    rr::Typer typer = renderer.typer(
        pos.to_gfx() + gfx::size{ .h = kTextVerticalOffset },
        text_color );
    typer.write( fmt::format(
        "{}% ({})", info.sol_integral_percent, info.rebels ) );
  }
  if( info.tory_integral_percent == 0 )
    // The original game does not draw the crown when there are
    // no tories.
    return;
  string const tories_str = fmt::format(
      "{}% ({})", info.tory_integral_percent, info.tories );
  gfx::size const tories_text_width =
      rr::rendered_text_line_size_pixels( tories_str );
  pos.x = bounds( coord ).right_edge() - kIconPadding -
          sprite_size( e_tile::crown ).w - kIconPadding -
          tories_text_width.w;
  rr::Typer typer = renderer.typer(
      pos.to_gfx() + gfx::size{ .h = kTextVerticalOffset },
      text_color );
  typer.write( tories_str );
  pos.x = typer.position().x;
  pos.x += kIconPadding;
  render_sprite( renderer, pos, e_tile::crown );
}

void PopulationView::draw( rr::Renderer& renderer,
                           Coord coord ) const {
  rr::Painter painter = renderer.painter();
  painter.draw_empty_rect( bounds( coord ).with_inc_size(),
                           rr::Painter::e_border_mode::inside,
                           gfx::pixel::black() );

  // SoL.
  draw_sons_of_liberty( renderer, coord );

  // Colonists.
  vector<UnitId> units = colony_units_all( colony_ );
  auto unit_pos        = coord + Delta{ .h = 16 };
  unit_pos.x -= 3;
  for( UnitId unit_id : units ) {
    render_unit( renderer, unit_pos,
                 ss_.units.unit_for( unit_id ),
                 UnitRenderOptions{} );
    unit_pos.x += 15;
  }

  // Production Spreads.
  draw_production_spreads( renderer, coord );
}

PopulationView::Layout PopulationView::create_layout(
    size const sz ) {
  Layout l;
  l.size          = sz;
  l.spread_margin = 2;
  l.spread_origin = { .x = l.spread_margin, .y = 16 + 32 };
  int const spread_width = sz.w - 2 * l.spread_margin;
  l.production_spreads   = create_production_spreads(
      colview_production(), spread_width );
  return l;
}

void PopulationView::update_this_and_children() {
  // This method is only called when the logical resolution
  // hasn't changed, so we assume the size hasn't changed.
  layout_ = create_layout( layout_.size );
}

std::unique_ptr<PopulationView> PopulationView::create(
    SS& ss, TS& ts, Player& player, Colony& colony,
    Delta size ) {
  Layout layout = create_layout( size );
  return std::make_unique<PopulationView>(
      ss, ts, player, colony, std::move( layout ) );
}

} // namespace rn
