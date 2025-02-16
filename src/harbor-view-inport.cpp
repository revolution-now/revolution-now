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
#include "co-time.hpp"
#include "co-wait.hpp"
#include "commodity.hpp"
#include "damaged.hpp"
#include "harbor-units.hpp"
#include "harbor-view-backdrop.hpp"
#include "harbor-view-ships.hpp"
#include "igui.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"

// config
#include "config/ui.rds.hpp"
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// render
#include "render/extra.hpp"
#include "render/renderer.hpp"
#include "render/typer.hpp"

// gfx
#include "gfx/cartesian.hpp"

// base
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"

// C++ standard library
#include <bit>

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
** HarborInPortShips
*****************************************************************/
Delta HarborInPortShips::delta() const {
  return layout_.view.size;
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
  for( auto [id, r] : units() )
    if( where.is_inside( r ) )
      return UnitWithPosition{ .id = id, .bounds = r };
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborInPortShips::object_here( Coord const& where ) const {
  maybe<UnitWithPosition> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  point const origin =
      gfx::centered_in( g_tile_delta, unit->bounds );
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj    = HarborDraggableObject::unit{ .id = unit->id },
    .bounds = rect{ .origin = origin, .size = g_tile_delta } };
}

vector<HarborInPortShips::UnitWithPosition>
HarborInPortShips::units() const {
  vector<UnitWithPosition> units;
  auto spot_it = layout_.slots.begin();
  for( UnitId const unit_id :
       harbor_units_in_port( ss_.units, player_.nation ) ) {
    if( spot_it == layout_.slots.end() ) break;
    units.push_back( { .id = unit_id, .bounds = *spot_it } );
    ++spot_it;
  }
  return units;
}

point HarborInPortShips::frame_nw() const {
  return layout_.white_box.nw().origin_becomes_point(
      layout_.view.origin );
}

wait<> HarborInPortShips::click_on_unit( UnitId const unit_id ) {
  if( get_active_unit() == unit_id ) {
    Unit const& unit = ss_.units.unit_for( unit_id );
    if( auto damaged =
            unit.orders().get_if<unit_orders::damaged>();
        damaged.has_value() ) {
      co_await ts_.gui.message_box( ship_still_damaged_message(
          damaged->turns_until_repair ) );
      co_return;
    }
    ChoiceConfig config{
      .msg = fmt::format( "European harbor options for [{}]:",
                          unit.desc().name ),
      .options = {},
      .sort    = false,
    };
    config.options.push_back(
        { .key = "move", .display_name = "Move to front." } );
    config.options.push_back(
        { .key          = "set sail",
          .display_name = "Set sail for the New World." } );
    config.options.push_back(
        { .key          = "unload",
          .display_name = "Unload all cargo." } );
    config.options.push_back(
        { .key = "no changes", .display_name = "No Changes." } );

    maybe<string> choice =
        co_await ts_.gui.optional_choice( config );
    if( !choice.has_value() ) co_return;
    if( choice == "move" ) {
      ss_.units.bump_unit_ordering( unit_id );
      set_active_unit( unit_id );
      co_return;
    }
    if( choice == "unload" ) {
      co_await ts_.gui.message_box(
          "Auto-unload not implemented." );
      co_return;
    }
    if( choice == "set sail" ) {
      unit_sail_to_new_world( ss_, unit_id );
      if( harbor_units_in_port( ss_.units, player_.nation )
              .empty() ) {
        using namespace std::chrono_literals;
        // Small delay so that the user can see the ship moving
        // into the outbound box briefly before the screen
        // closes, otherwise things happen instantaneously and
        // might seem confusing to the player.
        co_await 200ms;
        throw harbor_view_exit_interrupt{};
      }
      try_select_in_port_ship( ss_.units, player_ );
      co_return;
    }
  }
  set_active_unit( unit_id );
  co_return;
}

wait<> HarborInPortShips::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborInPortShips::try_drag( HarborDraggableObject const& o,
                                  Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  dragging_ = Draggable{ .unit_id = unit.id };
  return true;
}

wait<base::valid_or<DragRejection>>
HarborInPortShips::source_check( HarborDraggableObject const&,
                                 Coord const ) {
  UNWRAP_CHECK( unit_id,
                dragging_.member( &Draggable::unit_id ) );
  Unit const& unit = ss_.units.unit_for( unit_id );
  if( auto damaged =
          unit.orders().get_if<unit_orders::damaged>();
      damaged.has_value() )
    // We return this in the result instead of showing it here so
    // that it will be displayed after the rubberbanding, which
    // looks a bit more natural.
    co_return DragRejection{
      .reason = ship_still_damaged_message(
          damaged->turns_until_repair ) };
  co_return base::valid;
}

void HarborInPortShips::cancel_drag() { dragging_ = nothing; }

wait<> HarborInPortShips::disown_dragged_object() {
  // Ideally we should do as the API spec says and disown the ob-
  // ject here. However, we're not actually going to do that, be-
  // cause the object is a ship which is being dragged either to
  // the outbound or inbound boxes, and if we disown it first
  // then it will lose its existing harbor state. In any case, we
  // don't have to disown it since the methods used to move it to
  // its new home will do that automatically.
  co_return;
}

