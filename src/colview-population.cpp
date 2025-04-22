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
#include "iengine.hpp"
#include "production.hpp"
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

TileSpreadRenderPlan create_people_spread(
    rr::ITextometer const& textometer, SSConst const& ss,
    Colony const& colony, int const width ) {
  vector<UnitId> const units = colony_units_all( colony );
  vector<TileWithOptions> tiles;
  tiles.reserve( units.size() );
  for( UnitId const unit_id : units )
    tiles.push_back( TileWithOptions{
      .tile = tile_for_unit_type(
          ss.units.unit_for( unit_id ).type() ) } );
  InhomogeneousTileSpreadConfig const config{
    .tiles       = std::move( tiles ),
    .max_spacing = 2,
    .options     = { .bounds       = width,
                     .label_policy = SpreadLabels::never{} } };
  return build_inhomogeneous_tile_spread( textometer, config );
}

TileSpreadRenderPlans create_production_spreads(
    IEngine& engine, SSConst const& ss,
    ColonyProduction const& production, int const width ) {
  ColonyViewFoodStats const food_stats =
      compute_colony_view_food_stats( production );
  TileSpreadConfigMulti const config{
    .tiles{
      { .tile  = e_tile::commodity_food_20,
        .count = food_stats.consumed },
      { .tile  = e_tile::commodity_food_20,
        .count = food_stats.surplus },
      { .tile   = e_tile::commodity_food_20,
        .count  = food_stats.deficit,
        .red_xs = SpreadXs{ .starting_position = 0 } },
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
      build_tile_spread_multi( engine.textometer(), config );
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
void PopulationView::draw_people_spread( rr::Renderer& renderer,
                                         Coord coord ) const {
  point const origin =
      layout_.people_spread_origin.origin_becomes_point( coord );
  draw_rendered_icon_spread( renderer, origin,
                             layout_.people_spread );
}

void PopulationView::draw_production_spreads(
    rr::Renderer& renderer, Coord coord ) const {
  point const origin =
      layout_.production_spread_origin.origin_becomes_point(
          coord );
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
      ss_.settings.game_setup_options.difficulty, info.tories );
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
  rr::Typer typer = renderer.typer( rr::TextLayout{} );
  typer.set_color( text_color );
  gfx::size const tories_text_width =
      typer.dimensions_for_line( tories_str );
  pos.x = bounds( coord ).right_edge() - kIconPadding -
          sprite_size( e_tile::crown ).w - kIconPadding -
          tories_text_width.w;
  typer.set_position( pos.to_gfx() +
                      gfx::size{ .h = kTextVerticalOffset } );
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
  draw_people_spread( renderer, coord );

  // Production Spreads.
  draw_production_spreads( renderer, coord );
}

PopulationView::Layout PopulationView::create_layout(
    IEngine& engine, SSConst const& ss, size const sz,
    Colony const& colony ) {
  Layout l;
  l.size                     = sz;
  l.spread_margin            = 6;
  l.people_spread_origin     = { .x = l.spread_margin, .y = 16 };
  l.production_spread_origin = { .x = l.spread_margin,
                                 .y = 16 + 32 + 6 };
  int const spread_width     = sz.w - 2 * l.spread_margin;
  l.people_spread            = create_people_spread(
      engine.textometer(), ss, colony, spread_width );
  l.production_spreads = create_production_spreads(
      engine, ss, colview_production(), spread_width );
  return l;
}

void PopulationView::update_this_and_children() {
  // This method is only called when the logical resolution
  // hasn't changed, so we assume the size hasn't changed.
  layout_ = create_layout( engine_, ss_, layout_.size, colony_ );
}

std::unique_ptr<PopulationView> PopulationView::create(
    IEngine& engine, SS& ss, TS& ts, Player& player,
    Colony& colony, Delta size ) {
  Layout layout =
      create_layout( engine, ss.as_const, size, colony );
  return std::make_unique<PopulationView>(
      engine, ss, ts, player, colony, std::move( layout ) );
}

} // namespace rn
