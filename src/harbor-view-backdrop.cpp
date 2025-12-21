/****************************************************************
**harbor-view-backdrop.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Backdrop image layout within the harbor view.
*
*****************************************************************/
#include "harbor-view-backdrop.hpp"

// Revolution Now
#include "co-time.hpp"
#include "irand.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/nation.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/player.rds.hpp"

// render
#include "render/renderer.hpp"

using namespace std;

namespace rn {

namespace {

using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** HarborBackdrop
*****************************************************************/
Delta HarborBackdrop::delta() const { return size_; }

maybe<int> HarborBackdrop::entity() const {
  return static_cast<int>( e_harbor_view_entity::backdrop );
}

ui::View& HarborBackdrop::view() noexcept { return *this; }

ui::View const& HarborBackdrop::view() const noexcept {
  return *this;
}

wait<> HarborBackdrop::birds_thread() {
  using namespace std::chrono_literals;
  auto& birds_state = birds_state_.emplace();
  SCOPE_EXIT { birds_state_.reset(); };
  birds_state.frame_states.resize( layout_.birds_states.size() );
  CHECK( !birds_state.frame_states.empty() );
  for( int frame_idx = 0;
       auto& frame : birds_state.frame_states ) {
    frame.frame   = frame_idx++;
    frame.visible = 0;
  }
  birds_state.frame_states[0].visible =
      BirdsFrameState::kVisTotal;
  int constexpr kDelta = 5;
  static_assert( BirdsFrameState::kVisTotal % kDelta == 0 );
  int constexpr kCycles = BirdsFrameState::kVisTotal / kDelta;
  chrono::milliseconds constexpr kDepixelateInterval = 50ms;

  while( true ) {
    co_await 10s;

    for( int i = 0; i < kCycles; ++i ) {
      co_await kDepixelateInterval;
      auto it = find_if( birds_state.frame_states.begin(),
                         birds_state.frame_states.end(),
                         []( BirdsFrameState const& state ) {
                           return state.visible > 0;
                         } );
      if( it == birds_state.frame_states.end() ) co_return;
      it->visible = std::max( it->visible - kDelta, 0 );
      ++it;
      if( it == birds_state.frame_states.end() ) continue;
      it->visible = std::min( it->visible + kDelta,
                              BirdsFrameState::kVisTotal );
    }
  }
}

wait<> HarborBackdrop::smoke_thread() {
  using namespace std::chrono_literals;
  using namespace std::chrono;
  int constexpr kCycles            = 20;
  milliseconds constexpr kInterval = 150ms;
  auto& l_stage                    = smoke_state_.l_stage;
  auto& m_stage                    = smoke_state_.m_stage;
  auto& r_stage                    = smoke_state_.r_stage;

  auto const move_to_target =
      [&]( SmokeState const state ) -> wait<> {
    double const l_delta = ( state.l_stage - l_stage ) / kCycles;
    double const m_delta = ( state.m_stage - m_stage ) / kCycles;
    double const r_delta = ( state.r_stage - r_stage ) / kCycles;
    SCOPE_EXIT { smoke_state_ = state; };
    if( abs( l_delta ) < 0.001 && abs( m_delta ) < 0.001 &&
        abs( r_delta ) < 0.001 )
      co_return;
    for( int i = 0; i < kCycles; ++i ) {
      co_await kInterval;
      l_stage += l_delta;
      m_stage += m_delta;
      r_stage += r_delta;
    }
  };

  smoke_state_ = { 0.0, 0.0, 1.0 };
  while( true ) {
    co_await move_to_target( { 0.0, 0.0, 1.0 } );
    co_await move_to_target( { 0.0, 1.0, 0.0 } );
    co_await move_to_target( { 1.0, 0.0, 0.5 } );
  }
}

wait<> HarborBackdrop::flag_thread() {
  using namespace std::chrono_literals;
  using namespace std::chrono;
  int constexpr kCycles            = 20;
  milliseconds constexpr kInterval = 150ms;
  auto& l_stage                    = flag_state_.l_stage;
  auto& r_stage                    = flag_state_.r_stage;

  auto const move_to_target =
      [&]( FlagState const state ) -> wait<> {
    double const l_delta = ( state.l_stage - l_stage ) / kCycles;
    double const r_delta = ( state.r_stage - r_stage ) / kCycles;
    SCOPE_EXIT { flag_state_ = state; };
    if( abs( l_delta ) < 0.001 && abs( r_delta ) < 0.001 )
      co_return;
    for( int i = 0; i < kCycles; ++i ) {
      co_await kInterval;
      l_stage += l_delta;
      r_stage += r_delta;
    }
  };

  flag_state_ = { 1.0, 0.0 };
  while( true ) {
    co_await move_to_target( { 1.0, 1.0 } );
    co_await move_to_target( { 1.0, 0.0 } );
  }
}

void HarborBackdrop::draw( rr::Renderer& renderer,
                           Coord coord ) const {
  rr::Painter painter = renderer.painter();

  // Draw sky.
  painter.draw_solid_rect( Rect::from( coord, size_ ),
                           layout_.sky_color );

  // Sun.
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
    tile_sprite( renderer, e_tile::harbor_sun, layout_.sun );
  }

