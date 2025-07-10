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
#include "revolution.rds.hpp"
#include "tiles.hpp"
#include "treasure.hpp"
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
#include "render/renderer.hpp"

// base
#include "base/conv.hpp"
#include "base/range-lite.hpp"
#include "base/scope-exit.hpp"

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

bool HarborDockUnits::update_units_impl( int const row_inc ) {
  units_.clear();
  auto const& units_layout = backdrop_.dock_units_layout();
  vector<UnitId> const dock_units =
      harbor_units_on_dock( ss_.units, player_.type );

  auto units_iter = dock_units.begin();

  auto const sizes_for_curr_unit = [&] {
    CHECK( units_iter != dock_units.end() );
    UnitId const unit_id = *units_iter;
    Unit const& unit     = ss_.units.unit_for( unit_id );
    e_tile const tile    = unit.desc().tile;
    rect const trimmed   = trimmed_area_for( tile );
    return pair{ trimmed, sprite_size( tile ) };
  };

  auto const place_unit = [&]( point const p ) {
    auto const [trimmed, sprite] = sizes_for_curr_unit();
    rect const trimmed_bounds =
        rect{
          .origin = p,
          .size = { .w = trimmed.size.w, .h = -trimmed.size.h } }
            .normalized()
            .point_becomes_origin( layout_.view.origin );
    size const delta = trimmed.origin.distance_from_origin();
    units_.push_back( UnitPlacement{
      .id             = *units_iter,
      .trimmed_bounds = trimmed_bounds,
      .sprite_bounds =
          rect{ .origin = trimmed_bounds.origin - delta,
                .size   = sprite } } );
    ++units_iter;
    return trimmed.size.w;
  };

  int const kUnitDockSpacing = 4;

  auto const place_along_line_right = [&]( point const start ) {
    point p = start;
    while( units_iter != dock_units.end() &&
           p.x < units_layout.right_edge ) {
      auto const [trimmed, _] = sizes_for_curr_unit();
      if( p.x + trimmed.size.w + kUnitDockSpacing >=
          units_layout.right_edge )
        break;
      p.x += place_unit( p ) + kUnitDockSpacing;
    }
  };

  auto const place_along_line_left = [&]( point const end ) {
    point p = end;
    p.x     = units_layout.right_edge;
    while( units_iter != dock_units.end() ) {
      auto const [trimmed, _] = sizes_for_curr_unit();
      p.x -= ( trimmed.size.w + kUnitDockSpacing );
      if( p.x < end.x ) break;
      place_unit( p );
    }
  };

  // Place units on the dock.
  place_along_line_right( units_layout.dock_row_start );

  // Place units on the hill.
  place_along_line_left( units_layout.hill_row_start );

  // Place units on the ground.
  auto rows_iter = units_layout.ground_rows.begin();
  bool move_left = true;
  while( true ) {
    if( rows_iter == units_layout.ground_rows.end() ) break;
    if( move_left )
      place_along_line_left( *rows_iter );
    else
      place_along_line_right( *rows_iter );
    move_left = !move_left;
    if( units_layout.ground_rows.end() - rows_iter < row_inc )
      break;
    rows_iter += row_inc;
  }

  return units_iter == dock_units.end();
}

void HarborDockUnits::update_units() {
  for( int row_inc = 12; row_inc > 0; --row_inc )
    if( update_units_impl( row_inc ) ) return;
}

maybe<HarborDockUnits::UnitPlacement>
HarborDockUnits::unit_at_location( point const where ) const {
  for( auto const& unit_with_bounds : rl::rall( units_ ) )
    if( where.is_inside( unit_with_bounds.trimmed_bounds ) )
      return unit_with_bounds;
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborDockUnits::object_here( Coord const& where ) const {
  maybe<UnitPlacement> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj    = HarborDraggableObject::unit{ .id = unit->id },
    .bounds = unit->sprite_bounds };
}

wait<> HarborDockUnits::click_on_unit( UnitId unit_id ) {
  SCOPE_EXIT { update_units(); };
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
  // Need to update the dock units before popping up a box so
  // that when the unit sprite changes the other units will be
  // repositioned accordingly.
  update_units();
  // Will only display something if there is a price change.
  co_await display_price_change_notification( player_, agent_,
                                              price_change );
}

