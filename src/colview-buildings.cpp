/****************************************************************
**colview-buildings.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Buildings view UI within the colony view.
*
*****************************************************************/
#include "colview-buildings.hpp"

// Revolution Now
#include "colony-buildings.hpp"
#include "custom-house.hpp"
#include "iengine.hpp"
#include "production.hpp"
#include "render.hpp"
#include "spread-builder.hpp"
#include "spread-render.hpp"
#include "teaching.hpp"
#include "tiles.hpp"
#include "unit-ownership.hpp"

// config
#include "config/tile-enum.rds.hpp"

// ss
#include "ss/colony.hpp"
#include "ss/ref.hpp"
#include "ss/settings.rds.hpp"
#include "ss/units.hpp"

// config
#include "config/colony.rds.hpp"

// render
#include "render/renderer.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

using ::base::NoDiscard;
using ::gfx::pixel;
using ::gfx::point;
using ::gfx::rect;
using ::gfx::size;
using ::refl::enum_values;

maybe<e_tile> tile_for_slot_20(
    e_colony_building_slot const slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return e_tile::commodity_muskets_20;
    case e_colony_building_slot::tools:
      return e_tile::commodity_tools_20;
    case e_colony_building_slot::rum:
      return e_tile::commodity_rum_20;
    case e_colony_building_slot::cloth:
      return e_tile::commodity_cloth_20;
    case e_colony_building_slot::coats:
      return e_tile::commodity_coats_20;
    case e_colony_building_slot::cigars:
      return e_tile::commodity_cigars_20;
    case e_colony_building_slot::hammers:
      return e_tile::product_hammers_20;
    case e_colony_building_slot::town_hall:
      return e_tile::product_bells_20;
    case e_colony_building_slot::newspapers:
      return nothing;
    case e_colony_building_slot::schools:
      return nothing;
    case e_colony_building_slot::offshore:
      return nothing;
    case e_colony_building_slot::horses:
      return e_tile::commodity_horses_20;
    case e_colony_building_slot::wall:
      return nothing;
    case e_colony_building_slot::warehouses:
      return nothing;
    case e_colony_building_slot::crosses:
      return e_tile::product_crosses_20;
    case e_colony_building_slot::custom_house:
      return nothing;
  }
}

TileSpreadRenderPlan create_production_spread(
    rr::ITextometer const& textometer, SSConst const& ss,
    ColonyProduction const& production,
    e_colony_building_slot const slot, int const width ) {
  auto const tile = tile_for_slot_20( slot );
  if( !tile.has_value() ) return {};
  auto const quantity = production_for_slot( production, slot );
  TileSpreadConfig const config{
    .tile = { .tile = *tile, .count = quantity.value_or( 0 ) },
    .options = {
      .bounds = width,
      .label_policy =
          ss.settings.colony_options.numbers
              ? SpreadLabels{ SpreadLabels::always{} }
              : SpreadLabels{ SpreadLabels::auto_decide{} },
      .label_opts =
          { .placement =
                SpreadLabelPlacement::left_middle_adjusted{} },
    } };
  return build_tile_spread( textometer, config );
}

TileSpreadRenderPlan create_workers_spread(
    rr::ITextometer const& textometer, SSConst const& ss,
    Colony const& colony, e_colony_building_slot const slot,
    int const width, int const unit_shadow_offset ) {
  TileSpreadRenderPlan res;
  maybe<e_indoor_job> const indoor_job =
      indoor_job_for_slot( slot );
  if( !indoor_job ) return res;
  vector<UnitId> const& units = colony.indoor_jobs[*indoor_job];
  vector<TileWithOptions> const tiles = [&] {
    vector<TileWithOptions> res;
    res.reserve( units.size() );
    for( UnitId const unit_id : units )
      res.push_back( TileWithOptions{
        .tile = tile_for_unit_type(
            ss.units.unit_for( unit_id ).type() ) } );
    return res;
  }();
  InhomogeneousTileSpreadConfig const config{
    .tiles = tiles,
    // Have the max spacing be enough so that, including the unit
    // shadows, there will still be one pixel of separation be-
    // tween them when they are at max spacing.
    .max_spacing = -unit_shadow_offset + 1,
    .options     = { .bounds       = width,
                     .label_policy = SpreadLabels::never{} } };
  res = build_inhomogeneous_tile_spread( textometer, config );
  return res;
}

} // namespace

