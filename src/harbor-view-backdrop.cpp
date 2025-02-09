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
#include "tiles.hpp"

// config
#include "config/tile-enum.rds.hpp"

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

  // Ocean.
  tile_sprite( renderer, e_tile::harbor_ocean, layout_.ocean );

  // Land.
  render_sprite( renderer, layout_.land_origin,
                 e_tile::harbor_land_shadows );
  render_sprite( renderer, layout_.land_origin,
                 e_tile::harbor_land_dirt );

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
    size const sz ) {
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
    if( i % 2 == 1 ) ++ground.x;
    if( i % 2 == 1 ) ++ground.x;
    // if( i % 4 == 1 ) ++ground.x;
  }
  l.dock_units.bottom_edge = ground.y;

  return l;
}

PositionedHarborSubView<HarborBackdrop> HarborBackdrop::create(
    SS& ss, TS& ts, Player& player, Rect canvas ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborBackdrop> view;
  HarborSubView* harbor_sub_view = nullptr;

  Layout const layout = recomposite( canvas.delta() );
  point const origin  = {};

  view            = make_unique<HarborBackdrop>( ss, ts, player,
                                                 canvas.delta(), layout );
  harbor_sub_view = view.get();
  HarborBackdrop* p_actual = view.get();
  return PositionedHarborSubView<HarborBackdrop>{
    .owned  = { .view = std::move( view ), .coord = origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborBackdrop::HarborBackdrop( SS& ss, TS& ts, Player& player,
                                Delta size, Layout layout )
  : HarborSubView( ss, ts, player ),
    size_( size ),
    layout_( layout ) {}

} // namespace rn
