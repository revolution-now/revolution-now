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
#include "harbor-view-status.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"

// config
#include "config/text.rds.hpp"
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/cargo.hpp"
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/unit.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// rds
#include "rds/switch-macro.hpp"

// base
#include "base/range-lite.hpp"

using namespace std;

namespace rl = base::rl;

namespace rn {

namespace {

using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;

} // namespace

/****************************************************************
** HarborCargo
*****************************************************************/
Delta HarborCargo::delta() const { return layout_.view.size; }

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

maybe<HarborDraggableObject>
HarborCargo::draggable_in_cargo_slot( int slot ) const {
  maybe<UnitId> const active_unit = get_active_unit();
  Unit const& unit = ss_.units.unit_for( *active_unit );
  if( slot >= unit.cargo().slots_total() ) return nothing;
  CargoSlot const& cargo = unit.cargo()[slot];
  switch( cargo.to_enum() ) {
    case CargoSlot::e::empty:
      return nothing;
    case CargoSlot::e::overflow:
      return nothing;
    case CargoSlot::e::cargo: {
      Cargo const& draggable =
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
  Unit const& unit      = ss_.units.unit_for( *active_unit );
  int const slots_total = unit.cargo().slots_total();
  CHECK_LE( slots_total, 6 );
  for( int i = 0; i < slots_total; ++i )
    if( where.is_inside( layout_.slot_drag_boxes[i] ) ) //
      return i;
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborCargo::object_here( Coord const& where ) const {
  UNWRAP_RETURN( slot, slot_under_cursor( where ) );
  maybe<HarborDraggableObject> const draggable =
      draggable_in_cargo_slot( slot );
  if( !draggable.has_value() ) return nothing;
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj = *draggable, .bounds = layout_.slots[slot] };
}

void HarborCargo::clear_status_bar_msg() const {
  harbor_status_bar_.inject_message(
      HarborStatusMsg::default_msg{} );
}

void HarborCargo::send_sell_info_to_status_bar(
    e_commodity const comm ) {
  CommodityPrice const price = market_price( player_, comm );
  string const comm_name =
      uppercase_commodity_display_name( comm );
  string const msg = fmt::format(
      "Selling {} at {}{}", comm_name, price.bid * 100,
      config_text.special_chars.currency );
  harbor_status_bar_.inject_message(
      HarborStatusMsg::sticky_override{ .msg = msg } );
}

bool HarborCargo::try_drag( HarborDraggableObject const& o,
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
      e_commodity const type =
          o.get<HarborDraggableObject::cargo_commodity>()
              .comm.type;
      dragging_ =
          Draggable{ .slot = slot, .quantity = quantity };
      send_sell_info_to_status_bar( type );
      break;
    }
  }
  return true;
}

void HarborCargo::cancel_drag() {
  dragging_ = nothing;
  clear_status_bar_msg();
}

wait<maybe<HarborDraggableObject>>
HarborCargo::user_edit_object() const {
  UNWRAP_CHECK( slot, dragging_.member( &Draggable::slot ) );
  UNWRAP_CHECK( draggable, draggable_in_cargo_slot( slot ) );
  auto cargo_commodity =
      draggable.get_if<HarborDraggableObject::cargo_commodity>();
  if( !cargo_commodity.has_value() ) co_return nothing;
  Commodity const& comm = cargo_commodity->comm;
  int const max_allowed = comm.quantity;
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
  CargoSlot const& held   = active_unit.cargo()[slot];
  switch( held.to_enum() ) {
    case CargoSlot::e::empty:
    case CargoSlot::e::overflow: //
      SHOULD_NOT_BE_HERE;
    case CargoSlot::e::cargo: {
      Cargo const& cargo = held.get<CargoSlot::cargo>().contents;
      switch( cargo.to_enum() ) {
        case Cargo::e::unit: {
          UnitId const held_id = cargo.get<Cargo::unit>().id;
          UnitOwnershipChanger( ss_, held_id ).change_to_free();
          break;
        }
        case Cargo::e::commodity: {
          // We might be moving a partial amount of the commodity
          // in this slot. So remove all of it, then restore the
          // part that is left, if any.
          UNWRAP_CHECK( quantity, dragging_.maybe_member(
                                      &Draggable::quantity ) );
          Commodity const removed = rm_commodity_from_cargo(
              ss_.units,
              ss_.units.unit_for( active_unit_id ).cargo(),
              slot );
          Commodity to_add = removed;
          to_add.quantity -= quantity;
          CHECK_GE( to_add.quantity, 0 );
          if( to_add.quantity > 0 )
            add_commodity_to_cargo(
                ss_.units, to_add,
                ss_.units.unit_for( active_unit_id ).cargo(),
                slot,
                /*try_other_slots=*/false );
          break;
        }
      }
    }
  }
  co_return;
}

maybe<CanReceiveDraggable<HarborDraggableObject>>
HarborCargo::can_receive( HarborDraggableObject const& o,
                          int from_entity,
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
      auto const& alt = o.get<HarborDraggableObject::unit>();
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
      return CanReceiveDraggable<HarborDraggableObject>::yes{
        .draggable = alt };
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      Commodity const& comm = alt.comm;
      Commodity corrected   = comm;
      corrected.quantity    = std::min(
          corrected.quantity,
          active_unit.cargo().max_commodity_quantity_that_fits(
              comm.type ) );
      if( corrected.quantity == 0 ) return nothing;
      return CanReceiveDraggable<HarborDraggableObject>::yes{
        .draggable = HarborDraggableObject::market_commodity{
          .comm = corrected } };
    }
    case HarborDraggableObject::e::cargo_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::cargo_commodity>();
      Commodity const& comm = alt.comm;
      Commodity corrected   = comm;
      corrected.quantity    = std::min(
          corrected.quantity,
          active_unit.cargo().max_commodity_quantity_that_fits(
              comm.type ) );
      if( corrected.quantity == 0 ) return nothing;
      return CanReceiveDraggable<HarborDraggableObject>::yes{
        .draggable = HarborDraggableObject::cargo_commodity{
          .comm = corrected, .slot = alt.slot } };
    }
  }
}