  // Clouds.
  for( auto const& [delta, tile] : layout_.clouds )
    render_sprite( renderer, layout_.clouds_origin + delta,
                   tile );

  // Birds.
  if( birds_state_.has_value() ) {
    for( BirdsFrameState const& state :
         birds_state_->frame_states ) {
      CHECK_LT( state.frame,
                int( layout_.birds_states.size() ) );
      auto const& birds_state =
          layout_.birds_states[state.frame];
      SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                               ( 100 - state.visible ) / 100.0 );
      render_sprite( renderer, birds_state.p, birds_state.tile );
    }
  }

  // Ocean.
  tile_sprite( renderer, e_tile::harbor_ocean, layout_.ocean );

  // Land.
  render_sprite( renderer, layout_.land_origin,
                 e_tile::harbor_land_shadows );
  render_sprite( renderer, layout_.land_origin,
                 e_tile::harbor_land_dirt );

  // Flag. Needs to be behind houses because in some configura-
  // tions the bottom of the pole needs to go behind a building.
  auto const render_flag_no_pole = [&]( point const p ) {
    render_sprite_silhouette(
        renderer, p, e_tile::harbor_flag_silhouette,
        config_nation.players[colonial_player_.type]
            .flag_color );
    {
      SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, 0.3 );
      render_sprite( renderer, p, e_tile::harbor_flag_shades );
    }
  };
  render_sprite( renderer, layout_.flag_origin,
                 e_tile::harbor_flag_pole );
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             1.0 - flag_state_.l_stage );
    render_flag_no_pole( layout_.flag_origin );
  }
  {
    SCOPED_RENDERER_MOD_MUL( painter_mods.depixelate.stage,
                             1.0 - flag_state_.r_stage );
    render_flag_no_pole( layout_.flag_origin + size{ .w = 1 } );
  }

  // Houses.  These need to be in order.
  render_sprite( renderer, layout_.houses_origin,
                 e_tile::harbor_houses_trees_far );
  render_sprite( renderer, layout_.houses_origin,
                 e_tile::harbor_houses_buildings );
  render_sprite( renderer, layout_.houses_origin,
                 e_tile::harbor_houses_road );
  render_sprite( renderer, layout_.houses_origin,
                 e_tile::harbor_houses_docks );

  // Smoke.
  {
    point const p = layout_.houses_origin - size{ .w = 8 };
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                             p );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             smoke_state_.l_stage );
    render_sprite( renderer, p, e_tile::harbor_houses_smoke );
  }
  {
    point const p = layout_.houses_origin - size{ .w = 4 };
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                             p );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             smoke_state_.m_stage );
    render_sprite( renderer, p, e_tile::harbor_houses_smoke );
  }
  {
    point const p = layout_.houses_origin - size{ .w = 0 };
    SCOPED_RENDERER_MOD_SET( painter_mods.depixelate.hash_anchor,
                             p );
    SCOPED_RENDERER_MOD_MUL( painter_mods.alpha,
                             smoke_state_.r_stage );
    render_sprite( renderer, p, e_tile::harbor_houses_smoke );
  }

  // Dock. NOTE: the dock overlay is draw in a separate function
  // so that it can be drawn over the doc units.
  render_sprite( renderer, layout_.dock_sprite_nw,
                 e_tile::harbor_dock_shadows );
  render_sprite( renderer, layout_.dock_sprite_nw,
                 e_tile::harbor_dock );
}

