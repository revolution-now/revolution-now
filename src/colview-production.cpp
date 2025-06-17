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
#include "iengine.hpp"
#include "input.hpp"
#include "production.rds.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/colony.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"

// render
#include "render/renderer.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

namespace rl = base::rl;

namespace rn {

namespace {

using ::base::NoDiscard;
using ::gfx::centered_in;
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
    out[i] = build_progress_tile_spread( engine_.textometer(),
                                         config );
    // Label only on first row.
    label_policy   = SpreadLabels::never{};
    label_override = nothing;
  }
}

void ProductionView::create_production_spreads(
    Layout::ProductionArray& out ) const {
  TileSpreadOptions const options{
    .bounds = layout_.production_spread_x_bounds.len,
    .label_policy =
        ss_.settings.colony_options.numbers
            ? SpreadLabels{ SpreadLabels::always{} }
            : SpreadLabels{ SpreadLabels::auto_decide{} },
    .label_opts =
        { .placement =
              SpreadLabelPlacement::left_middle_adjusted{} },
  };

  auto const for_raw = []( RawMaterialAndProduct const info,
                           e_tile const tile ) {
    int const produced_count = info.raw_produced;
    int const deficit_count  = std::max(
        info.raw_consumed_theoretical - info.raw_produced, 0 );
    return TileSpread{
      .tile   = tile,
      .count  = produced_count + deficit_count,
      .red_xs = SpreadXs{ .starting_position = produced_count },
    };
  };

  auto const for_produced = []( RawMaterialAndProduct const info,
                                e_tile const tile ) {
    int const produced_count = info.product_produced_actual;
    int const deficit_count = info.product_produced_theoretical -
                              info.product_produced_actual;
    return TileSpread{
      .tile   = tile,
      .count  = produced_count + deficit_count,
      .red_xs = SpreadXs{ .starting_position = produced_count },
    };
  };

  auto const for_horses = []( FoodProduction const info,
                              e_tile const tile ) {
    int const produced_count = info.horses_produced_actual;
    int const deficit_count  = info.horses_produced_theoretical -
                              info.horses_produced_actual;
    return TileSpread{
      .tile   = tile,
      .count  = produced_count + deficit_count,
      .red_xs = SpreadXs{ .starting_position = produced_count },
    };
  };

  auto const& P = colview_production();

  using enum e_tile;
  vector<TileSpread> spreads{
    for_raw( P.sugar_rum, commodity_sugar_20 ),
    for_raw( P.tobacco_cigars, commodity_tobacco_20 ),
    for_raw( P.cotton_cloth, commodity_cotton_20 ),
    for_raw( P.fur_coats, commodity_furs_20 ),
    for_raw( P.ore_tools, commodity_ore_20 ),
    for_produced( P.sugar_rum, commodity_rum_20 ),
    for_produced( P.tobacco_cigars, commodity_cigars_20 ),
    for_produced( P.cotton_cloth, commodity_cloth_20 ),
    for_produced( P.fur_coats, commodity_coats_20 ),
    for_produced( P.ore_tools, commodity_tools_20 ),
    for_raw( P.lumber_hammers, commodity_lumber_20 ),
    for_produced( P.lumber_hammers, product_hammers_20 ),
    for_raw( P.silver, commodity_silver_20 ),
    for_horses( P.food_horses, commodity_horses_20 ),
    for_produced( P.tools_muskets, commodity_muskets_20 ),
  };
  erase_if( spreads,
            []( TileSpread const& o ) { return o.count == 0; } );

  int const num_spreads = ssize( spreads );
  // Should be at least one otherwise `chunks` will crash.
  int const num_per_row =
      std::max( 1, num_spreads % layout_.kNumRows > 0
                       ? ( num_spreads / layout_.kNumRows + 1 )
                       : num_spreads / layout_.kNumRows );
  CHECK_LE( num_spreads, num_per_row * layout_.kNumRows; );
  auto const chunks = rl::all( spreads ).chunk( num_per_row );
  for( int idx = 0; auto const chunk : chunks ) {
    TileSpreadConfigMulti config{
      .options       = options,
      .group_spacing = 6,
    };
    for( TileSpread const& spread : chunk )
      config.tiles.push_back( spread );
    out[idx++].plans =
        build_tile_spread_multi( engine_.textometer(), config );
  }
}

void ProductionView::draw_mode_production(
    rr::Renderer& renderer ) const {
  for( auto const& spread : layout_.production_spreads ) {
    rect const available_rect{
      .origin = { .x = layout_.production_spread_x_bounds.start,
                  .y = spread.origin_y },
      .size   = { .w = layout_.production_spread_x_bounds.len,
                  .h = layout_.production_spreads_row_h } };
    draw_rendered_icon_spread(
        renderer,
        gfx::centered_in( spread.plans.bounds, available_rect ),
        spread.plans );
  }
}

void ProductionView::draw_mode_units( rr::Renderer& ) const {
  // TODO
}