maybe<CanReceiveDraggable<HarborDraggableObject>>
HarborInPortShips::can_receive( HarborDraggableObject const& o,
                                int from_entity,
                                Coord const& where ) const {
  CONVERT_ENTITY( entity_enum, from_entity );
  auto draggable_unit = o.get_if<HarborDraggableObject::unit>();
  if( draggable_unit.has_value() ) {
    Unit const& unit = ss_.units.unit_for( draggable_unit->id );
    if( unit.desc().ship ) {
      // We're dragging a ship to the in port box. It would be ok
      // to always accept this, since the right thing will happen
      // no matter what state the ship is in. But, for a good
      // user experience, we will reject drags coming from the
      // inbound box, that way they will rubberband back to the
      // inbound box (which such a ship must remain if it is
      // still to come to port); if we didn't reject it then the
      // drag would be accepted and we'd tell it to sail to port,
      // and the logic that handles that would just keep it in
      // the inbound box, causing the ship to visually snap back
      // abruptly to the inbound box. This way, it goes back
      // smoothly for a better player experience. We don't do the
      // same for the outbound box because dragging such outbound
      // ships to the in-port box is actually useful in that it
      // will cause them to be moved to the inbound box.
      if( entity_enum == e_harbor_view_entity::inbound )
        return nothing;
      return CanReceiveDraggable<HarborDraggableObject>::yes{
        .draggable = o };
    }
  }
  // At this point we're either not dragging a unit or we are but
  // it's not a ship. In that case we are dragging a colonist or
  // commodity and so we need to make sure that we're dragging
  // over top of a ship.
  maybe<UnitWithPosition> unit_with_pos =
      unit_at_location( where );
  if( !unit_with_pos.has_value() ) return nothing;
  // At this point we are dragging a colonist or commodity over
  // top of a ship that is in port. So see if there is room.
  Unit const& ship = ss_.units.unit_for( unit_with_pos->id );
  CHECK( ship.desc().ship );

  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto const& alt = o.get<HarborDraggableObject::unit>();
      UnitId const dragged_id = alt.id;
      if( get_active_unit() == ship.id() &&
          entity_enum == e_harbor_view_entity::cargo ) {
        // We're dragging from the active ship's cargo into it-
        // self, i.e., ship == active_unit.
        UNWRAP_CHECK( from_slot,
                      ship.cargo().find_unit( dragged_id ) );
        if( !ship.cargo().fits_somewhere_with_item_removed(
                ss_.units, Cargo::unit{ .id = dragged_id },
                from_slot ) )
          return nothing;
      } else {
        if( !ship.cargo().fits_somewhere(
                ss_.units, Cargo::unit{ .id = dragged_id } ) )
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
          ship.cargo().max_commodity_quantity_that_fits(
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
          ship.cargo().max_commodity_quantity_that_fits(
              comm.type ) );
      if( corrected.quantity == 0 ) return nothing;
      return CanReceiveDraggable<HarborDraggableObject>::yes{
        .draggable = HarborDraggableObject::cargo_commodity{
          .comm = corrected, .slot = alt.slot } };
    }
  }
}

wait<> HarborInPortShips::drop( HarborDraggableObject const& o,
                                Coord const& where ) {
  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto const& alt = o.get<HarborDraggableObject::unit>();
      UnitId const dragged_id = alt.id;
      Unit const& dragged     = ss_.units.unit_for( dragged_id );
      if( dragged.desc().ship ) {
        unit_sail_to_harbor( ss_, dragged_id );
      } else {
        UNWRAP_CHECK( unit_with_pos, unit_at_location( where ) );
        UnitId const holder_id = unit_with_pos.id;
        UnitOwnershipChanger( ss_, dragged_id )
            .change_to_cargo( holder_id, /*starting_slot=*/0 );
      }
      break;
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      UNWRAP_CHECK( unit_with_pos, unit_at_location( where ) );
      Unit& ship = ss_.units.unit_for( unit_with_pos.id );
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo( ss_.units, comm, ship.cargo(),
                              /*slot=*/0,
                              /*try_other_slots=*/true );
      break;
    }
    case HarborDraggableObject::e::cargo_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::cargo_commodity>();
      UNWRAP_CHECK( unit_with_pos, unit_at_location( where ) );
      Unit& ship = ss_.units.unit_for( unit_with_pos.id );
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo( ss_.units, comm, ship.cargo(),
                              /*slot=*/0,
                              /*try_other_slots=*/true );
    }
  }
  co_return;
}