void HarborBackdrop::draw_dock_overlay( rr::Renderer& renderer,
                                        gfx::point ) const {
  render_sprite( renderer, layout_.dock_sprite_nw,
                 e_tile::harbor_dock_overlay );
}

DockUnitsLayout const& HarborBackdrop::dock_units_layout()
    const {
  return layout_.dock_units;
}

point HarborBackdrop::horizon_center() const {
  return layout_.horizon_center;
}

int HarborBackdrop::extra_space_for_ships() const {
  return layout_.extra_space_for_ships;
}

#define CLOUD( size, n ) e_tile::harbor_cloud_##size##_##n

void HarborBackdrop::insert_clouds( Layout& l,
                                    size const shift ) {
  auto const add_cloud = [&]( gfx::size const d,
                              e_tile const tile ) {
    l.clouds.push_back( std::pair{ d + shift, tile } );
  };
  add_cloud( { .w = -193, .h = -267 }, CLOUD( large, 3 ) );
  add_cloud( { .w = 68, .h = -267 }, CLOUD( large, 1 ) );
  add_cloud( { .w = -50, .h = -320 }, CLOUD( large, 2 ) );
  add_cloud( { .w = -300, .h = -330 }, CLOUD( large, 6 ) );
  add_cloud( { .w = 250, .h = -330 }, CLOUD( large, 4 ) );

  add_cloud( { .w = -281, .h = -203 }, CLOUD( large, 1 ) );
  add_cloud( { .w = 126, .h = -202 }, CLOUD( large, 2 ) );
  add_cloud( { .w = 102, .h = -176 }, CLOUD( large, 3 ) );
  add_cloud( { .w = -238, .h = -142 }, CLOUD( large, 4 ) );
  add_cloud( { .w = -58, .h = -124 }, CLOUD( large, 5 ) );
  add_cloud( { .w = 128, .h = -115 }, CLOUD( large, 6 ) );
  add_cloud( { .w = -72, .h = -213 }, CLOUD( large, 7 ) );

  add_cloud( { .w = -267, .h = -88 }, CLOUD( medium, 1 ) );
  add_cloud( { .w = 53, .h = -85 }, CLOUD( medium, 2 ) );
  add_cloud( { .w = 203, .h = -72 }, CLOUD( medium, 3 ) );
  add_cloud( { .w = -143, .h = -55 }, CLOUD( medium, 4 ) );
  add_cloud( { .w = 6, .h = -45 }, CLOUD( medium, 5 ) );

  add_cloud( { .w = -118, .h = -92 }, CLOUD( small, 1 ) );
  add_cloud( { .w = -193, .h = -30 }, CLOUD( small, 2 ) );
  add_cloud( { .w = 95, .h = -21 }, CLOUD( small, 3 ) );
  add_cloud( { .w = 251, .h = -32 }, CLOUD( small, 4 ) );

  add_cloud( { .w = -130, .h = -17 }, CLOUD( tiny, 1 ) );
  add_cloud( { .w = 62, .h = -7 }, CLOUD( tiny, 2 ) );
}