/****************************************************************
** Buildings
*****************************************************************/
Rect ColViewBuildings::visible_rect_for_unit_in_slot(
    e_colony_building_slot const slot,
    int const unit_idx ) const {
  BuildingLayoutSlot const& slot_plan = layout_.slots[slot];
  TileSpreadRenderPlan const& workers_plan =
      slot_plan.workers_plan;
  CHECK_LT( unit_idx, ssize( workers_plan.tiles ) );
  TileRenderPlan const& tile_plan = workers_plan.tiles[unit_idx];
  rect const trimmed = trimmed_area_for( tile_plan.tile );
  return trimmed.origin_becomes_point( tile_plan.where )
      .origin_becomes_point( slot_plan.workers_plan_origin );
}

Rect ColViewBuildings::sprite_rect_for_unit_in_slot(
    e_colony_building_slot const slot,
    int const unit_idx ) const {
  BuildingLayoutSlot const& slot_plan = layout_.slots[slot];
  TileSpreadRenderPlan const& workers_plan =
      slot_plan.workers_plan;
  CHECK_LT( unit_idx, ssize( workers_plan.tiles ) );
  TileRenderPlan const& tile_plan = workers_plan.tiles[unit_idx];
  return rect{ .origin = tile_plan.where,
               .size   = sprite_size( tile_plan.tile ) }
      .origin_becomes_point( slot_plan.workers_plan_origin );
}

maybe<e_colony_building_slot> ColViewBuildings::slot_for_coord(
    Coord where ) const {
  for( e_colony_building_slot slot :
       refl::enum_values<e_colony_building_slot> )
    if( where.is_inside( layout_.slots[slot].bounds ) ) //
      return slot;
  return nothing;
}

void ColViewBuildings::draw_workers(
    rr::Renderer& renderer,
    e_colony_building_slot const slot ) const {
  TileSpreadRenderOptions const options = [&] {
    TileSpreadRenderOptions res{
      .shadow = TileSpreadRenderShadow{
        .offset = layout_.unit_shadow_offset,
        .color =
            config_colony.colors.unit_shadow_color_light } };
    if( !dragging_.has_value() ) return res;
    auto const iter =
        layout_.slots[slot].units.find( dragging_->id );
    if( iter == layout_.slots[slot].units.end() ) return res;
    res.suppress = iter->second;
    return res;
  }();
  draw_rendered_icon_spread(
      renderer, layout_.slots[slot].workers_plan_origin,
      layout_.slots[slot].workers_plan, options );
}

void ColViewBuildings::draw( rr::Renderer& renderer,
                             Coord coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation2,
      gfx::size( coord.distance_from_origin() ).to_double() );
  rr::Painter painter = renderer.painter();
  for( e_colony_building_slot slot :
       refl::enum_values<e_colony_building_slot> ) {
    Rect const rect = layout_.slots[slot].bounds;
    painter.draw_empty_rect(
        rect, rr::Painter::e_border_mode::in_out, BROWN_COLOR );
    rr::Typer typer = renderer.typer(
        rect.upper_left() + Delta{ .w = 2 } + Delta{ .h = 2 },
        BROWN_COLOR );
    maybe<e_colony_building> const building =
        building_for_slot( colony_, slot );
    if( !building.has_value() ) {
      typer.write( "({})", slot );
      continue;
    }
    CHECK( building.has_value() );
    typer.write( "{}", *building );

    draw_workers( renderer, slot );

    maybe<int> quantity =
        production_for_slot( colview_production(), slot );
    if( quantity.has_value() )
      draw_rendered_icon_spread(
          renderer, layout_.slots[slot].product_plan_origin,
          layout_.slots[slot].product_plan );
  }
}

maybe<CanReceiveDraggable<ColViewObject>>
ColViewBuildings::can_receive( ColViewObject const& o, int,
                               Coord const& where ) const {
  // Verify that there is a slot under the cursor.
  UNWRAP_RETURN( slot, slot_for_coord( where ) );
  // Check that the colony has a building in this slot.
  if( !building_for_slot( colony_, slot ).has_value() )
    return nothing;
  // Check if this slot represents an indoor job that a colonist
  // can work at.
  if( !indoor_job_for_slot( slot ).has_value() ) return nothing;
  // Verify that the dragged object is a unit.
  maybe<UnitId> unit_id = o.get_if<ColViewObject::unit>().member(
      &ColViewObject::unit::id );
  if( !unit_id.has_value() ) return nothing;
  // Check if the unit is a colonist.
  UnitsState const& units_state = ss_.units;
  Unit const& unit = units_state.unit_for( *unit_id );
  if( !unit.is_colonist() ) return nothing;
  // Check if this unit is coming from another building; if so
  // we'll allow it.
  if( dragging_.has_value() )
    return CanReceiveDraggable<ColViewObject>::yes{ .draggable =
                                                        o };
  // Note that we don't check for number of workers in building
  // here; that is done in the check function.
  return CanReceiveDraggable<ColViewObject>::yes{
    .draggable = o }; // allowed.
}

