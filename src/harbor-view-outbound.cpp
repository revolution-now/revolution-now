/****************************************************************
**harbor-view-outbound.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-09-10.
*
* Description: Outbound ships UI element within the harbor view.
*
*****************************************************************/
#include "harbor-view-outbound.hpp"

// Revolution Now
#include "co-wait.hpp"
#include "harbor-units.hpp"
#include "harbor-view-inport.hpp"
#include "harbor-view-ships.hpp"
#include "igui.hpp"
#include "render.hpp"
#include "tiles.hpp"
#include "ts.hpp"

// config
#include "config/nation.rds.hpp"
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
** HarborOutboundShips
*****************************************************************/
Delta HarborOutboundShips::delta() const {
  return layout_.view.size;
}

maybe<int> HarborOutboundShips::entity() const {
  return static_cast<int>( e_harbor_view_entity::outbound );
}

ui::View& HarborOutboundShips::view() noexcept { return *this; }

ui::View const& HarborOutboundShips::view() const noexcept {
  return *this;
}

maybe<UnitId> HarborOutboundShips::get_active_unit() const {
  return player_.old_world.harbor_state.selected_unit;
}

void HarborOutboundShips::set_active_unit( UnitId unit_id ) {
  CHECK( as_const( ss_.units )
             .ownership_of( unit_id )
             .holds<UnitOwnership::harbor>() );
  player_.old_world.harbor_state.selected_unit = unit_id;
}

maybe<HarborOutboundShips::UnitWithPosition>
HarborOutboundShips::unit_at_location( Coord where ) const {
  for( auto [id, r] : units() )
    if( where.is_inside( r ) )
      return UnitWithPosition{ .id = id, .bounds = r };
  return nothing;
}

maybe<DraggableObjectWithBounds<HarborDraggableObject>>
HarborOutboundShips::object_here( Coord const& where ) const {
  maybe<UnitWithPosition> const unit = unit_at_location( where );
  if( !unit.has_value() ) return nothing;
  point const origin =
      gfx::centered_in( g_tile_delta, unit->bounds );
  return DraggableObjectWithBounds<HarborDraggableObject>{
    .obj    = HarborDraggableObject::unit{ .id = unit->id },
    .bounds = rect{ .origin = origin, .size = g_tile_delta } };
}

vector<HarborOutboundShips::UnitWithPosition>
HarborOutboundShips::units() const {
  vector<UnitWithPosition> units;
  auto spot_it = layout_.slots.begin();
  for( UnitId const unit_id :
       harbor_units_outbound( ss_.units, player_.nation ) ) {
    if( spot_it == layout_.slots.end() ) break;
    units.push_back( { .id = unit_id, .bounds = *spot_it } );
    ++spot_it;
  }
  return units;
}

point HarborOutboundShips::frame_nw() const {
  return layout_.view.nw();
}

wait<> HarborOutboundShips::click_on_unit( UnitId unit_id ) {
  if( get_active_unit() == unit_id ) {
    Unit const& unit = ss_.units.unit_for( unit_id );
    ChoiceConfig config{
      .msg = fmt::format( "European harbor options for [{}]:",
                          unit.desc().name ),
      .options = {},
      .sort    = false,
    };
    config.options.push_back(
        { .key          = "sail to port",
          .display_name = "Sail back to the European port." } );
    config.options.push_back(
        { .key = "no changes", .display_name = "No Changes." } );

    maybe<string> choice =
        co_await ts_.gui.optional_choice( config );
    if( !choice.has_value() ) co_return;
    if( choice == "sail to port" ) {
      unit_sail_to_harbor( ss_, unit_id );
      co_return;
    }
  }
  set_active_unit( unit_id );
  co_return;
}

wait<> HarborOutboundShips::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  CHECK( event.pos.is_inside( bounds( {} ) ) );
  maybe<UnitWithPosition> const unit =
      unit_at_location( event.pos );
  if( !unit.has_value() ) co_return;
  co_await click_on_unit( unit->id );
}

