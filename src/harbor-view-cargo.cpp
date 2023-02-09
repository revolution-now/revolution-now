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
#include "harbor-units.hpp"
#include "igui.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// ss
#include "ss/cargo.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// gfx
#include "gfx/iter.hpp"

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

maybe<int> HarborCargo::slot_under_cursor( Coord where ) const {
  maybe<UnitId> active_unit = get_active_unit();
  if( !active_unit.has_value() ) return nothing;
  Unit const& unit  = ss_.units.unit_for( *active_unit );
  auto        boxes = rect( Coord{} ) / g_tile_delta;
  UNWRAP_RETURN( slot, boxes.rasterize( where / g_tile_delta ) );
  if( slot >= unit.cargo().slots_total() ) return nothing;
  return slot;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject_t>>
HarborCargo::object_here( Coord const& where ) const {
  UNWRAP_RETURN( slot, slot_under_cursor( where ) );
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
  return DraggableObjectWithBounds<HarborDraggableObject_t>{
      .obj = *draggable, .bounds = box };
}

bool HarborCargo::try_drag( HarborDraggableObject_t const& o,
                            Coord const& ) {
  dragging_ = nothing;
  // This method will only be called if there was already an ob-
  // ject under the cursor, which can always be dragged, so long
  // as the active ship is in port.
  UNWRAP_CHECK( active_unit_id, get_active_unit() );
  if( !is_unit_in_port( ss_.units, active_unit_id ) )
    return false;
  Unit const& active_unit = ss_.units.unit_for( active_unit_id );
  // Now we have to get the slot of the thing being dragged.
  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      UnitId const unit_id =
          o.get<HarborDraggableObject::unit>().id;
      UNWRAP_CHECK( unit_slot,
                    active_unit.cargo().find_unit( unit_id ) );
      dragging_ =
          Draggable{ .slot = unit_slot, .quantity = nothing };
      break;
    }
    case HarborDraggableObject::e::market_commodity: {
      SHOULD_NOT_BE_HERE;
    }
    case HarborDraggableObject::e::cargo_commodity: {
      int const slot =
          o.get<HarborDraggableObject::cargo_commodity>().slot;
      int const quantity =
          o.get<HarborDraggableObject::cargo_commodity>()
              .comm.quantity;
      dragging_ =
          Draggable{ .slot = slot, .quantity = quantity };
      break;
    }
  }
  return true;
}

void HarborCargo::cancel_drag() { dragging_ = nothing; }

wait<maybe<HarborDraggableObject_t>>
HarborCargo::user_edit_object() const {
  UNWRAP_CHECK( slot, dragging_.member( &Draggable::slot ) );
  UNWRAP_CHECK( draggable, draggable_in_cargo_slot( slot ) );
  auto cargo_commodity =
      draggable.get_if<HarborDraggableObject::cargo_commodity>();
  if( !cargo_commodity.has_value() ) co_return nothing;
  Commodity const& comm        = cargo_commodity->comm;
  int const        max_allowed = comm.quantity;
  CHECK_GT( max_allowed, 0 );
  // FIXME: need to find the right verb here; could be "move" if
  // we're moving to another ship, or could be "sell" if we are
  // selling.
  string const text = fmt::format(
      "What quantity of [{}] would you like to move? "
      "(0-{}):",
      lowercase_commodity_display_name( comm.type ),
      max_allowed );

  maybe<int> const quantity =
      co_await ts_.gui.optional_int_input(
          { .msg           = text,
            .initial_value = comm.quantity,
            .min           = 0,
            .max           = max_allowed } );
  if( !quantity.has_value() ) co_return nothing;
  if( quantity == 0 ) co_return nothing;
  // We shouldn't have to update the dragging_ member here be-
  // cause the framework should call try_drag again with the mod-
  // ified value.
  Commodity new_comm = comm;
  new_comm.quantity  = *quantity;
  CHECK( new_comm.quantity > 0 );
  co_return HarborDraggableObject::cargo_commodity{
      .comm = new_comm, .slot = slot };
}

wait<> HarborCargo::disown_dragged_object() {
  UNWRAP_CHECK( slot, dragging_.member( &Draggable::slot ) );
  UNWRAP_CHECK( active_unit_id, get_active_unit() );
  Unit const& active_unit = ss_.units.unit_for( active_unit_id );
  CargoSlot_t const& held = active_unit.cargo()[slot];
  switch( held.to_enum() ) {
    case CargoSlot::e::empty:
    case CargoSlot::e::overflow: //
      SHOULD_NOT_BE_HERE;
    case CargoSlot::e::cargo: {
      Cargo_t const& cargo =
          held.get<CargoSlot::cargo>().contents;
      switch( cargo.to_enum() ) {
        case Cargo::e::unit: {
          UnitId const held_id = cargo.get<Cargo::unit>().id;
          ss_.units.disown_unit( held_id );
          break;
        }
        case Cargo::e::commodity: {
          // We might be moving a partial amount of the commodity
          // in this slot. So remove all of it, then restore the
          // part that is left, if any.
          UNWRAP_CHECK( quantity, dragging_.maybe_member(
                                      &Draggable::quantity ) );
          Commodity const removed = rm_commodity_from_cargo(
              ss_.units, active_unit_id, slot );
          Commodity to_add = removed;
          to_add.quantity -= quantity;
          CHECK_GE( to_add.quantity, 0 );
          if( to_add.quantity > 0 )
            add_commodity_to_cargo( ss_.units, to_add,
                                    active_unit_id, slot,
                                    /*try_other_slots=*/false );
          break;
        }
      }
    }
  }
  co_return;
}