wait<> HarborCargo::drop( HarborDraggableObject const& o,
                          Coord const& where ) {
  UNWRAP_CHECK( slot, slot_under_cursor( where ) );
  UNWRAP_CHECK( active_unit_id, get_active_unit() );
  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto const& alt = o.get<HarborDraggableObject::unit>();
      UnitId const dragged_id = alt.id;
      UnitOwnershipChanger( ss_, dragged_id )
          .change_to_cargo( active_unit_id, slot );
      break;
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo(
          ss_.units, comm,
          ss_.units.unit_for( active_unit_id ).cargo(), slot,
          /*try_other_slots=*/true );
      break;
    }
    case HarborDraggableObject::e::cargo_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::cargo_commodity>();
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo(
          ss_.units, comm,
          ss_.units.unit_for( active_unit_id ).cargo(), slot,
          /*try_other_slots=*/true );
    }
  }
  co_return;
}

void HarborCargo::draw( rr::Renderer& renderer,
                        Coord coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      point( coord ).distance_from_origin().to_double() );
  render_sprite( renderer, layout_.cargohold_nw,
                 e_tile::harbor_cargo_hold );

  maybe<UnitId> active_unit = get_active_unit();
  if( !active_unit.has_value() ) {
    for( int i = 0; i < 6; ++i )
      render_sprite( renderer, layout_.slots[i].origin,
                     e_tile::harbor_cargo_front );
    return;
  }
  bool const in_port =
      is_unit_in_port( ss_.units, *active_unit );

  // Draw the contents of the cargo since we have a selected ship
  // in port.
  auto& unit              = ss_.units.unit_for( *active_unit );
  auto const& cargo_slots = unit.cargo().slots();
  auto zipped =
      rl::zip( rl::ints(), layout_.slots, cargo_slots );
  // Need to draw any overflow tiles before the cargo contents,
  // so run through those first.
  for( auto const [idx, slot_rect, slot] : zipped ) {
    SWITCH( slot ) {
      CASE( overflow ) {
        render_sprite( renderer, layout_.left_wall[idx].nw(),
                       e_tile::harbor_cargo_overflow );
        break;
      }
      default:
        break;
    }
  }
  for( auto const [idx, slot_rect, slot] : zipped ) {
    if( dragging_.has_value() && dragging_->slot == idx )
      continue;
    SWITCH( slot ) {
      CASE( empty ) { break; }
      CASE( overflow ) { break; }
      CASE( cargo ) {
        SWITCH( cargo.contents ) {
          CASE( unit ) {
            // Non-ship units tend to have some space at the top
            // because they are all grounded, i.e. feet/wheels
            // touching the ground, i.e. they are not centered
            // within their 32x32 tiles, so we'll bump them up-
            // wards a bit which seems to look better.
            render_unit( renderer, slot_rect.origin,
                         ss_.units.unit_for( unit.id ),
                         UnitRenderOptions{} );
            break;
          }
          CASE( commodity ) {
            {
              SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .25 );
              render_commodity_20_outline(
                  renderer,
                  slot_rect.origin +
                      kLabeledCommodity20CargoRenderOffset,
                  commodity.obj.type, pixel::banana() );
            }
            Commodity const& comm = commodity.obj;
            bool const full       = ( comm.quantity == 100 );
            using enum e_commodity_label_render_colors;
            render_commodity_annotated_20(
                renderer,
                slot_rect.origin +
                    kLabeledCommodity20CargoRenderOffset,
                comm.type,
                CommodityRenderStyle{
                  .label =
                      CommodityLabel::quantity{
                        .value  = comm.quantity,
                        .colors = full ? harbor_cargo_100
                                       : harbor_cargo },
                  .dulled = !full } );
            break;
          }
        }
        break;
      }
    }
    if( !in_port ) {
      {
        SCOPED_RENDERER_MOD_MUL( painter_mods.alpha, .5 );
        render_sprite( renderer, layout_.slots[idx].origin,
                       e_tile::harbor_cargo_front );
      }
      render_sprite( renderer, layout_.slots[idx].origin,
                     e_tile::harbor_cargo_lock );
    }
  }
  for( int i = cargo_slots.size(); i < 6; ++i )
    render_sprite( renderer, layout_.slots[i].origin,
                   e_tile::harbor_cargo_front );
}

