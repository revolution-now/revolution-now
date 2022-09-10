/****************************************************************
**harbor-view-inport.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: In-port ships UI element within the harbor view.
*
*****************************************************************/
#include "harbor-view-inport.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "harbor-units.hpp"
#include "harbor-view-market.hpp"
#include "render.hpp"
#include "tiles.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** HarborInPortShips
*****************************************************************/
Delta HarborInPortShips::size_blocks( bool is_wide ) {
  static constexpr H height_blocks{ 3 };
  static constexpr W width_wide{ 3 };
  static constexpr W width_narrow{ 2 };
  return Delta{ .w = is_wide ? width_wide : width_narrow,
                .h = height_blocks };
}

// This is the size without the lower/right border.
Delta HarborInPortShips::size_pixels( bool is_wide ) {
  Delta res = size_blocks( is_wide );
  return res * g_tile_delta;
}

Delta HarborInPortShips::delta() const {
  Delta res = size_pixels( is_wide_ );
  // +1 in each dimension for the border.
  ++res.w;
  ++res.h;
  return res;
}

maybe<int> HarborInPortShips::entity() const {
  return static_cast<int>( e_harbor_view_entity::in_port );
}

ui::View& HarborInPortShips::view() noexcept { return *this; }

ui::View const& HarborInPortShips::view() const noexcept {
  return *this;
}

maybe<UnitId> HarborInPortShips::get_active_unit() const {
  return player_.old_world.harbor_state.selected_unit;
}

void HarborInPortShips::set_active_unit( UnitId unit_id ) {
  CHECK( as_const( ss_.units )
             .ownership_of( unit_id )
             .holds<UnitOwnership::harbor>() );
  player_.old_world.harbor_state.selected_unit = unit_id;
}

maybe<HarborInPortShips::UnitWithPosition>
HarborInPortShips::unit_at_location( Coord where ) const {
  for( auto [id, coord] : units( Coord{} ) ) {
    Rect const r = Rect::from( coord, g_tile_delta );
    if( where.is_inside( r ) )
      return UnitWithPosition{ .id = id, .pixel_coord = coord };
  }
  return nothing;
}

maybe<DraggableObjectWithBounds> HarborInPortShips::object_here(
    Coord const& where ) const {
  maybe<UnitWithPosition> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  return DraggableObjectWithBounds{
      .obj    = HarborDraggableObject::unit{ .id = unit->id },
      .bounds = Rect::from( unit->pixel_coord, g_tile_delta ) };
}

vector<HarborInPortShips::UnitWithPosition>
HarborInPortShips::units( Coord origin ) const {
  vector<UnitWithPosition> units;
  Rect const               r = rect( origin );
  Coord coord                = r.lower_right() - g_tile_delta;
  for( UnitId id :
       harbor_units_in_port( ss_.units, player_.nation ) ) {
    units.push_back( { .id = id, .pixel_coord = coord } );
    coord -= Delta{ .w = g_tile_delta.w };
    if( coord.x < r.left_edge() )
      coord = Coord{ ( r.lower_right() - g_tile_delta ).x,
                     coord.y - g_tile_delta.h };
  }
  return units;
}

wait<> HarborInPortShips::click_on_unit( UnitId unit_id ) {
  if( get_active_unit() == unit_id ) {
    // TODO: present menu.
  }
  set_active_unit( unit_id );
  co_return;
}

wait<> HarborInPortShips::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( rect( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

void HarborInPortShips::draw( rr::Renderer& renderer,
                              Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  auto        r       = rect( coord );
  painter.draw_empty_rect( r, rr::Painter::e_border_mode::inside,
                           gfx::pixel::white() );
  rr::Typer typer =
      renderer.typer( r.upper_left() + Delta{ .w = 2, .h = 2 },
                      gfx::pixel::white() );
  typer.write( "In Port" );

  HarborState const& hb_state = player_.old_world.harbor_state;

  for( auto const& [unit_id, unit_coord] : units( coord ) ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    render_unit( renderer, unit_coord,
                 ss_.units.unit_for( unit_id ),
                 UnitRenderOptions{ .flag = false } );
    if( hb_state.selected_unit == unit_id )
      painter.draw_empty_rect(
          Rect::from( unit_coord, g_tile_delta ) -
              Delta{ .w = 1, .h = 1 },
          rr::Painter::e_border_mode::in_out,
          gfx::pixel::green() );
  }
}

PositionedHarborSubView HarborInPortShips::create(
    SS& ss, TS& ts, Player& player, Rect,
    HarborMarketCommodities const& market_commodities,
    Coord                          harbor_cargo_upper_left ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborInPortShips> view;
  HarborSubView*                harbor_sub_view = nullptr;

  bool const  is_wide = !market_commodities.stacked();
  Delta const size    = size_pixels( is_wide );
  Coord const pos =
      harbor_cargo_upper_left - Delta{ .h = size.h };

  view =
      make_unique<HarborInPortShips>( ss, ts, player, is_wide );
  harbor_sub_view = view.get();
  return PositionedHarborSubView{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view };
}

HarborInPortShips::HarborInPortShips( SS& ss, TS& ts,
                                      Player& player,
                                      bool    is_wide )
  : HarborSubView( ss, ts, player ), is_wide_( is_wide ) {}

} // namespace rn