bool HarborOutboundShips::try_drag(
    HarborDraggableObject const& o, Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  dragging_ = Draggable{ .unit_id = unit.id };
  return true;
}

void HarborOutboundShips::cancel_drag() { dragging_ = nothing; }

wait<> HarborOutboundShips::disown_dragged_object() {
  // Ideally we should do as the API spec says and disown the ob-
  // ject here. However, we're not actually going to do that, be-
  // cause the object is a ship which is being dragged either to
  // the inbound or in-port boxes, and if we disown it first then
  // it will lose its existing harbor state. In any case, we
  // don't have to disown it since the methods used to move it to
  // its new home will do that automatically.
  co_return;
}

maybe<CanReceiveDraggable<HarborDraggableObject>>
HarborOutboundShips::can_receive( HarborDraggableObject const& a,
                                  int from_entity,
                                  Coord const& ) const {
  CONVERT_ENTITY( entity_enum, from_entity );
  if( entity_enum == e_harbor_view_entity::inbound ||
      entity_enum == e_harbor_view_entity::in_port )
    return CanReceiveDraggable<HarborDraggableObject>::yes{
      .draggable = a };
  return nothing;
}

wait<> HarborOutboundShips::drop( HarborDraggableObject const& o,
                                  Coord const& ) {
  UNWRAP_CHECK( unit, o.get_if<HarborDraggableObject::unit>() );
  UnitId const dragged_id = unit.id;
  unit_sail_to_new_world( ss_, dragged_id );
  co_return;
}

void HarborOutboundShips::draw( rr::Renderer& renderer,
                                Coord coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      point( coord ).distance_from_origin().to_double() );
  renderer.painter().draw_empty_rect(
      rect{ .size = layout_.view.size },
      rr::Painter::e_border_mode::inside,
      pixel::white().with_alpha( 128 ) );

  HarborState const& hb_state = player_.old_world.harbor_state;

  string const label_line_1 = "Bound For";
  string label_line_2;
  string label_line_3;

  if( !layout_.compact ) {
    string const new_world_name =
        player_.new_world_name.has_value()
            ? *player_.new_world_name
            : config_nation.nations[player_.nation]
                  .new_world_name;
    label_line_2 = new_world_name;
  } else {
    // Just use "new world" here because we're in compact mode
    // which probably won't have enough horizontal space to dis-
    // play the name that the user chose.
    label_line_2 = "the";
    label_line_3 = "New World";
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

  auto const units = this->units();

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
    if( hb_state.selected_unit == unit_id )
      rr::draw_empty_rect_faded_corners(
          renderer,
          rect{ .origin = {}, .size = g_tile_delta } -
              size{ .w = 1, .h = 1 },
          config_ui.harbor.ship_select_box_color );
  }
}

HarborOutboundShips::Layout HarborOutboundShips::create_layout(
    rect const canvas, HarborInPortShips const& in_port_ships ) {
  Layout l;

  l.compact = canvas.size.w < 600;

  l.view.origin = in_port_ships.frame_nw() - size{ .w = 104 };
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

PositionedHarborSubView<HarborOutboundShips>
HarborOutboundShips::create(
    SS& ss, TS& ts, Player& player, Rect const canvas,
    HarborInPortShips const& in_port_ships ) {
  Layout layout = create_layout( canvas, in_port_ships );
  auto view =
      make_unique<HarborOutboundShips>( ss, ts, player, layout );
  HarborSubView* const harbor_sub_view = view.get();
  HarborOutboundShips* const p_actual  = view.get();
  return PositionedHarborSubView<HarborOutboundShips>{
    .owned  = { .view  = std::move( view ),
                .coord = layout.view.origin },
    .harbor = harbor_sub_view,
    .actual = p_actual };
}

HarborOutboundShips::HarborOutboundShips( SS& ss, TS& ts,
                                          Player& player,
                                          Layout layout )
  : HarborSubView( ss, ts, player ), layout_( layout ) {}

} // namespace rn
