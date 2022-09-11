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
#include "commodity.hpp"
#include "harbor-units.hpp"
#include "harbor-view-market.hpp"
#include "igui.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

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
    Unit const&  unit = ss_.units.unit_for( unit_id );
    ChoiceConfig config{
        .msg = fmt::format(
            "European harbor options for @[H]{}@[]:",
            unit.desc().name ),
        .options = {},
        .sort    = false,
    };
    config.options.push_back(
        { .key          = "set sail",
          .display_name = "Set sail for the New World." } );
    config.options.push_back(
        { .key = "no changes", .display_name = "No Changes." } );

    maybe<string> choice =
        co_await ts_.gui.optional_choice( config );
    if( !choice.has_value() ) co_return;
    if( choice == "set sail" ) {
      unit_sail_to_new_world( ss_.terrain, ss_.units, player_,
                              unit_id );
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
  CHECK( event.pos.is_inside( rect( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborInPortShips::try_drag( any const& a, Coord const& ) {
  UNWRAP_DRAGGABLE( o, a );
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  dragging_ = Draggable{ .unit_id = unit.id };
  return true;
}

void HarborInPortShips::cancel_drag() { dragging_ = nothing; }

void HarborInPortShips::disown_dragged_object() {
  // Ideally we should do as the API spec says and disown the ob-
  // ject here. However, we're not actually going to do that, be-
  // cause the object is a ship which is being dragged either to
  // the outbound or inbound boxes, and if we disown it first
  // then it will lose its existing harbor state. In any case, we
  // don't have to disown it since the methods used to move it to
  // its new home will do that automatically.
}

maybe<any> HarborInPortShips::can_receive(
    any const& a, int from_entity, Coord const& where ) const {
  UNWRAP_DRAGGABLE( o, a );
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
      return a;
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
      auto const&  alt = o.get<HarborDraggableObject::unit>();
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
      return alt;
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      Commodity const& comm      = alt.comm;
      Commodity        corrected = comm;
      corrected.quantity         = std::min(
          corrected.quantity,
          ship.cargo().max_commodity_quantity_that_fits(
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
          ship.cargo().max_commodity_quantity_that_fits(
              comm.type ) );
      if( corrected.quantity == 0 ) return nothing;
      return HarborDraggableObject::cargo_commodity{
          .comm = corrected, .slot = alt.slot };
    }
  }
}

void HarborInPortShips::drop( any const&   a,
                              Coord const& where ) {
  UNWRAP_DRAGGABLE( o, a );
  switch( o.to_enum() ) {
    case HarborDraggableObject::e::unit: {
      auto const&  alt = o.get<HarborDraggableObject::unit>();
      UnitId const dragged_id = alt.id;
      Unit const&  dragged    = ss_.units.unit_for( dragged_id );
      if( dragged.desc().ship ) {
        unit_sail_to_harbor( ss_.terrain, ss_.units, player_,
                             dragged_id );
      } else {
        UNWRAP_CHECK( unit_with_pos, unit_at_location( where ) );
        ss_.units.change_to_cargo_somewhere( unit_with_pos.id,
                                             dragged_id );
      }
      break;
    }
    case HarborDraggableObject::e::market_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::market_commodity>();
      UNWRAP_CHECK( unit_with_pos, unit_at_location( where ) );
      Unit const& ship = ss_.units.unit_for( unit_with_pos.id );
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo( ss_.units, comm, ship.id(),
                              /*slot=*/0,
                              /*try_other_slots=*/true );
      break;
    }
    case HarborDraggableObject::e::cargo_commodity: {
      auto const& alt =
          o.get<HarborDraggableObject::cargo_commodity>();
      UNWRAP_CHECK( unit_with_pos, unit_at_location( where ) );
      Unit const& ship = ss_.units.unit_for( unit_with_pos.id );
      Commodity const& comm = alt.comm;
      add_commodity_to_cargo( ss_.units, comm, ship.id(),
                              /*slot=*/0,
                              /*try_other_slots=*/true );
    }
  }
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

PositionedHarborSubView<HarborInPortShips>
HarborInPortShips::create(
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
  harbor_sub_view             = view.get();
  HarborInPortShips* p_actual = view.get();
  return PositionedHarborSubView<HarborInPortShips>{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view,
      .actual = p_actual };
}

HarborInPortShips::HarborInPortShips( SS& ss, TS& ts,
                                      Player& player,
                                      bool    is_wide )
  : HarborSubView( ss, ts, player ), is_wide_( is_wide ) {}

} // namespace rn
