/****************************************************************
**harbor-view-inbound.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Inbound ships UI element within the harbor view.
*
*****************************************************************/
#include "harbor-view-inbound.hpp"

// Revolution Now
#include "co-wait.hpp"
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
** HarborInboundShips
*****************************************************************/
Delta HarborInboundShips::size_blocks( bool is_wide ) {
  static constexpr H height_blocks{ 3 };
  static constexpr W width_wide{ 3 };
  static constexpr W width_narrow{ 2 };
  return Delta{ .w = is_wide ? width_wide : width_narrow,
                .h = height_blocks };
}

// This is the size without the lower/right border.
Delta HarborInboundShips::size_pixels( bool is_wide ) {
  Delta res = size_blocks( is_wide );
  return res * g_tile_delta;
}

Delta HarborInboundShips::delta() const {
  Delta res = size_pixels( is_wide_ );
  // +1 in each dimension for the border.
  ++res.w;
  ++res.h;
  return res;
}

maybe<int> HarborInboundShips::entity() const {
  return static_cast<int>( e_harbor_view_entity::inbound );
}

ui::View& HarborInboundShips::view() noexcept { return *this; }

ui::View const& HarborInboundShips::view() const noexcept {
  return *this;
}

maybe<UnitId> HarborInboundShips::get_active_unit() const {
  return player_.old_world.harbor_state.selected_unit;
}

void HarborInboundShips::set_active_unit( UnitId unit_id ) {
  CHECK( as_const( ss_.units )
             .ownership_of( unit_id )
             .holds<UnitOwnership::harbor>() );
  player_.old_world.harbor_state.selected_unit = unit_id;
}

maybe<HarborInboundShips::UnitWithPosition>
HarborInboundShips::unit_at_location( Coord where ) const {
  for( auto [id, coord] : units( Coord{} ) ) {
    Rect const r = Rect::from( coord, g_tile_delta );
    if( where.is_inside( r ) )
      return UnitWithPosition{ .id = id, .pixel_coord = coord };
  }
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject_t>>
HarborInboundShips::object_here( Coord const& where ) const {
  maybe<UnitWithPosition> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  return DraggableObjectWithBounds<HarborDraggableObject_t>{
      .obj    = HarborDraggableObject::unit{ .id = unit->id },
      .bounds = Rect::from( unit->pixel_coord, g_tile_delta ) };
}

vector<HarborInboundShips::UnitWithPosition>
HarborInboundShips::units( Coord origin ) const {
  vector<UnitWithPosition> units;
  Rect const               r = rect( origin );
  Coord coord                = r.lower_right() - g_tile_delta;
  for( UnitId id :
       harbor_units_inbound( ss_.units, player_.nation ) ) {
    units.push_back( { .id = id, .pixel_coord = coord } );
    coord -= Delta{ .w = g_tile_delta.w };
    if( coord.x < r.left_edge() )
      coord = Coord{ ( r.lower_right() - g_tile_delta ).x,
                     coord.y - g_tile_delta.h };
  }
  return units;
}

wait<> HarborInboundShips::click_on_unit( UnitId unit_id ) {
  if( get_active_unit() == unit_id ) {
    Unit const&  unit = ss_.units.unit_for( unit_id );
    ChoiceConfig config{
        .msg = fmt::format(
            "European harbor options for [{}]:",
            unit.desc().name ),
        .options = {},
        .sort    = false,
    };
    config.options.push_back(
        { .key          = "sail to new world",
          .display_name = "Sail back to the New World." } );
    config.options.push_back(
        { .key = "no changes", .display_name = "No Changes." } );

    maybe<string> choice =
        co_await ts_.gui.optional_choice( config );
    if( !choice.has_value() ) co_return;
    if( choice == "sail to new world" ) {
      unit_sail_to_new_world( ss_.terrain, ss_.units, player_,
                              unit_id );
      co_return;
    }
  }
  set_active_unit( unit_id );
  co_return;
}

wait<> HarborInboundShips::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( rect( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborInboundShips::try_drag(
    HarborDraggableObject_t const& o, Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  dragging_ = Draggable{ .unit_id = unit.id };
  return true;
}

void HarborInboundShips::cancel_drag() { dragging_ = nothing; }

wait<> HarborInboundShips::disown_dragged_object() {
  // Ideally we should do as the API spec says and disown the ob-
  // ject here. However, we're not actually going to do that, be-
  // cause the object is a ship which is being dragged either to
  // the outbound or in-port boxes, and if we disown it first
  // then it will lose its existing harbor state. In any case, we
  // don't have to disown it since the methods used to move it to
  // its new home will do that automatically.
  co_return;
}

maybe<HarborDraggableObject_t> HarborInboundShips::can_receive(
    HarborDraggableObject_t const& a, int from_entity,
    Coord const& ) const {
  CONVERT_ENTITY( entity_enum, from_entity );
  if( entity_enum == e_harbor_view_entity::outbound ) return a;
  return nothing;
}

wait<> HarborInboundShips::drop(
    HarborDraggableObject_t const& o, Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  UnitId const dragged_id = unit.id;
  unit_sail_to_harbor( ss_.terrain, ss_.units, player_,
                       dragged_id );
  co_return;
}

void HarborInboundShips::draw( rr::Renderer& renderer,
                               Coord         coord ) const {
  rr::Painter painter = renderer.painter();
  auto        r       = rect( coord );
  painter.draw_empty_rect( r, rr::Painter::e_border_mode::inside,
                           gfx::pixel::white() );
  rr::Typer typer =
      renderer.typer( r.upper_left() + Delta{ .w = 2, .h = 2 },
                      gfx::pixel::white() );
  typer.write( "Inbound" );

  HarborState const& hb_state = player_.old_world.harbor_state;

  for( auto const& [unit_id, unit_coord] : units( coord ) ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    render_unit( renderer, unit_coord,
                 ss_.units.unit_for( unit_id ),
                 UnitRenderOptions{} );
    if( hb_state.selected_unit == unit_id )
      painter.draw_empty_rect(
          Rect::from( unit_coord, g_tile_delta ) -
              Delta{ .w = 1, .h = 1 },
          rr::Painter::e_border_mode::in_out,
          gfx::pixel::green() );
  }
}

PositionedHarborSubView<HarborInboundShips>
HarborInboundShips::create(
    SS& ss, TS& ts, Player& player, Rect,
    HarborMarketCommodities const& market_commodities,
    Coord                          harbor_outbound_upper_left ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborInboundShips> view;
  HarborSubView*                 harbor_sub_view = nullptr;

  bool const  is_wide = !market_commodities.stacked();
  Delta const size    = size_pixels( is_wide );
  Coord const pos =
      harbor_outbound_upper_left - Delta{ .w = size.w };

  view =
      make_unique<HarborInboundShips>( ss, ts, player, is_wide );
  harbor_sub_view              = view.get();
  HarborInboundShips* p_actual = view.get();
  return PositionedHarborSubView<HarborInboundShips>{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view,
      .actual = p_actual };
}

HarborInboundShips::HarborInboundShips( SS& ss, TS& ts,
                                        Player& player,
                                        bool    is_wide )
  : HarborSubView( ss, ts, player ), is_wide_( is_wide ) {}

} // namespace rn