maybe<HarborDraggableObject_t> HarborCargo::can_receive(
    HarborDraggableObject_t const& o, int from_entity,
    Coord const& where ) const {
  if( !get_active_unit().has_value() ) return nothing;
  UNWRAP_CHECK( active_unit_id, get_active_unit() );
  CONVERT_ENTITY( entity_enum, from_entity );
  if( !is_unit_in_port( ss_.units, active_unit_id ) )
    return nothing;
  Unit const& active_unit = ss_.units.unit_for( active_unit_id );
  UNWRAP_RETURN( slot, slot_under_cursor( where ) );
  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto const&  alt = o.get<HarborDraggableObject::unit>();
      UnitId const dragged_id = alt.id;
      if( entity_enum == e_harbor_view_entity::cargo ) {
        UNWRAP_CHECK( from_slot, active_unit.cargo().find_unit(
                                     dragged_id ) );
        if( !active_unit.cargo()
                 .fits_somewhere_with_item_removed(
                     ss_.units, Cargo::unit{ .id = dragged_id },
                     from_slot, slot ) )
          return nothing;
      } else {
        if( !active_unit.cargo().fits_somewhere(
                ss_.units, Cargo::unit{ .id = dragged_id },
                slot ) )
          return nothing;
      }
      return alt;
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      Commodity const& comm      = alt.comm;
      Commodity        corrected = comm;
      corrected.quantity         = std::min(
          corrected.quantity,
          active_unit.cargo().max_commodity_quantity_that_fits(
              comm.type ) );
      if( corrected.quantity == 0 ) return nothing;
      return HarborDraggableObject::market_commodity{
          .comm = corrected };
    }
    case HarborDraggableObject::e::cargo_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::cargo_commodity>();
      Commodity const& comm      = alt.comm;
      Commodity        corrected = comm;
      corrected.quantity         = std::min(
          corrected.quantity,
          active_unit.cargo().max_commodity_quantity_that_fits(
              comm.type ) );
      if( corrected.quantity == 0 ) return nothing;
      return HarborDraggableObject::cargo_commodity{
          .comm = corrected, .slot = alt.slot };
    }
  }
}

wait<> HarborCargo::drop( HarborDraggableObject_t const& o,
                          Coord const& where ) {
  UNWRAP_CHECK( slot, slot_under_cursor( where ) );
  UNWRAP_CHECK( active_unit_id, get_active_unit() );
  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto const&  alt = o.get<HarborDraggableObject::unit>();
      UnitId const dragged_id = alt.id;
      ss_.units.change_to_cargo_somewhere( active_unit_id,
                                           dragged_id, slot );
      break;
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo( ss_.units, comm, active_unit_id,
                              slot,
                              /*try_other_slots=*/true );
      break;
    }
    case HarborDraggableObject::e::cargo_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::cargo_commodity>();
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo( ss_.units, comm, active_unit_id,
                              slot,
                              /*try_other_slots=*/true );
    }
  }
  co_return;
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
  for( Rect const rect : gfx::subrects( r, g_tile_delta ) )
    painter.draw_empty_rect( rect,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::white() );
  for( Rect const rect : gfx::subrects( r, g_tile_delta ) )
    painter.draw_solid_rect( rect, gfx::pixel::white() );
  maybe<UnitId> active_unit = get_active_unit();
  if( !active_unit.has_value() ) return;

  // Draw the contents of the cargo since we have a selected ship
  // in port.
  auto&       unit        = ss_.units.unit_for( *active_unit );
  auto const& cargo_slots = unit.cargo().slots();
  base::generator<Rect> grid = gfx::subrects( r, g_tile_delta );
  auto zipped = rl::zip( rl::ints(), cargo_slots, grid );
  for( auto const [idx, cargo_slot, rect] : zipped ) {
    painter.draw_solid_rect(
        rect.with_inc_size().edges_removed(),
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
                           UnitRenderOptions{} );
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

PositionedHarborSubView<HarborCargo> HarborCargo::create(
    SS& ss, TS& ts, Player& player, Rect canvas ) {
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
  HarborCargo* p_actual = view.get();
  return PositionedHarborSubView<HarborCargo>{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view,
      .actual = p_actual };
}

HarborCargo::HarborCargo( SS& ss, TS& ts, Player& player )
  : HarborSubView( ss, ts, player ) {}

} // namespace rn