wait<> HarborDockUnits::perform_click(
    input::mouse_button_event_t const& event ) {
  SCOPE_EXIT { update_units(); };
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  maybe<UnitPlacement> const unit =
      unit_at_location( event.pos );
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
  SCOPE_EXIT { update_units(); };
  UNWRAP_CHECK( unit_id,
                dragging_.member( &Draggable::unit_id ) );
  UnitOwnershipChanger( ss_, unit_id ).change_to_free();
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
  if( as_const( ss_.units )
          .ownership_of( unit->id )
          .holds<UnitOwnership::harbor>() )
    // Prevent dragging dock units onto the dock, since there is
    // no purpose in doing that and it would cause a reordering
    // of the units.
    return nothing;
  return CanReceiveDraggable<HarborDraggableObject>::yes{
    .draggable = o };
}

wait<> HarborDockUnits::drop( HarborDraggableObject const& o,
                              Coord const& ) {
  SCOPE_EXIT { update_units(); };
  UNWRAP_CHECK( draggable_unit,
                o.get_if<HarborDraggableObject::unit>() );
  e_unit_type const type =
      ss_.units.unit_for( draggable_unit.id ).type();
  if( type == e_unit_type::treasure ) {
    TreasureReceipt const receipt = treasure_in_harbor_receipt(
        ss_, player_, ss_.units.unit_for( draggable_unit.id ) );
    apply_treasure_reimbursement( ss_, player_, receipt );
    co_await show_treasure_receipt( player_, agent_, receipt );
    // !! Note: the treasure unit is now destroyed!
    CHECK( !ss_.units.exists( draggable_unit.id ) );
    co_return;
  }
  unit_move_to_port( ss_, draggable_unit.id );
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
  auto const highlighted_unit = [&]() -> maybe<UnitPlacement> {
    if( dragging_.has_value() ) return nothing;
    for( auto const& unit_with_bounds : rl::rall( units_ ) )
      if( mouse_pos.is_inside(
              unit_with_bounds.trimmed_bounds ) )
        return unit_with_bounds;
    return nothing;
  }();

  for( auto const& unit_with_bounds : units_ ) {
    if( dragging_.has_value() &&
        dragging_->unit_id == unit_with_bounds.id )
      continue;
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation2,
        point( coord ).distance_from_origin().to_double() );
    Unit const& unit = ss_.units.unit_for( unit_with_bounds.id );
    e_tile const tile = unit.desc().tile;
    if( highlighted_unit.has_value() &&
        unit_with_bounds.id == highlighted_unit->id )
      render_sprite_silhouette(
          renderer,
          unit_with_bounds.sprite_bounds.nw() - size{ .w = 1 },
          tile, config_ui.harbor.unit_highlight_color );
    render_sprite( renderer, unit_with_bounds.sprite_bounds.nw(),
                   tile );
  }

  // After independence is declared, draw a single REF unit at
  // the base of the dock.
  if( colonial_player_.revolution.status >=
      e_revolution_status::declared ) {
    auto const& dock_layout = backdrop_.dock_units_layout();
    point p{ .x = dock_layout.right_edge - 32,
             .y = dock_layout.dock_row_start.y - 32 };
    e_unit_type const guard = e_unit_type::regular;
    int const spacing       = trimmed_area_for_unit_type( guard )
                            .horizontal_slice()
                            .len +
                        2;
    for( int i = 0; i < 2; ++i ) {
      render_unit_type( renderer, p, guard,
                        UnitRenderOptions{} );
      p.x -= spacing;
    }
  }

  // Must be done after units since they are supposed to appear
  // behind it.
  backdrop_.draw_dock_overlay( renderer, coord );
}

HarborDockUnits::Layout HarborDockUnits::create_layout(
    HarborBackdrop const& backdrop ) {
  Layout l;
  auto const& dock_layout = backdrop.dock_units_layout();

  l.view = rect::from(
      point{ .x = dock_layout.dock_row_start.x,
             .y = dock_layout.dock_row_start.y - 32 },
      point{ .x = dock_layout.right_edge,
             .y = dock_layout.bottom_edge } );
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
