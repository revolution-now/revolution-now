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
#include "treasure.hpp"
#include "ts.hpp"
#include "unit-ownership.hpp"

// config
#include "config/unit-type.rds.hpp"

// ss
#include "ss/player.rds.hpp"
#include "ss/ref.hpp"
#include "ss/units.hpp"

// render
#include "render/renderer.hpp"

// base
#include "base/conv.hpp"
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
** HarborDockUnits
*****************************************************************/
Delta HarborDockUnits::delta() const {
  return layout_.view.size;
}

maybe<int> HarborDockUnits::entity() const {
  return static_cast<int>( e_harbor_view_entity::dock );
}

ui::View& HarborDockUnits::view() noexcept { return *this; }

ui::View const& HarborDockUnits::view() const noexcept {
  return *this;
}

void HarborDockUnits::update_units() {
  units_.clear();
  auto const& slots_layout = backdrop_.dock_units_layout();
  auto spot_it             = slots_layout.units.begin();
  for( UnitId const unit_id :
       harbor_units_on_dock( ss_.units, player_.nation ) ) {
    if( spot_it == slots_layout.units.end() ) break;
    units_.push_back( { .id     = unit_id,
                        .bounds = spot_it->point_becomes_origin(
                            layout_.view.origin ) } );
    ++spot_it;
  }
}

maybe<HarborDockUnits::UnitWithRect>
HarborDockUnits::unit_at_location( point const where ) const {
  for( auto [id, bounds] : rl::rall( units_ ) )
    if( where.is_inside( bounds ) )
      return UnitWithRect{ .id = id, .bounds = bounds };
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborDockUnits::object_here( Coord const& where ) const {
  maybe<UnitWithRect> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj    = HarborDraggableObject::unit{ .id = unit->id },
    .bounds = unit->bounds };
}

wait<> HarborDockUnits::click_on_unit( UnitId unit_id ) {
  Unit const& unit = ss_.units.unit_for( unit_id );
  ChoiceConfig config{
    .msg     = fmt::format( "European dock options for [{}]:",
                            unit.desc().name ),
    .options = {},
    .sort    = false,
  };
  vector<HarborEquipOption> const equip_opts =
      harbor_equip_options( ss_, player_, unit.composition() );
  for( int idx = 0; idx < int( equip_opts.size() ); ++idx ) {
    HarborEquipOption const& equip_opt = equip_opts[idx];
    ChoiceConfigOption option{
      .key          = fmt::to_string( idx ),
      .display_name = harbor_equip_description( equip_opt ),
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
  PriceChange const price_change = perform_harbor_equip_option(
      ss_, ts_, player_, unit.id(), equip_opts[chosen_idx] );
  // Will only display something if there is a price change.
  co_await display_price_change_notification( ts_, player_,
                                              price_change );
}

wait<> HarborDockUnits::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  maybe<UnitWithRect> const unit = unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborDockUnits::try_drag( HarborDraggableObject const& o,
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
  UnitOwnershipChanger( ss_, unit_id ).change_to_free();
  update_units();
  co_return;
}

maybe<CanReceiveDraggable<HarborDraggableObject>>
HarborDockUnits::can_receive( HarborDraggableObject const& o,
                              int /*from_entity*/,
                              Coord const& ) const {
  auto const& unit = o.get_if<HarborDraggableObject::unit>();
  if( !unit.has_value() ) return nothing;
  if( ss_.units.unit_for( unit->id ).desc().ship )
    return nothing;
  return CanReceiveDraggable<HarborDraggableObject>::yes{
    .draggable = o };
}

wait<> HarborDockUnits::drop( HarborDraggableObject const& o,
                              Coord const& ) {
  UNWRAP_CHECK( draggable_unit,
                o.get_if<HarborDraggableObject::unit>() );
  e_unit_type const type =
      ss_.units.unit_for( draggable_unit.id ).type();
  if( type == e_unit_type::treasure ) {
    TreasureReceipt const receipt = treasure_in_harbor_receipt(
        player_, ss_.units.unit_for( draggable_unit.id ) );
    apply_treasure_reimbursement( ss_, player_, receipt );
    co_await show_treasure_receipt( ts_, player_, receipt );
    // !! Note: the treasure unit is now destroyed!
    CHECK( !ss_.units.exists( draggable_unit.id ) );
    co_return;
  }
  unit_move_to_port( ss_, draggable_unit.id );
  update_units();
}

void HarborDockUnits::draw( rr::Renderer& renderer,
                            Coord const coord ) const {
  point const mouse_pos = input::current_mouse_position()
                              .to_gfx()
                              .point_becomes_origin( coord );
  // Because of how the units are layered on top of each other,
  // we need to iterate backwards to find the unit that is high-
  // lighted (if any) first before we start drawing, given that
  // we draw in the opposite order (first to last).
  auto const highlighted_unit = [&]() -> maybe<UnitId> {
    if( dragging_.has_value() ) return nothing;
    for( auto const& [unit_id, bounds] : rl::rall( units_ ) )
      if( mouse_pos.is_inside( bounds ) ) return unit_id;
    return nothing;
  }();

  for( auto const& [unit_id, bounds] : units_ ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation,
        point( coord ).distance_from_origin().to_double() );
    Unit const& unit  = ss_.units.unit_for( unit_id );
    e_tile const tile = unit.desc().tile;
    if( unit_id == highlighted_unit )
      render_sprite_silhouette(
          renderer, bounds.nw() - size{ .w = 1 }, tile,
          pixel::from_hex_rgb( 0xeeeeaa ) );
    render_sprite( renderer, bounds.nw(), tile );
  }
  // Must be done after units since they are supposed to appear
  // behind it.
  backdrop_.draw_dock_overlay( renderer, coord );
}

HarborDockUnits::Layout HarborDockUnits::create_layout(
    HarborBackdrop const& backdrop ) {
  Layout l;
  vector<rect> const& unit_slots =
      backdrop.dock_units_layout().units;
  CHECK( !unit_slots.empty() );
  rect composite = unit_slots[0];
  for( rect const& r : unit_slots )
    composite = composite.uni0n( r );
  l.view = composite;
  for( rect const& r : unit_slots )
    l.slots.push_back( rect{
      .origin = r.nw().point_becomes_origin( l.view.nw() ),
      .size   = g_tile_delta } );
  return l;
}

PositionedHarborSubView<HarborDockUnits> HarborDockUnits::create(
    SS& ss, TS& ts, Player& player, Rect const,
    HarborBackdrop const& backdrop ) {
  Layout layout = create_layout( backdrop );
  auto view     = make_unique<HarborDockUnits>( ss, ts, player,
                                                backdrop, layout );
  HarborSubView* const harbor_sub_view = view.get();
  HarborDockUnits* p_actual            = view.get();
  return PositionedHarborSubView<HarborDockUnits>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborDockUnits::HarborDockUnits( SS& ss, TS& ts, Player& player,
                                  HarborBackdrop const& backdrop,
                                  Layout layout )
  : HarborSubView( ss, ts, player ),
    backdrop_( backdrop ),
    layout_( std::move( layout ) ) {
  update_units();
}

} // namespace rn
