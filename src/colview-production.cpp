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
#include "colony-constants.hpp"
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
void ProductionView::create_hammer_spreads(
    Layout::HammerArray& out ) const {
  out = {};
  if( !colony_.construction.has_value() ) return;
  // This will check-fail if it is not possible to construct the
  // thing being constructed, e.g. if we are trying to construct
  // a free colonist, which means that the game is in an invalid
  // state.
  UNWRAP_CHECK_T( auto const reqs, requirements_for_construction(
                                       *colony_.construction ) );

  int const total_hammers_needed = reqs.hammers;
  static_assert(
      std::tuple_size_v<decltype( layout_.hammer_spreads )> ==
      kNumHammerRowsInColonyView );
  int const max_hammers_per_row =
      total_hammers_needed / kNumHammerRowsInColonyView;

  int hammers_remaining = colony_.hammers;
  SpreadLabels label_policy =
      ss_.settings.colony_options.numbers
          ? SpreadLabels{ SpreadLabels::always{} }
          : SpreadLabels{ SpreadLabels::auto_decide{} };
  maybe<int> label_override = colony_.hammers;
  for( int i = 0; i < int( kNumHammerRowsInColonyView ); ++i ) {
    CHECK_GE( hammers_remaining, 0 );
    if( hammers_remaining == 0 ) break;
    int const hammers_in_row =
        std::min( hammers_remaining, max_hammers_per_row );
    hammers_remaining -= hammers_in_row;
    ProgressTileSpreadConfig const config{
      .tile           = e_tile::product_hammers_20,
      .count          = max_hammers_per_row,
      .progress_count = hammers_in_row,
      .label_override = label_override,
      .options =
          {
            .bounds       = layout_.hammer_spread_rect.size.w,
            .label_policy = label_policy,
            .label_opts =
                { .placement =
                      SpreadLabelPlacement::in_first_tile{
                        .placement = e_cdirection::w } },
          },
    };
    out[i] = build_progress_tile_spread( config );
    // Label only on first row.
    label_policy   = SpreadLabels::never{};
    label_override = nothing;
  }
}

void ProductionView::draw_production_spreads(
    rr::Renderer& renderer ) const {
  renderer.painter().draw_vertical_line(
      point{ .x = layout_.hammer_spread_rect.right() + 1,
             .y = 0 },
      layout_.size.h, pixel::black() );
  int y = layout_.hammer_spread_rect.origin.y;
  for( TileSpreadRenderPlan const& plan :
       layout_.hammer_spreads ) {
    draw_rendered_icon_spread(
        renderer, layout_.hammer_spread_rect.origin.with_y( y ),
        plan );
    y += layout_.hammer_row_interval;
  }
}

void ProductionView::draw( rr::Renderer& renderer,
                           Coord const coord ) const {
  rr::Painter painter = renderer.painter();
  painter.draw_empty_rect( bounds( coord ).with_inc_size(),
                           rr::Painter::e_border_mode::inside,
                           BROWN_COLOR );
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      gfx::size( coord.distance_from_origin() ).to_double() );
  rr::Typer typer =
      renderer.typer( Coord{ .x = 2, .y = 2 }, BROWN_COLOR );
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
  create_hammer_spreads( layout_.hammer_spreads );
}

ProductionView::Layout ProductionView::create_layout(
    size const sz ) {
  Layout l;
  l.size            = sz;
  int const kMargin = 2;
  l.margin          = kMargin;

  int const text_height = 2 * 8;

  // Hammer spread.
  l.hammer_tile             = e_tile::product_hammers_20;
  auto const hammer_trimmed = trimmed_area_for( l.hammer_tile );
  l.hammer_spread_rect.size = sz;
  l.hammer_spread_rect =
      l.hammer_spread_rect
          .with_new_top_edge( kMargin + text_height + kMargin )
          .with_new_bottom_edge( l.hammer_spread_rect.bottom() -
                                 kMargin )
          .with_new_left_edge( kMargin )
          .with_new_right_edge( l.hammer_spread_rect.right() -
                                kMargin );
  l.hammer_spread_rect.size.w =
      std::min( l.hammer_spread_rect.size.w,
                ( hammer_trimmed.size.w + 1 ) * 13 - 1 );
  l.hammer_row_interval = hammer_trimmed.size.h - 3;
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