wait<base::valid_or<DragRejection>> ColViewBuildings::sink_check(
    ColViewObject const& o, int, Coord const where ) {
  // These should have already been checked.
  UNWRAP_CHECK( slot, slot_for_coord( where ) );
  UNWRAP_CHECK( indoor_job, indoor_job_for_slot( slot ) );
  // Check if this unit is coming from the same building. If so
  // we'll cancel it. We do this here instead of in the
  // can_receive function because if we were to reject it there
  // then there would be a red X over the cursor. Also, if we
  // were to let the drag proceed then it would change the or-
  // dering of the units which would be strange.
  if( dragging_.has_value() && slot == dragging_->slot )
    co_return DragRejection{};
  // This should have already been checked.
  UNWRAP_CHECK( building, building_for_slot( colony_, slot ) );
  // Check that there aren't more than the max allowed units in
  // this slot.
  int const allowed_units = max_workers_for_building( building );
  if( int( colony_.indoor_jobs[indoor_job].size() ) >=
      allowed_units ) {
    string const worker_name =
        allowed_units > 1
            ? config_colony.worker_names_plural[indoor_job]
            : config_colony.worker_names_singular[indoor_job];
    co_return DragRejection{
      .reason = fmt::format(
          "There can be at most [{}] {} in a [{}].",
          allowed_units, worker_name,
          config_colony.building_display_names[building] ) };
  }
  // This should have already been checked.
  UNWRAP_CHECK( unit_id, o.get_if<ColViewObject::unit>().member(
                             &ColViewObject::unit::id ) );
  UnitsState const& units_state = ss_.units;
  Unit const& unit = units_state.unit_for( unit_id );
  // If this is a school type building then make sure that the
  // unit has the right expertise.
  maybe<e_school_type> const school_type =
      school_type_from_building( building );
  if( school_type.has_value() ) {
    base::valid_or<string> can_teach =
        can_unit_teach_in_building( unit.type(), *school_type );
    if( !can_teach.valid() )
      co_return DragRejection{ .reason = can_teach.error() };
  }

  co_return base::valid; // proceed.
}

// Implement IDragSink.
wait<> ColViewBuildings::drop( ColViewObject const& o,
                               Coord const& where ) {
  UNWRAP_CHECK( unit_id, o.get_if<ColViewObject::unit>().member(
                             &ColViewObject::unit::id ) );
  UNWRAP_CHECK( slot, slot_for_coord( where ) );
  UNWRAP_CHECK( indoor_job, indoor_job_for_slot( slot ) );
  ColonyJob const job = ColonyJob::indoor{ .job = indoor_job };
  UnitOwnershipChanger( ss_, unit_id )
      .change_to_colony( ts_, colony_, job );
  CHECK_HAS_VALUE( colony_.validate() );
  co_return;
}

maybe<DraggableObjectWithBounds<ColViewObject>>
ColViewBuildings::object_here( Coord const& where ) const {
  UNWRAP_RETURN( slot, slot_for_coord( where ) );
  UNWRAP_RETURN( indoor_job, indoor_job_for_slot( slot ) );
  vector<UnitId> const& colonists =
      colony_.indoor_jobs[indoor_job];
  if( colonists.size() == 0 ) return nothing;
  for( int idx = colonists.size() - 1; idx >= 0; --idx ) {
    Rect rect = visible_rect_for_unit_in_slot( slot, idx );
    if( where.is_inside( rect ) )
      return DraggableObjectWithBounds<ColViewObject>{
        .obj    = ColViewObject::unit{ .id = colonists[idx] },
        .bounds = sprite_rect_for_unit_in_slot( slot, idx ) };
  }
  return nothing;
}