HarborBackdrop::Layout HarborBackdrop::recomposite(
    IRand& rand, size const sz ) {
  Layout l;
  rect const all{ .size = sz };

  // Horizon.
  l.horizon_height = 152;
  CHECK( l.horizon_height ==
         sprite_size( e_tile::harbor_ocean ).h );
  l.horizon_center = { .x = sz.w / 2,
                       .y = sz.h - l.horizon_height };

  // Sun.
  l.sun = all.with_new_bottom_edge(
      sprite_size( e_tile::harbor_sun ).h );

  // Clouds.
  l.clouds_origin =
      point{ .x = sz.w / 2, .y = l.horizon_center.y };
  insert_clouds( l, /*shift=*/size{} );
  insert_clouds( l, /*shift=*/size{ .w = -600, .h = -25 } );
  insert_clouds( l, /*shift=*/size{ .w = 600, .h = -25 } );

  // Ocean.
  l.ocean =
      rect{ .origin = { .x = 0, .y = sz.h - l.horizon_height },
            .size   = { .w = sz.w, .h = l.horizon_height } };

  // Land.
  size const land_shift = sz.w >= 600 ? size{} : size{ .w = 30 };

  l.land_origin = all.se() -
                  sprite_size( e_tile::harbor_land_dirt ) +
                  land_shift;
  point const land_point = l.land_origin + size{ .h = 8 };

  l.extra_space_for_ships = l.land_origin.x - l.horizon_center.x;

  // Houses. This is the origin of the various sprites (layers)
  // in the houses file.
  l.houses_origin = land_point - size{ .w = 10, .h = 53 };

  // Flag. For most resolutions the flag goes over the parliament
  // tower building, but for narrow screens it goes over the
  // church otherwise it would be hidden by the rpt buttons.
  l.flag_origin =
      land_point + ( sz.w > 600
                         ? size{ .w = 228, .h = -48 }
                         : size{ .w = 228 - 24, .h = -48 } );

  // Dock.
  l.dock_physical_nw =
      all.se() - size{ .w = 201, .h = 113 } + land_shift;
  l.dock_sprite_nw = l.dock_physical_nw - size{ .w = 5 };
  l.dock_board_nw  = l.dock_sprite_nw + size{ .w = 5, .h = 6 };

  l.dock_units.right_edge = all.right() - 8;

  // First row (on dock).
  l.dock_units.dock_row_start =
      l.dock_board_nw + size{ .w = 13, .h = 3 };

  // Second row (on hill).
  l.dock_units.hill_row_start =
      l.dock_board_nw + size{ .w = 160, .h = 24 };

  // Extra rows (on ground).
  point ground = l.dock_board_nw + size{ .w = 30, .h = 44 };
  for( int i = 0; i < 30; ++i ) {
    l.dock_units.ground_rows.push_back( ground );
    ++ground.y;
    if( i % 2 == 1 ) ground.x += 2;
  }
  l.dock_units.bottom_edge = ground.y;

  // Birds.
  rect const birds_available_space =
      all.with_new_bottom_edge( l.horizon_center.y - 107 );
  point p_birds{
    .x = rand.between_ints( birds_available_space.left(),
                            birds_available_space.right() ),
    .y = rand.between_ints( birds_available_space.top(),
                            birds_available_space.bottom() ) };
  l.birds_states.emplace_back() = Layout::BirdsLayout{
    .p = p_birds, .tile = e_tile::harbor_birds_1 };
  p_birds += size{ .w = 5, .h = 6 };
  l.birds_states.emplace_back() = Layout::BirdsLayout{
    .p = p_birds, .tile = e_tile::harbor_birds_2 };
  p_birds += size{ .w = 5, .h = 5 };
  l.birds_states.emplace_back() = Layout::BirdsLayout{
    .p = p_birds, .tile = e_tile::harbor_birds_3 };
  p_birds += size{ .w = 3, .h = 3 };
  l.birds_states.emplace_back() = Layout::BirdsLayout{
    .p = p_birds, .tile = e_tile::harbor_birds_4 };
  p_birds += size{ .w = 1, .h = 2 };
  l.birds_states.emplace_back() = Layout::BirdsLayout{
    .p = p_birds, .tile = e_tile::harbor_birds_5 };
  return l;
}

PositionedHarborSubView<HarborBackdrop> HarborBackdrop::create(
    IEngine& engine, SS& ss, TS& ts, IRand& rand, Player& player,
    Rect canvas ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborBackdrop> view;
  HarborSubView* harbor_sub_view = nullptr;

  Layout const layout = recomposite( rand, canvas.delta() );
  point const origin  = {};

  view = make_unique<HarborBackdrop>( engine, ss, ts, player,
                                      canvas.delta(), layout );
  harbor_sub_view          = view.get();
  HarborBackdrop* p_actual = view.get();
  return PositionedHarborSubView<HarborBackdrop>{
    .owned  = { .view = std::move( view ), .coord = origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborBackdrop::HarborBackdrop( IEngine& engine, SS& ss, TS& ts,
                                Player& player, Delta size,
                                Layout layout )
  : HarborSubView( engine, ss, ts, player ),
    size_( size ),
    layout_( layout ),
    birds_thread_( birds_thread() ),
    smoke_thread_( smoke_thread() ),
    flag_thread_( flag_thread() ) {}

} // namespace rn
