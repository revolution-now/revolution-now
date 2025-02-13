/****************************************************************
**colview-production.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-02-12.
*
* Description: Production view UI within the colony view.
*
*****************************************************************/
#include "colview-production.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "colview-entities.hpp"
#include "construction.hpp"
#include "production.rds.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/tile-enum.rds.hpp"

// render
#include "render/renderer.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::rn::config::colony::construction_requirements;

maybe<construction_requirements> requirements_for_construction(
    Construction const& project ) {
  switch( project.to_enum() ) {
    case Construction::e::building:
      return config_colony.requirements_for_building
          [project.get<Construction::building>().what];
    case Construction::e::unit:
      return config_colony.requirements_for_unit
          [project.get<Construction::unit>().type];
  }
}

} // namespace

/****************************************************************
** ProductionView
*****************************************************************/
TileSpreadRenderPlan ProductionView::create_hammer_spread()
    const {
  if( !colony_.construction.has_value() ) return {};
  // This will check-fail if it is not possible to construct the
  // thing being constructed, e.g. if we are trying to construct
  // a free colonist, which means that the game is in an invalid
  // state.
  UNWRAP_CHECK_T( auto const reqs, requirements_for_construction(
                                       *colony_.construction ) );
  int const total_hammers_needed = reqs.hammers;
  TileSpreadConfig const config{
    .tile    = { .tile           = e_tile::product_hammers_20,
                 .count          = total_hammers_needed,
                 .progress_count = colony_.hammers,
                 .line_breaks =
                     TileSpreadLineBreaks{
                       .line_width =
                        layout_.hammer_spread_rect.size.w,
                       .line_spacing =
                        layout_.hammer_row_interval } },
    .options = {
      .bounds = layout_.effective_hammer_spread_width,
      .label_policy =
          ss_.settings.colony_options.numbers
              ? SpreadLabels{ SpreadLabels::always{} }
              : SpreadLabels{ SpreadLabels::auto_decide{} },
      .label_opts =
          { .placement =
                SpreadLabelPlacement::left_middle_adjusted{} },
    } };
  return build_tile_spread( config );
}

void ProductionView::draw_production_spreads(
    rr::Renderer& renderer ) const {
  renderer.painter().draw_empty_rect(
      layout_.hammer_spread_rect,
      rr::Painter::e_border_mode::inside, pixel::red() );
  draw_rendered_icon_spread( renderer,
                             layout_.hammer_spread_rect.origin,
                             layout_.hammer_spread );
}

void ProductionView::draw( rr::Renderer& renderer,
                           Coord const coord ) const {
  rr::Painter painter = renderer.painter();
  painter.draw_empty_rect( bounds( coord ).with_inc_size(),
                           rr::Painter::e_border_mode::inside,
                           gfx::pixel::black() );
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      gfx::size( coord.distance_from_origin() ).to_double() );
  rr::Typer typer = renderer.typer( Coord{ .x = 2, .y = 2 },
                                    gfx::pixel::black() );
  typer.write( "Construction: " );
  if( colony_.construction.has_value() ) {
    typer.write( "{}\n",
                 construction_name( *colony_.construction ) );
    typer.write( "right-click to buy.\n" );
  } else {
    typer.write( "nothing\n" );
  }

  // Production Spreads.
  draw_production_spreads( renderer );
}

// Implement AwaitView.
wait<> ProductionView::perform_click(
    input::mouse_button_event_t const& event ) {
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  if( event.buttons == input::e_mouse_button_event::right_up ) {
    maybe<RushConstruction> const invoice =
        rush_construction_cost( ss_, colony_ );
    if( !invoice.has_value() )
      // This can happen if either the colony is not building
      // anything or if it is building something that it al-
      // ready has.
      co_return;
    co_await rush_construction_prompt( player_, colony_, ts_.gui,
                                       *invoice );
    co_return;
  }
  co_await select_colony_construction( ss_, ts_, colony_ );
}

void ProductionView::update_this_and_children() {
  // This method is only called when the logical resolution
  // hasn't changed, so we assume the size hasn't changed.
  layout_ = create_layout( layout_.size );
  update_hammer_spread();
}

void ProductionView::update_hammer_spread() {
  layout_.hammer_spread = create_hammer_spread();
}

ProductionView::Layout ProductionView::create_layout(
    size const sz ) {
  Layout l;
  l.size            = sz;
  int const kMargin = 2;
  l.margin          = kMargin;

  int const text_height = 2 * 8;

  // Hammer spread.
  l.hammer_spread_rect.size = sz;
  l.hammer_spread_rect =
      l.hammer_spread_rect
          .with_new_top_edge( kMargin + text_height + kMargin )
          .with_new_bottom_edge( l.hammer_spread_rect.bottom() -
                                 kMargin )
          .with_new_left_edge( kMargin )
          .with_new_right_edge( l.hammer_spread_rect.right() -
                                kMargin );
  l.hammer_tile             = e_tile::product_hammers_20;
  auto const hammer_trimmed = trimmed_area_for( l.hammer_tile );
  int const hammer_top_padding = hammer_trimmed.origin.y;
  int const hammer_bottom_padding =
      sprite_size( l.hammer_tile ).h - hammer_trimmed.bottom();
  l.hammer_row_interval = hammer_trimmed.size.h / 2;
  l.num_hammer_rows =
      std::max( ( l.hammer_spread_rect.size.h -
                  hammer_top_padding - hammer_bottom_padding ) /
                        l.hammer_row_interval -
                    0,
                1 );
  // FIXME: the total width of the last row is not being used.
  l.effective_hammer_spread_width =
      l.num_hammer_rows * l.hammer_spread_rect.size.w -
      ( hammer_trimmed.size.w + 1 ) * l.num_hammer_rows;
  return l;
}

unique_ptr<ProductionView> ProductionView::create(
    SS& ss, TS& ts, Player& player, Colony& colony,
    Delta size ) {
  Layout layout = create_layout( size );
  return make_unique<ProductionView>( ss, ts, player, colony,
                                      std::move( layout ) );
}

ProductionView::ProductionView( SS& ss, TS& ts, Player& player,
                                Colony& colony, Layout layout )
  : ColonySubView( ss, ts, player, colony ),
    layout_( std::move( layout ) ) {
  update_hammer_spread();
}

} // namespace rn
