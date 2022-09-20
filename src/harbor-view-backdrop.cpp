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

using namespace std;

namespace rn {

namespace {} // namespace

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
                           Coord         coord ) const {
  rr::Painter painter = renderer.painter();

  // 1. Draw sky.
  gfx::pixel const sky_color{
      .r = 0xa0, .g = 0xa0, .b = 0xc0, .a = 0xff };
  painter.draw_solid_rect( Rect::from( coord, size_ ),
                           sky_color );

  // 2. Draw clouds.
  // TODO

  // 3. Draw water.
  {
    Coord coord;
    coord.y = size_.h - layout_.horizon;
    Delta const water_size =
        sprite_size( e_tile::harbor_water_slice );
    while( coord.x < size_.w ) {
      render_sprite( painter, coord,
                     e_tile::harbor_water_slice );
      coord.x += water_size.w;
    }
  }

  // 4. Draw land.
  {
    Coord       coord;
    Delta const land_size =
        sprite_size( e_tile::harbor_land_front );
    // First the main part.
    coord.y                     = size_.h - layout_.horizon - 16;
    coord.x                     = layout_.land_tip;
    Coord const main_upper_left = coord;
    render_sprite( painter, coord, e_tile::harbor_land_front );
    // Next the bottom slices.
    coord = main_upper_left;
    coord.y += land_size.h;
    while( coord.y < size_.h ) {
      render_sprite( painter, coord,
                     e_tile::harbor_land_bottom_slice );
      coord.y += land_size.h;
    }
    // Next the right slices.
    coord = main_upper_left;
    coord.x += land_size.w;
    while( coord.x < size_.w ) {
      render_sprite( painter, coord,
                     e_tile::harbor_land_right_slice );
      coord.x += land_size.w;
    }
    // Next the bottom right pieces.
    coord = main_upper_left;
    coord.y += land_size.h;
    while( coord.y < size_.h ) {
      coord.x = main_upper_left.x + land_size.w;
      while( coord.x < size_.w ) {
        render_sprite( painter, coord,
                       e_tile::harbor_land_right_bottom_slice );
        coord.x += land_size.w;
      }
      coord.y += land_size.h;
    }
  }

  // 5. Draw dock.
  {
    Delta const dock_segment_size =
        sprite_size( e_tile::harbor_dock_middle );
    Coord coord = layout_.dock_lower_right;
    coord.x -= kDockEdgeThickness;
    coord.y -= dock_segment_size.h;
    render_sprite( painter, coord, e_tile::harbor_dock_right );
    for( int i = 0; i < layout_.num_dock_segments; i++ ) {
      coord.x -= dock_segment_size.w;
      render_sprite( painter, coord,
                     e_tile::harbor_dock_middle );
    }
    coord.x -= dock_segment_size.w;
    render_sprite( painter, coord, e_tile::harbor_dock_left );
  }
}

HarborBackdrop::DockUnitsLayout
HarborBackdrop::dock_units_layout() const {
  return DockUnitsLayout{
      .units_start_floor =
          layout_.dock_lower_right -
          Delta{ .w = kDockEdgeThickness } -
          Delta{ .w = layout_.num_dock_segments *
                      kDockSegmentWidth } -
          Delta{ .h = 20 },
      .dock_length =
          layout_.num_dock_segments * kDockSegmentWidth };
}

H HarborBackdrop::top_of_houses() const {
  return layout_.horizon + 16;
}

HarborBackdrop::Layout HarborBackdrop::recomposite(
    Delta size, Coord cargo_upper_right,
    Coord inport_upper_right ) {
  Layout res;
  H      inport_top_from_bottom = size.h - inport_upper_right.y;
  Delta const water_size =
      sprite_size( e_tile::harbor_water_slice );
  res.horizon =
      std::min( inport_top_from_bottom - 16, water_size.h );
  W const kDistanceFromTipToDockRight = 180;
  res.land_tip = inport_upper_right.x + 50;
  if( res.land_tip + kDistanceFromTipToDockRight > size.w )
    res.land_tip = size.w - 8 - kDistanceFromTipToDockRight;
  res.dock_lower_right   = Coord{ .x = res.land_tip + 180,
                                  .y = size.h - res.horizon / 2 };
  res.dock_lower_right.y = std::min( res.dock_lower_right.y,
                                     cargo_upper_right.y - 8 );
  W const available_for_dock =
      res.dock_lower_right.x - inport_upper_right.x - 8;
  W const dock_segment_width =
      sprite_size( e_tile::harbor_dock_middle ).w;
  res.num_dock_segments =
      ( available_for_dock - 2 * kDockEdgeThickness ) /
      dock_segment_width;
  return res;
}

PositionedHarborSubView<HarborBackdrop> HarborBackdrop::create(
    SS& ss, TS& ts, Player& player, Rect canvas,
    Coord cargo_upper_right, Coord inport_upper_right ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborBackdrop> view;
  HarborSubView*             harbor_sub_view = nullptr;

  Layout const layout = recomposite(
      canvas.delta(), cargo_upper_right, inport_upper_right );
  Coord const origin = {};

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