HarborCargo::Layout HarborCargo::create_layout(
    gfx::rect const canvas ) {
  Layout l;
  Delta const size_pixels =
      sprite_size( e_tile::harbor_cargo_hold );
  l.view.origin  = { .x = canvas.center().x - size_pixels.w / 2,
                     .y = canvas.bottom() - 106 };
  l.view.size    = size_pixels;
  l.cargohold_nw = {};
  l.slots[0].origin = point{ .x = 1, .y = 6 };
  l.slots[0].size   = { .w = 32, .h = 32 };
  l.left_wall[0]    = {};

  l.slots[1] = l.slots[0];
  l.slots[1].origin.x += 34;
  l.left_wall[1] = { .origin = { .x = 32, .y = 6 },
                     .size   = { .w = 10, .h = 32 } };

  l.slots[2] = l.slots[1];
  l.slots[2].origin.x += 34;
  l.left_wall[2] = { .origin = { .x = 66, .y = 6 },
                     .size   = { .w = 10, .h = 32 } };

  l.slots[3] = l.slots[2];
  l.slots[3].origin.x += 34;
  l.left_wall[3] = { .origin = { .x = 100, .y = 6 },
                     .size   = { .w = 10, .h = 32 } };

  l.slots[4] = l.slots[3];
  l.slots[4].origin.x += 34;
  l.left_wall[4] = { .origin = { .x = 131, .y = 6 },
                     .size   = { .w = 10, .h = 32 } };

  l.slots[5] = l.slots[4];
  l.slots[5].origin.x += 34;
  l.left_wall[5] = { .origin = { .x = 162, .y = 6 },
                     .size   = { .w = 10, .h = 32 } };

  // Expand the slots' bounding regions by one pixel so that when
  // the mouse is over a divider it can still accept drags. Oth-
  // erwise the behavior might seem a bit inconsistent or con-
  // fusing to a new player.
  l.drag_box_buffer = 1;
  l.slot_drag_boxes = l.slots;
  for( auto& slot : l.slot_drag_boxes )
    slot = slot.with_border_added( l.drag_box_buffer );
  return l;
}

PositionedHarborSubView<HarborCargo> HarborCargo::create(
    SS& ss, TS& ts, Player& player, rect const canvas,
    HarborStatusBar& harbor_status_bar ) {
  Layout const layout = create_layout( canvas );

  unique_ptr<HarborCargo> view = make_unique<HarborCargo>(
      ss, ts, player, harbor_status_bar, layout );
  HarborSubView* harbor_sub_view = view.get();
  HarborCargo* p_actual          = view.get();
  return PositionedHarborSubView<HarborCargo>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.nw() },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborCargo::HarborCargo( SS& ss, TS& ts, Player& player,
                          HarborStatusBar& harbor_status_bar,
                          Layout const& layout )
  : HarborSubView( ss, ts, player ),
    harbor_status_bar_( harbor_status_bar ),
    layout_( layout ) {}

} // namespace rn
