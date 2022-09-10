/****************************************************************
**harbor-view-cargo.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-09.
*
* Description: Cargo UI element within the harbor view.
*
*****************************************************************/
#include "harbor-view-cargo.hpp"

// Revolution Now
#include "commodity.hpp"
#include "render.hpp"
#include "tiles.hpp"

// ss
#include "ss/cargo.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

namespace rl = base::rl;

namespace rn {

namespace {} // namespace

/****************************************************************
** HarborCargo
*****************************************************************/
Delta HarborCargo::delta() const {
  int x_boxes = 6;
  int y_boxes = 1;
  // +1 in each dimension for the border.
  return Delta{ .w = 32 * x_boxes + 1, .h = 32 * y_boxes + 1 };
}

maybe<int> HarborCargo::entity() const {
  return static_cast<int>( e_harbor_view_entity::cargo );
}

ui::View& HarborCargo::view() noexcept { return *this; }

ui::View const& HarborCargo::view() const noexcept {
  return *this;
}

maybe<UnitId> HarborCargo::get_active_unit() const {
  return player_.old_world.harbor_state.selected_unit;
}

maybe<HarborDraggableObject_t>
HarborCargo::draggable_in_cargo_slot( int slot ) const {
  maybe<UnitId> const active_unit = get_active_unit();
  Unit const&         unit = ss_.units.unit_for( *active_unit );
  if( slot >= unit.cargo().slots_total() ) return nothing;
  CargoSlot_t const& cargo = unit.cargo()[slot];
  switch( cargo.to_enum() ) {
    case CargoSlot::e::empty: return nothing;
    case CargoSlot::e::overflow: return nothing;
    case CargoSlot::e::cargo: {
      Cargo_t const& draggable =
          cargo.get<CargoSlot::cargo>().contents;
      switch( draggable.to_enum() ) {
        case Cargo::e::commodity: {
          Commodity const comm =
              draggable.get<Cargo::commodity>().obj;
          return HarborDraggableObject::cargo_commodity{
              .comm = comm, .slot = slot };
        }
        case Cargo::e::unit: {
          UnitId const unit_id = draggable.get<Cargo::unit>().id;
          return HarborDraggableObject::unit{ .id = unit_id };
        }
      }
    }
  }
}

maybe<DraggableObjectWithBounds> HarborCargo::object_here(
    Coord const& where ) const {
  maybe<UnitId> active_unit = get_active_unit();
  if( !active_unit.has_value() ) return nothing;
  Unit const& unit  = ss_.units.unit_for( *active_unit );
  auto        boxes = rect( Coord{} ) / g_tile_delta;
  UNWRAP_CHECK( slot, boxes.rasterize( where / g_tile_delta ) );
  if( slot >= unit.cargo().slots_total() ) return nothing;
  maybe<HarborDraggableObject_t> const draggable =
      draggable_in_cargo_slot( slot );
  if( !draggable.has_value() ) return nothing;
  Coord box_origin =
      where.rounded_to_multiple_to_minus_inf( g_tile_delta );
  Delta scale = g_tile_delta;
  using HarborDraggableObject::cargo_commodity;
  if( draggable->holds<cargo_commodity>() ) {
    box_origin += kCommodityInCargoHoldRenderingOffset;
    scale = kCommodityTileSize;
  }
  Rect const box = Rect::from( box_origin, scale );
  return DraggableObjectWithBounds{ .obj    = draggable,
                                    .bounds = box };
}

void HarborCargo::draw( rr::Renderer& renderer,
                        Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  auto        r       = rect( coord );
  // Our delta for this view has one extra pixel added to the
  // width and height to allow for the border, and so we need to
  // remove that otherwise the to-grid method below will create
  // too many grid boxes.
  --r.w;
  --r.h;
  auto const grid = r.to_grid_noalign( g_tile_delta );
  for( Rect const rect : grid )
    painter.draw_solid_rect( rect, gfx::pixel::white() );
  maybe<UnitId> active_unit = get_active_unit();
  if( !active_unit.has_value() ) return;

  // Draw the contents of the cargo since we have a selected ship
  // in port.
  auto&       unit        = ss_.units.unit_for( *active_unit );
  auto const& cargo_slots = unit.cargo().slots();
  auto        zipped = rl::zip( rl::ints(), cargo_slots, grid );
  for( auto const [idx, cargo_slot, rect] : zipped ) {
    painter.draw_solid_rect( rect.edges_removed(),
                             gfx::pixel::wood() );
    if( dragging_.has_value() && dragging_->slot == idx )
      continue;
    Coord const dst_coord       = rect.upper_left();
    auto const  cargo_slot_copy = cargo_slot;
    switch( auto& v = cargo_slot_copy; v.to_enum() ) {
      case CargoSlot::e::empty: {
        break;
      }
      case CargoSlot::e::overflow: {
        break;
      }
      case CargoSlot::e::cargo: {
        auto& cargo = v.get<CargoSlot::cargo>();
        overload_visit(
            cargo.contents,
            [&]( Cargo::unit u ) {
              render_unit( renderer, dst_coord,
                           ss_.units.unit_for( u.id ),
                           UnitRenderOptions{ .flag = false } );
            },
            [&]( Cargo::commodity const& c ) {
              render_commodity_annotated(
                  renderer,
                  dst_coord +
                      kCommodityInCargoHoldRenderingOffset,
                  c.obj );
            } );
        break;
      }
    }
  }
}

PositionedHarborSubView HarborCargo::create( SS& ss, TS& ts,
                                             Player& player,
                                             Rect    canvas ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborCargo> view;
  HarborSubView*          harbor_sub_view = nullptr;
  Coord                   pos;

  // This is the size without the bottom/right border.
  Delta const size_pixels = { .w = 32 * 6, .h = 32 * 1 };
  pos = { .x = canvas.center().x - size_pixels.w / 2,
          .y = canvas.bottom_edge() - size_pixels.h };

  view            = make_unique<HarborCargo>( ss, ts, player );
  harbor_sub_view = view.get();
  return PositionedHarborSubView{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view };
}

HarborCargo::HarborCargo( SS& ss, TS& ts, Player& player )
  : HarborSubView( ss, ts, player ) {}

} // namespace rn
