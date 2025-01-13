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
#include "harbor-view-outbound.hpp"
#include "igui.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

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
** HarborInboundShips
*****************************************************************/
Delta HarborInboundShips::delta() const {
  return layout_.view.size;
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
  for( auto [id, r] : units() )
    if( where.is_inside( r ) )
      return UnitWithPosition{ .id = id, .bounds = r };
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborInboundShips::object_here( Coord const& where ) const {
  maybe<UnitWithPosition> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  point const origin =
      gfx::centered_in( g_tile_delta, unit->bounds );
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj    = HarborDraggableObject::unit{ .id = unit->id },
    .bounds = rect{ .origin = origin, .size = g_tile_delta } };
}

vector<HarborInboundShips::UnitWithPosition>
HarborInboundShips::units() const {
  vector<UnitWithPosition> units;
  auto spot_it = layout_.slots.begin();
  for( UnitId const unit_id :
       harbor_units_inbound( ss_.units, player_.nation ) ) {
    if( spot_it == layout_.slots.end() ) break;
    units.push_back( { .id = unit_id, .bounds = *spot_it } );
    ++spot_it;
  }
  return units;
}

point HarborInboundShips::frame_nw() const {
  return layout_.view.nw();
}

wait<> HarborInboundShips::click_on_unit( UnitId unit_id ) {
  if( get_active_unit() == unit_id ) {
    Unit const& unit = ss_.units.unit_for( unit_id );
    ChoiceConfig config{
      .msg = fmt::format( "European harbor options for [{}]:",
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
      unit_sail_to_new_world( ss_, unit_id );
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
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborInboundShips::try_drag(
    HarborDraggableObject const& o, Coord const& ) {
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

maybe<CanReceiveDraggable<HarborDraggableObject>>
HarborInboundShips::can_receive( HarborDraggableObject const& a,
                                 int from_entity,
                                 Coord const& ) const {
  CONVERT_ENTITY( entity_enum, from_entity );
  if( entity_enum == e_harbor_view_entity::outbound )
    return CanReceiveDraggable<HarborDraggableObject>::yes{
      .draggable = a };
  return nothing;
}

wait<> HarborInboundShips::drop( HarborDraggableObject const& o,
                                 Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  UnitId const dragged_id = unit.id;
  unit_sail_to_harbor( ss_, dragged_id );
  co_return;
}

void HarborInboundShips::draw( rr::Renderer& renderer,
                               Coord coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      point( coord ).distance_from_origin().to_double() );
  rr::Painter painter = renderer.painter();
  painter.draw_empty_rect( rect{ .size = layout_.view.size },
                           rr::Painter::e_border_mode::inside,
                           pixel::white().with_alpha( 128 ) );

  HarborState const& hb_state = player_.old_world.harbor_state;

  string label_line_1;
  string label_line_2;
  string label_line_3;

  if( !layout_.compact ) {
    label_line_1 = "Expected Soon";
  } else {
    label_line_1 = "Expected";
    label_line_2 = "Soon";
  }

  int const label_h =
      rr::rendered_text_line_size_pixels( "X" ).h;

  // This needs to be kept in sync with the other boxes.
  static auto const kTextColor =
      config_ui.dialog_text.normal.shaded( 2 );
  point center{ .x = layout_.view.size.w / 2,
                .y = layout_.label_top };
  center.y += label_h / 2;
  rr::write_centered( renderer, kTextColor, center,
                      label_line_1 );
  center.y += label_h;
  rr::write_centered( renderer, kTextColor, center,
                      label_line_2 );
  center.y += label_h;
  rr::write_centered( renderer, kTextColor, center,
                      label_line_3 );

  // Draw in reverse order so that the front rows (and highlight
  // boxes around them) will be on top of back rows.
  for( auto const& [unit_id, bounds] : rl::rall( units() ) ) {
    if( dragging_.has_value() && dragging_->unit_id == unit_id )
      continue;
    SCOPED_RENDERER_MOD_ADD(
        painter_mods.repos.translation2,
        bounds.nw().distance_from_origin().to_double() );
    SCOPED_RENDERER_MOD_MUL( painter_mods.repos.scale,
                             double( bounds.size.w ) / 32 );
    rr::Painter painter = renderer.painter();
    render_unit( renderer, point{},
                 ss_.units.unit_for( unit_id ),
                 UnitRenderOptions{} );
    if( hb_state.selected_unit == unit_id )
      painter.draw_empty_rect(
          rect{ .origin = {}, .size = g_tile_delta } -
              size{ .w = 1, .h = 1 },
          rr::Painter::e_border_mode::in_out, pixel::green() );
  }
}

HarborInboundShips::Layout HarborInboundShips::create_layout(
    rect const canvas,
    HarborOutboundShips const& outbound_ships ) {
  Layout l;

  l.compact = canvas.size.w < 600;

  l.view.origin = outbound_ships.frame_nw() - size{ .w = 104 };
  l.view.size   = { .w = 102, .h = 96 };
  if( l.compact ) {
    l.view.origin.x += 32;
    l.view.size.w -= 32;
  }

  // Relative to view origin.
  l.units_area =
      rect{ .origin = point{} + size{ .w = 3, .h = 40 },
            .size   = { .w = 96, .h = 56 } };
  if( l.compact ) l.units_area.size.w -= 32;

  l.label_top = 2;

  rect slot;
  array<int, 3> row_counts;

  if( l.compact )
    row_counts = { 2, 4, 8 };
  else
    row_counts = { 3, 6, 12 };

  // Row 1 (32x32).
  slot = { .origin = l.units_area.origin + size{ .h = 24 },
           .size   = { .w = 32, .h = 32 } };
  for( int i = 0; i < row_counts[0]; ++i ) {
    l.slots.push_back( slot );
    slot.origin.x += slot.size.w;
  }

  // Row 2 (16x16).
  slot = { .origin = l.units_area.origin + size{ .h = 8 },
           .size   = { .w = 16, .h = 16 } };
  for( int i = 0; i < row_counts[1]; ++i ) {
    l.slots.push_back( slot );
    slot.origin.x += slot.size.w;
  }

  // Row 3 (8x8).
  slot = { .origin = l.units_area.origin,
           .size   = { .w = 8, .h = 8 } };
  for( int i = 0; i < row_counts[2]; ++i ) {
    l.slots.push_back( slot );
    slot.origin.x += slot.size.w;
  }
  return l;
}

PositionedHarborSubView<HarborInboundShips>
HarborInboundShips::create(
    SS& ss, TS& ts, Player& player, Rect const canvas,
    HarborOutboundShips const& outbound_ships ) {
  Layout layout = create_layout( canvas, outbound_ships );
  auto view =
      make_unique<HarborInboundShips>( ss, ts, player, layout );
  HarborSubView* const harbor_sub_view = view.get();
  HarborInboundShips* const p_actual   = view.get();
  return PositionedHarborSubView<HarborInboundShips>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborInboundShips::HarborInboundShips( SS& ss, TS& ts,
                                        Player& player,
                                        Layout layout )
  : HarborSubView( ss, ts, player ), layout_( layout ) {}

} // namespace rn