void HarborInPortShips::draw( rr::Renderer& renderer,
                              Coord coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      point( coord ).distance_from_origin().to_double() );
  renderer.painter().draw_empty_rect(
      layout_.white_box, rr::Painter::e_border_mode::inside,
      pixel::white().with_alpha( 128 ) );

  HarborState const& hb_state = player_.old_world.harbor_state;

  auto const units = this->units();

  string const label =
      units.empty() ? "No Ships in Port"
      : !hb_state.selected_unit.has_value()
          ? "Loading Ship Cargo"
          : fmt::format(
                "Loading: {}",
                ss_.units.unit_for( *hb_state.selected_unit )
                    .desc()
                    .name );
  size const label_size =
      rr::rendered_text_line_size_pixels( label );
  point const label_nw =
      gfx::centered_at_top( label_size, layout_.label_area );
  // This needs to be kept in sync with the other boxes.
  static auto const kTextColor =
      config_ui.dialog_text.normal.shaded( 2 );
  rr::Typer typer = renderer.typer( label_nw, kTextColor );
  typer.write( label );

  point const mouse_pos = input::current_mouse_position()
                              .to_gfx()
                              .point_becomes_origin( coord );
  auto const hover_unit = [&]() -> maybe<UnitId> {
    if( dragging_.has_value() ) return nothing;
    for( auto const& [unit_id, bounds] : units )
      if( mouse_pos.is_inside( bounds ) ) //
        return unit_id;
    return nothing;
  }();

  // Draw in reverse order so that the front rows (and highlight
  // boxes around them) will be on top of back rows.
  for( auto const& [unit_id, bounds] : rl::rall( units ) ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation2,
        bounds.nw().distance_from_origin().to_double() );
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale,
                             double( bounds.size.w ) / 32 );
    // NOTE: can remove this once we can render to an offscreen
    // buffer to automatically downsample.
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.sampling.downsample,
        countr_zero( 32u ) -
            countr_zero( uint32_t( bounds.size.w ) ) );
    auto const& unit = ss_.units.unit_for( unit_id );
    if( unit_id == hover_unit ) {
      CHECK( bounds.size.w > 0 );
      render_unit_glow( renderer, point{}, unit.type(),
                        32 / bounds.size.w );
    }
    render_unit( renderer, point{}, unit, UnitRenderOptions{} );
  }

  // Draw the select box after all units so that it is never par-
  // tially covered by any unit.
  for( auto const& [unit_id, bounds] : rl::rall( units ) ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation2,
        bounds.nw().distance_from_origin().to_double() );
    if( hb_state.selected_unit == unit_id )
      rr::draw_empty_rect_faded_corners(
          renderer,
          rect{ .origin = {}, .size = bounds.size } -
              size{ .w = 1, .h = 1 },
          config_ui.harbor.ship_select_box_color );
  }
}

HarborInPortShips::Layout HarborInPortShips::create_layout(
    HarborBackdrop const& backdrop ) {
  Layout l;
  l.view.origin =
      backdrop.horizon_center() - size{ .w = 96, .h = 6 };
  l.view.size = { .w = 192, .h = 56 };

  // Relative to view origin.
  l.white_box =
      rect{ .origin = point{} - size{ .w = 9, .h = 40 },
            .size   = { .w = 210, .h = 134 } };

  l.label_area = l.white_box;
  l.label_area.origin.y += 2;

  // Slots. The counts in each row are chosen to conform to the
  // perspective of the scene.
  rect slot;

  // Row 1 (32x32).
  slot = { .origin = point{} + size{ .h = 24 },
           .size   = { .w = 32, .h = 32 } };
  for( int i = 0; i < 6; ++i ) {
    l.slots.push_back( slot );
    slot.origin.x += slot.size.w;
  }

  // Row 2 (16x16).
  slot = { .origin = point{ .x = 16 } + size{ .h = 8 },
           .size   = { .w = 16, .h = 16 } };
  int const row_2_extra = backdrop.extra_space_for_ships() / 16;
  for( int i = 0; i < std::min( 9 + row_2_extra, 9 ); ++i ) {
    l.slots.push_back( slot );
    slot.origin.x += slot.size.w;
  }

  // Row 3 (8x8).
  slot                  = { .origin = point{ .x = 24 },
                            .size   = { .w = 8, .h = 8 } };
  int const row_3_extra = backdrop.extra_space_for_ships() / 8;
  for( int i = 0; i < std::min( 9 + row_3_extra, 15 ); ++i ) {
    l.slots.push_back( slot );
    slot.origin.x += slot.size.w;
  }
  return l;
}

PositionedHarborSubView<HarborInPortShips>
HarborInPortShips::create( SS& ss, TS& ts, Player& player, Rect,
                           HarborBackdrop const& backdrop ) {
  Layout layout = create_layout( backdrop );
  auto view =
      make_unique<HarborInPortShips>( ss, ts, player, layout );
  HarborSubView* const harbor_sub_view = view.get();
  HarborInPortShips* const p_actual    = view.get();
  return PositionedHarborSubView<HarborInPortShips>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborInPortShips::HarborInPortShips( SS& ss, TS& ts,
                                      Player& player,
                                      Layout layout )
  : HarborSubView( ss, ts, player ),
    layout_( std::move( layout ) ) {}

} // namespace rn