void ProductionView::draw_mode_construction(
    rr::Renderer& renderer ) const {
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
  {
    rr::Painter painter = renderer.painter();
    painter.draw_empty_rect( bounds( coord ).with_inc_size(),
                             rr::Painter::e_border_mode::inside,
                             BROWN_COLOR );
  }
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      gfx::size( coord.distance_from_origin() ).to_double() );
  rr::Painter painter = renderer.painter();

  switch( mode_ ) {
    case e_mode::production:
      draw_mode_production( renderer );
      break;
    case e_mode::units:
      draw_mode_units( renderer );
      break;
    case e_mode::construction:
      draw_mode_construction( renderer );
      break;
  }

  // Buttons.
  for( auto const& [mode, button] : layout_.buttons ) {
    painter.draw_solid_rect( button.bounds,
                             ( mode_ == mode )
                                 ? pixel::wood().highlighted( 3 )
                                 : pixel::wood() );
    render_sprite(
        renderer,
        centered_in( sprite_size( button.tile ), button.bounds ),
        button.tile );
  }
  renderer.painter().draw_vertical_line(
      point{ .x = layout_.buttons_area_rect.left() - 1, .y = 0 },
      layout_.size.h, pixel::black() );
}

wait<NoDiscard<bool>> ProductionView::perform_click(
    input::mouse_button_event_t const& event ) {
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  if( event.pos.is_inside( layout_.buttons_area_rect ) ) {
    for( auto const& [mode, button] : layout_.buttons ) {
      if( event.pos.is_inside( button.bounds ) ) {
        mode_ = mode;
        co_return true;
      }
    }
  }
  switch( mode_ ) {
    case e_mode::production: {
      ss_.settings.colony_options.numbers =
          !ss_.settings.colony_options.numbers;
      update_colony_view( ss_, colony_ );
      break;
    }
    case e_mode::units: {
      // TODO
      break;
    }
    case e_mode::construction: {
      if( event.buttons ==
          input::e_mouse_button_event::right_up ) {
        maybe<RushConstruction> const invoice =
            rush_construction_cost( ss_, colony_ );
        if( !invoice.has_value() )
          // This can happen if either the colony is not building
          // anything or if it is building something that it al-
          // ready has.
          break;
        co_await rush_construction_prompt( player_, colony_,
                                           ts_.gui, *invoice );
        break;
      }
      co_await select_colony_construction( ss_, ts_, colony_ );
      break;
    }
  }
  co_return true;
}

wait<NoDiscard<bool>> ProductionView::perform_key(
    input::key_event_t const& event ) {
  if( event.change != input::e_key_change::down )
    co_return false;
  switch( event.keycode ) {
    case '1':
    case ::SDLK_KP_1:
      mode_ = e_colview_production_mode::production;
      co_return true;
    case '2':
    case ::SDLK_KP_2:
      mode_ = e_colview_production_mode::units;
      co_return true;
    case '3':
    case ::SDLK_KP_3:
      mode_ = e_colview_production_mode::construction;
      co_return true;
  }
  co_return false;
}

void ProductionView::update_this_and_children() {
  // This method is only called when the logical resolution
  // hasn't changed, so we assume the size hasn't changed.
  layout_ = create_layout( layout_.size );
  update_spreads();
}

void ProductionView::update_spreads() {
  create_hammer_spreads( layout_.hammer_spreads );
  create_production_spreads( layout_.production_spreads );
}

ProductionView::Layout ProductionView::create_layout(
    size const sz ) {
  Layout l;
  rect const all{ .size = sz };
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

  // Buttons.
  l.buttons_area_rect =
      all.with_new_left_edge( l.hammer_spread_rect.right() + 2 );
  int const button_h = sz.h / 3 + 1;
  {
    auto& b           = l.buttons[e_mode::production];
    b.bounds          = l.buttons_area_rect;
    b.bounds.origin.y = 1;
    b.bounds.size.h   = button_h - 1;
    b.tile            = e_tile::production_button_house;
  }
  {
    auto& b           = l.buttons[e_mode::units];
    b.bounds          = l.buttons_area_rect;
    b.bounds.origin.y = 1 * button_h;
    b.bounds.size.h   = button_h;
    b.tile            = e_tile::production_button_gun;
  }
  {
    auto& b           = l.buttons[e_mode::construction];
    b.bounds          = l.buttons_area_rect;
    b.bounds.origin.y = 2 * button_h;
    b.bounds.size.h   = button_h - 1;
    b.tile            = e_tile::production_button_hammer;
  }

  // TODO: compute this first and then put the hammer spread in-
  // side of it.
  l.content_rect =
      all.with_new_right_edge( l.buttons_area_rect.left() );

  // Production spreads.
  l.production_spreads_row_h = 20; // 20x20 tiles.
  int constexpr kNumRows     = l.production_spreads.size();
  int const production_spreads_y_gap =
      std::max( ( l.content_rect.size.h -
                  kNumRows * l.production_spreads_row_h ),
                0 ) /
      ( kNumRows + 1 );
  int const production_spreads_x_start = kMargin;
  l.production_spread_x_bounds         = {
            .start = production_spreads_x_start,
            .len   = l.content_rect.size.w - 2 * kMargin };
  // The x origin is computed dynamically later.
  for( int i = 0; auto& row : l.production_spreads ) {
    row.origin_y = ( i + 1 ) * production_spreads_y_gap +
                   i * l.production_spreads_row_h;
    ++i;
  }
  return l;
}

unique_ptr<ProductionView> ProductionView::create(
    IEngine& engine, SS& ss, TS& ts, Player& player,
    Colony& colony, Delta size ) {
  Layout layout = create_layout( size );
  return make_unique<ProductionView>(
      engine, ss, ts, player, colony, std::move( layout ) );
}

ProductionView::ProductionView( IEngine& engine, SS& ss, TS& ts,
                                Player& player, Colony& colony,
                                Layout layout )
  : ColonySubView( engine, ss, ts, player, colony ),
    layout_( std::move( layout ) ) {
  update_spreads();
}

} // namespace rn
