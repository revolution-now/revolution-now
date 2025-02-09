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
    SSConst const& ss, ColonyProduction const& production,
    int const width ) {
  int const deficit =
      production.food_horses.food_deficit_without_stores;
  CHECK_GE( deficit, 0 );
  int const total_shown =
      ( deficit > 0 )
          ? production.food_horses
                .food_consumed_by_colonists_theoretical
          : production.food_horses.food_produced;
  int const total_produced =
      production.food_horses.food_produced;
  CHECK_GE( total_produced, 0 );
  int const available = total_shown - deficit;
  CHECK_GE( available, 0 );
  int const consumed =
      available -
      std::max( production.food_horses.food_delta_final, 0 );
  CHECK_GE( consumed, 0 );
  int const surplus = available - consumed;
  CHECK_GE( surplus, 0 );
  CHECK_EQ( consumed + surplus + deficit, total_shown );
  CHECK_EQ( consumed + surplus, total_produced );
  TileSpreadConfigMulti const config{
    .tiles{
      { .tile = e_tile::commodity_food_20, .count = consumed },
      { .tile = e_tile::commodity_food_20, .count = surplus },
      { .tile  = e_tile::commodity_food_20,
        .count = deficit,
        .has_x = e_red_x_size::small },
      { .tile  = e_tile::product_crosses_20,
        .count = production.crosses },
      { .tile  = e_tile::product_bells_20,
        .count = production.bells },
    },
    .options =
        {
          .bounds = width,
          .label_policy =
              ss.settings.colony_options.numbers
                  ? SpreadLabels{ SpreadLabels::always{} }
                  : SpreadLabels{ SpreadLabels::auto_decide{} },
          .label_opts = { .placement = SpreadLabelPlacement::
                              left_middle_adjusted{} },
        },
    .group_spacing = 6,
  };
  TileSpreadRenderPlans plans =
      build_tile_spread_multi( config );
  // The tile spread framework does not support rendering a
  // single spread with multiple different tiles. But that is
  // what we must do in order to replicate the behavior of the
  // OG, which, although it considers fish and corn to be equiva-
  // lent in term of their effects within the colony, it does
  // distinguish them when rendering production/surplus spreads
  // in the population view. Namely, it takes the total number of
  // non-deficit food tiles (the total produced) and it renders
  // the first Nf tiles as fish where Nf is the number of fish
  // produced, and the remainer as corn. This way it makes sense
  // to the player since the tile counts of each type match what
  // is actually being produced. However, the tiles can switch
  // from fish to corn within any of the spread groups, and at
  // any position within it, depending on the relative number of
  // fish and food produced. This is tricky to do with the ex-
  // isting spread algo because not only would a single spread
  // potentially have to be rendered with both fish and corn, but
  // the point at which fish changes to corn might happen in ei-
  // ther the first (consumed) spread or the second (surplus)
  // spread. So the solution here (which is a bit hacky, but
  // should be ok because this should be the only place in the
  // game where we do this) is to just compute the spread as-
  // suming all food tiles are corn, then after we will replace
  // the first Nf corn tiles with fish tiles. In order to work
  // properly, this requires that the corn and fish tiles have
  // the same trimmed dimensions, which we validate when brining
  // up the engine.
  //
  // A subtle point here is that the spread algo, depending on
  // how much space you give it, is not required to emit the
  // exact number of tiles that you ask it; it might emit less in
  // a given spread if there is limited space. So we need to be
  // defensive below and allow that we may not encounter as many
  // food icons as we've produced. This function will detect that
  // and just stop early. The visual effect that results is that
  // all of the tiles are fish, even though there might be some
  // corn being produced.
  replace_first_n_tiles(
      plans, production.food_horses.fish_produced,
      e_tile::commodity_food_20, e_tile::product_fish_20 );
  return plans;
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
    SSConst const& ss, size const sz ) {
  Layout l;
  l.size          = sz;
  l.spread_margin = 6;
  l.spread_origin = { .x = l.spread_margin, .y = 16 + 32 + 6 };
  int const spread_width = sz.w - 2 * l.spread_margin;
  l.production_spreads   = create_production_spreads(
      ss, colview_production(), spread_width );
  return l;
}

void PopulationView::update_this_and_children() {
  // This method is only called when the logical resolution
  // hasn't changed, so we assume the size hasn't changed.
  layout_ = create_layout( ss_, layout_.size );
}

std::unique_ptr<PopulationView> PopulationView::create(
    SS& ss, TS& ts, Player& player, Colony& colony,
    Delta size ) {
  Layout layout = create_layout( ss.as_const, size );
  return std::make_unique<PopulationView>(
      ss, ts, player, colony, std::move( layout ) );
}

} // namespace rn
