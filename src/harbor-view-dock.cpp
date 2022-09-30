/****************************************************************
**harbor-view-dock.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Units on dock UI element within the harbor view.
*
*****************************************************************/
#include "harbor-view-dock.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "equip.hpp"
#include "harbor-units.hpp"
#include "harbor-view-backdrop.hpp"
#include "igui.hpp"
#include "market.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// base
#include "base/conv.hpp"

using namespace std;

namespace rn {

namespace {} // namespace

/****************************************************************
** HarborDockUnits
*****************************************************************/
Delta HarborDockUnits::size_blocks() const {
  return size_blocks_;
}

// This is the size without the lower/right border.
Delta HarborDockUnits::size_pixels() const {
  Delta res = size_blocks();
  return res * g_tile_delta;
}

Delta HarborDockUnits::delta() const { return size_pixels(); }

maybe<int> HarborDockUnits::entity() const {
  return static_cast<int>( e_harbor_view_entity::dock );
}

ui::View& HarborDockUnits::view() noexcept { return *this; }

ui::View const& HarborDockUnits::view() const noexcept {
  return *this;
}

maybe<HarborDockUnits::UnitWithPosition>
HarborDockUnits::unit_at_location( Coord where ) const {
  for( auto [id, coord] : units( Coord{} ) ) {
    Rect const r = Rect::from( coord, g_tile_delta );
    if( where.is_inside( r ) )
      return UnitWithPosition{ .id = id, .pixel_coord = coord };
  }
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject_t>>
HarborDockUnits::object_here( Coord const& where ) const {
  maybe<UnitWithPosition> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  return DraggableObjectWithBounds<HarborDraggableObject_t>{
      .obj    = HarborDraggableObject::unit{ .id = unit->id },
      .bounds = Rect::from( unit->pixel_coord, g_tile_delta ) };
}

vector<HarborDockUnits::UnitWithPosition> HarborDockUnits::units(
    Coord origin ) const {
  vector<UnitWithPosition> units;
  Rect const               r       = rect( origin );
  X const                  x_start = r.lower_left().x;
  Coord coord = r.lower_left() - Delta{ .h = g_tile_delta.h };
  for( UnitId id :
       harbor_units_on_dock( ss_.units, player_.nation ) ) {
    units.push_back( { .id = id, .pixel_coord = coord } );
    coord += Delta{ .w = g_tile_delta.w };
    if( coord.x + g_tile_delta.w >= r.right_edge() ) {
      coord.x = x_start;
      coord.y -= g_tile_delta.h;
    }
  }
  return units;
}

wait<> HarborDockUnits::click_on_unit( UnitId unit_id ) {
  Unit const&  unit = ss_.units.unit_for( unit_id );
  ChoiceConfig config{
      .msg = fmt::format( "European dock options for @[H]{}@[]:",
                          unit.desc().name ),
      .options = {},
      .sort    = false,
  };
  vector<EquipOption> const equip_opts =
      equip_options( ss_, player_, unit.composition() );
  for( int idx = 0; idx < int( equip_opts.size() ); ++idx ) {
    EquipOption const& equip_opt = equip_opts[idx];
    ChoiceConfigOption option{
        .key          = fmt::to_string( idx ),
        .display_name = equip_description( equip_opt ),
        .disabled     = !equip_opt.can_afford };
    config.options.push_back( std::move( option ) );
  }
  static string const kNoChangesKey = "no changes";
  config.options.push_back(
      { .key = kNoChangesKey, .display_name = "No Changes." } );

  maybe<string> choice =
      co_await ts_.gui.optional_choice( config );
  if( !choice.has_value() ) co_return;
  if( choice == kNoChangesKey ) co_return;
  // Assume that we have an index into the EquipOptions vector.
  UNWRAP_CHECK( chosen_idx, base::from_chars<int>( *choice ) );
  CHECK_GE( chosen_idx, 0 );
  CHECK_LT( chosen_idx, int( equip_opts.size() ) );
  // This will change the unit type.
  PriceChange const price_change = perform_equip_option(
      ss_, unit.id(), equip_opts[chosen_idx] );
  // Will only display something if there is a price change.
  co_await display_price_change_notification( ts_, player_,
                                              price_change );
}

wait<> HarborDockUnits::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( rect( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborDockUnits::try_drag( HarborDraggableObject_t const& o,
                                Coord const& ) {
  // This method will only be called if there was already an ob-
  // ject under the cursor, which for us means a unit, and units
  // can always be dragged.
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  dragging_ = Draggable{ .unit_id = unit.id };
  return true;
}

void HarborDockUnits::cancel_drag() { dragging_ = nothing; }

wait<> HarborDockUnits::disown_dragged_object() {
  UNWRAP_CHECK( unit_id,
                dragging_.member( &Draggable::unit_id ) );
  ss_.units.disown_unit( unit_id );
  co_return;
}

maybe<HarborDraggableObject_t> HarborDockUnits::can_receive(
    HarborDraggableObject_t const& o, int /*from_entity*/,
    Coord const& ) const {
  auto const& unit = o.get_if<HarborDraggableObject::unit>();
  if( !unit.has_value() ) return nothing;
  if( ss_.units.unit_for( unit->id ).desc().ship )
    return nothing;
  return o;
}

wait<> HarborDockUnits::drop( HarborDraggableObject_t const& o,
                              Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  unit_move_to_port( ss_.units, player_, unit.id );
  co_return;
}

void HarborDockUnits::draw( rr::Renderer& renderer,
                            Coord         coord ) const {
  for( auto const& [unit_id, unit_coord] : units( coord ) ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    render_unit( renderer, unit_coord,
                 ss_.units.unit_for( unit_id ),
                 UnitRenderOptions{ .flag = false } );
  }
}

PositionedHarborSubView<HarborDockUnits> HarborDockUnits::create(
    SS& ss, TS& ts, Player& player, Rect,
    HarborBackdrop const& backdrop ) {
  // The canvas will exclude the market commodities.
  unique_ptr<HarborDockUnits> view;
  HarborSubView*              harbor_sub_view = nullptr;

  HarborBackdrop::DockUnitsLayout const dock_layout =
      backdrop.dock_units_layout();
  int max_vertical_units =
      dock_layout.units_start_floor.y / g_tile_delta.h;
  Coord const pos =
      dock_layout.units_start_floor -
      Delta{ .h = max_vertical_units * g_tile_delta.h };
  Delta const size_blocks{
      .w = dock_layout.dock_length / g_tile_delta.w,
      .h = max_vertical_units };

  view            = make_unique<HarborDockUnits>( ss, ts, player,
                                       size_blocks );
  harbor_sub_view = view.get();
  HarborDockUnits* p_actual = view.get();
  return PositionedHarborSubView<HarborDockUnits>{
      .owned  = { .view = std::move( view ), .coord = pos },
      .harbor = harbor_sub_view,
      .actual = p_actual };
}

HarborDockUnits::HarborDockUnits( SS& ss, TS& ts, Player& player,
                                  Delta size_blocks )
  : HarborSubView( ss, ts, player ),
    size_blocks_( size_blocks ) {}

} // namespace rn