bool ColViewBuildings::try_drag( ColViewObject const& o,
                                 Coord const& where ) {
  UNWRAP_CHECK( obj_with_bounds, object_here( where ) );
  UNWRAP_CHECK( slot, slot_for_coord( where ) );
  UNWRAP_CHECK(
      unit, obj_with_bounds.obj.get_if<ColViewObject::unit>() );
  // We can't test if o == unit because the `o` might have its
  // `transformed` field filled out, which `unit` will not. So we
  // will do the next best thing.
  CHECK( o.holds<ColViewObject::unit>() );
  CHECK( o.get<ColViewObject::unit>().id == unit.id );
  dragging_ = Dragging{ .id = unit.id, .slot = slot };
  return true;
}

// Implement IDragSource.
void ColViewBuildings::cancel_drag() { dragging_ = nothing; }

// Implement IDragSource.
wait<> ColViewBuildings::disown_dragged_object() {
  CHECK( dragging_.has_value() );
  UnitOwnershipChanger( ss_, dragging_->id ).change_to_free();
  co_return;
}

// Implement AwaitView.
wait_bool ColViewBuildings::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return true;
  maybe<e_colony_building_slot> const slot =
      slot_for_coord( event.pos );
  if( !slot.has_value() ) co_return true;
  maybe<e_colony_building> const building =
      building_for_slot( colony_, *slot );
  if( !building.has_value() ) co_return true;
  switch( *building ) {
    case e_colony_building::custom_house:
      co_await open_custom_house_menu( engine_, ts_, colony_ );
      co_return true;
    default:
      break;
  }
  co_return true;
}

ColViewBuildings::Layout ColViewBuildings::create_layout(
    IEngine& engine, SSConst const& ss, size const sz,
    Colony const& colony ) {
  Layout l;
  l.size = sz;
  // This is negative.
  l.unit_shadow_offset = UnitShadow::default_offset();

  for( e_colony_building_slot const slot :
       enum_values<e_colony_building_slot> ) {
    size const box_size = sz / size{ .w = 4, .h = 4 };
    int const idx       = static_cast<int>( slot );
    point const coord{ .x = X{ idx % 4 }, .y = Y{ idx / 4 } };
    point const upper_left = coord * box_size;
    point lower_right      = upper_left + box_size;
    if( idx / 4 == 3 ) lower_right.y = 0 + sz.h;
    rect const bounds = rect::from( upper_left, lower_right );
    maybe<e_indoor_job> const indoor_job =
        indoor_job_for_slot( slot );
    static vector<UnitId> const kEmpty;
    vector<UnitId> const& units =
        indoor_job.has_value() ? colony.indoor_jobs[*indoor_job]
                               : kEmpty;
    map<UnitId, int> units_map;
    for( int idx = 0; UnitId const unit_id : units )
      units_map[unit_id] = idx++;
    int const margin = 2;
    // This effectively subtracts the amount that is needed for
    // the unit shadows and margin.
    int const workers_spread_width = std::max(
        bounds.size.w + l.unit_shadow_offset - 2 * margin, 0 );
    int const product_spread_width =
        std::max( bounds.size.w - 2 * margin, 0 );
    l.slots[slot] = BuildingLayoutSlot{
      .bounds       = bounds,
      .product_plan = create_production_spread(
          engine.textometer(), ss, colview_production(), slot,
          product_spread_width ),
      .workers_plan = create_workers_spread(
          engine.textometer(), ss, colony, slot,
          workers_spread_width, l.unit_shadow_offset ),
      .product_plan_origin =
          bounds.nw() + size{ .w = margin, .h = 10 },
      .workers_plan_origin =
          bounds.sw() +
          size{ .w = -l.unit_shadow_offset + margin,
                .h = -32 - 1 },
      .units = std::move( units_map ) };
  }
  return l;
}

void ColViewBuildings::update_this_and_children() {
  // This method is only called when the logical resolution
  // hasn't changed, so we assume the size hasn't changed.
  layout_ = create_layout( engine_, ss_, layout_.size, colony_ );
}

std::unique_ptr<ColViewBuildings> ColViewBuildings::create(
    IEngine& engine, SS& ss, TS& ts, Player& player,
    Colony& colony, Delta sz ) {
  Layout layout = create_layout( engine, ss, sz, colony );
  return std::make_unique<ColViewBuildings>(
      engine, ss, ts, player, colony, std::move( layout ) );
}

} // namespace rn
