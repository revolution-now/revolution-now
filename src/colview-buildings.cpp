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
#include "colony-mgr.hpp"
#include "game-state.hpp" // FIXME
#include "production.hpp"
#include "render.hpp"
#include "tiles.hpp"

// game-state
#include "gs/colony.hpp"
#include "gs/units.hpp"

// config
#include "config/colony.rds.hpp"

// Rds
#include "production.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

maybe<e_tile> tile_for_slot( e_colony_building_slot slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return e_tile::commodity_muskets;
    case e_colony_building_slot::tools:
      return e_tile::commodity_tools;
    case e_colony_building_slot::rum:
      return e_tile::commodity_rum;
    case e_colony_building_slot::cloth:
      return e_tile::commodity_cloth;
    case e_colony_building_slot::coats:
      return e_tile::commodity_fur;
    case e_colony_building_slot::cigars:
      return e_tile::commodity_cigars;
    case e_colony_building_slot::hammers:
      return e_tile::product_hammers;
    case e_colony_building_slot::town_hall:
      return e_tile::product_bells;
    case e_colony_building_slot::newspapers: return nothing;
    case e_colony_building_slot::schools: return nothing;
    case e_colony_building_slot::offshore: return nothing;
    case e_colony_building_slot::horses:
      return e_tile::commodity_horses;
    case e_colony_building_slot::wall: return nothing;
    case e_colony_building_slot::warehouses: return nothing;
    case e_colony_building_slot::crosses:
      return e_tile::product_crosses;
    case e_colony_building_slot::custom_house: return nothing;
  }
}

} // namespace

/****************************************************************
** Buildings
*****************************************************************/
Rect ColViewBuildings::rect_for_slot(
    e_colony_building_slot slot ) const {
  // TODO: Temporary.
  Delta const box_size = delta() / Delta{ .w = 4, .h = 4 };
  int const   idx      = static_cast<int>( slot );
  Coord const coord{ .x = X{ idx % 4 }, .y = Y{ idx / 4 } };
  Coord const upper_left  = coord * box_size;
  Coord       lower_right = upper_left + box_size;
  if( idx / 4 == 3 ) lower_right.y = 0 + delta().h;
  return Rect::from( upper_left, lower_right );
}

int const kEffectiveUnitWidthPixels = g_tile_delta.w / 2;

Rect ColViewBuildings::visible_rect_for_unit_in_slot(
    e_colony_building_slot slot, int unit_idx ) const {
  Rect  rect = rect_for_slot( slot );
  Coord pos  = rect.lower_left() - Delta{ .h = g_tile_delta.h };
  pos.x += 3;
  pos.x += W{ kEffectiveUnitWidthPixels } * unit_idx;
  return Rect::from( pos, Delta{ .w = g_tile_delta.w / 2,
                                 .h = g_tile_delta.h } );
}

Rect ColViewBuildings::sprite_rect_for_unit_in_slot(
    e_colony_building_slot slot, int unit_idx ) const {
  return visible_rect_for_unit_in_slot( slot, unit_idx ) -
         Delta{ .w = W{ ( g_tile_delta.w -
                          kEffectiveUnitWidthPixels ) /
                        2 },
                .h = 0 };
}

maybe<e_colony_building_slot> ColViewBuildings::slot_for_coord(
    Coord where ) const {
  for( e_colony_building_slot slot :
       refl::enum_values<e_colony_building_slot> )
    if( where.is_inside( rect_for_slot( slot ) ) ) //
      return slot;
  return nothing;
}

void ColViewBuildings::draw( rr::Renderer& renderer,
                             Coord         coord ) const {
  SCOPED_RENDERER_MOD_ADD(
      painter_mods.repos.translation,
      gfx::to_double(
          gfx::size( coord.distance_from_origin() ) ) );
  rr::Painter painter = renderer.painter();
  for( e_colony_building_slot slot :
       refl::enum_values<e_colony_building_slot> ) {
    Rect const rect = rect_for_slot( slot );
    painter.draw_empty_rect( rect,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::black() );
    rr::Typer typer = renderer.typer(
        rect.upper_left() + Delta{ .w = 1 } + Delta{ .h = 1 },
        gfx::pixel::black() );
    maybe<e_colony_building> const building =
        building_for_slot( colony_, slot );
    if( !building.has_value() ) {
      typer.write( "({})", slot );
      continue;
    }
    CHECK( building.has_value() );
    typer.write( "{}", *building );

    maybe<e_indoor_job> const indoor_job =
        indoor_job_for_slot( slot );

    gfx::pixel const kShadowColor{
        .r = 60, .g = 80, .b = 80, .a = 255 };
    if( indoor_job ) {
      vector<UnitId> const& colonists =
          colony_.indoor_jobs[*indoor_job];
      for( int idx = 0; idx < int( colonists.size() ); ++idx ) {
        UnitId unit_id = colonists[idx];
        if( dragging_.has_value() && dragging_->id == unit_id )
          continue;
#if 0
        // For debugging the bounding rects.
        Coord pos = visible_rect_for_unit_in_slot( slot, idx )
                        .upper_left();
        painter.draw_empty_rect(
            Rect::from( pos,
                        Delta( W{ kEffectiveUnitWidthPixels },
                               g_tile_delta.h ) ),
            rr::Painter::e_border_mode::in_out,
            gfx::pixel{ .r = 0, .g = 0, .b = 0, .a = 30 } );
#endif
        render_unit(
            renderer,
            sprite_rect_for_unit_in_slot( slot, idx )
                .upper_left(),
            unit_id,
            UnitRenderOptions{ .flag   = false,
                               .shadow = UnitShadow{
                                   .color = kShadowColor } } );
      }
    }

    maybe<int> quantity =
        production_for_slot( colview_production(), slot );
    if( quantity.has_value() ) {
      UNWRAP_CHECK( tile, tile_for_slot( slot ) );
      Coord pos =
          rect.upper_left() + Delta{ .w = 2 } +
          Delta{ .h =
                     H{ rr::rendered_text_line_size_pixels( "x" )
                            .h } };
      render_sprite( painter, pos, tile );
      pos.x += sprite_size( tile ).w;
      rr::Typer typer =
          renderer.typer( pos, gfx::pixel::black() );
      typer.write( "x {}", *quantity );
    }
  }
}

maybe<ColViewObject_t> ColViewBuildings::can_receive(
    ColViewObject_t const& o, e_colview_entity,
    Coord const&           where ) const {
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
  // Check if the unit is a human.
  UnitsState const& units_state = GameState::units();
  Unit const&       unit = units_state.unit_for( *unit_id );
  if( !unit.is_human() ) return nothing;
  // Check if this unit is coming from another building; if so
  // we'll allow it.
  if( dragging_.has_value() ) return o;
  // Note that we don't check for number of workers in building
  // here; that is done in the check function.
  return o; // allowed.
}

wait<base::valid_or<IColViewDragSinkCheck::Rejection>>
ColViewBuildings::check( ColViewObject_t const&,
                         e_colview_entity,
                         Coord const where ) const {
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
    co_return IColViewDragSinkCheck::Rejection{};
  // Check that there aren't more than the max allowed units in
  // this slot.
  if( int( colony_.indoor_jobs[indoor_job].size() ) >=
      config_colony.max_workers_per_building ) {
    co_return IColViewDragSinkCheck::Rejection{
        .reason = fmt::format(
            "There can be at most @[H]{}@[] workers per colony "
            "building.",
            config_colony.max_workers_per_building ) };
  }
  co_return base::valid; // proceed.
}

// Implement IColViewDragSink.
void ColViewBuildings::drop( ColViewObject_t const& o,
                             Coord const&           where ) {
  UNWRAP_CHECK( unit_id, o.get_if<ColViewObject::unit>().member(
                             &ColViewObject::unit::id ) );
  UNWRAP_CHECK( slot, slot_for_coord( where ) );
  UNWRAP_CHECK( indoor_job, indoor_job_for_slot( slot ) );
  ColonyJob_t job = ColonyJob::indoor{ .job = indoor_job };
  move_unit_to_colony( GameState::units(), colony_, unit_id,
                       job );
  CHECK_HAS_VALUE( colony_.validate() );
}

maybe<ColViewObjectWithBounds> ColViewBuildings::object_here(
    Coord const& where ) const {
  UNWRAP_RETURN( slot, slot_for_coord( where ) );
  UNWRAP_RETURN( indoor_job, indoor_job_for_slot( slot ) );
  vector<UnitId> const& colonists =
      colony_.indoor_jobs[indoor_job];
  if( colonists.size() == 0 ) return nothing;
  for( int idx = colonists.size() - 1; idx >= 0; --idx ) {
    Rect rect = visible_rect_for_unit_in_slot( slot, idx );
    if( where.is_inside( rect ) )
      return ColViewObjectWithBounds{
          .obj    = ColViewObject::unit{ .id = colonists[idx] },
          .bounds = sprite_rect_for_unit_in_slot( slot, idx ) };
  }
  return nothing;
}

bool ColViewBuildings::try_drag( ColViewObject_t const& o,
                                 Coord const&           where ) {
  UNWRAP_CHECK( obj_with_bounds, object_here( where ) );
  UNWRAP_CHECK( slot, slot_for_coord( where ) );
  UNWRAP_CHECK(
      unit, obj_with_bounds.obj.get_if<ColViewObject::unit>() );
  CHECK( o == ColViewObject_t{ unit } ); // Sanity check.
  dragging_ = Dragging{ .id = unit.id, .slot = slot };
  return true;
}

// Implement IColViewDragSource.
void ColViewBuildings::cancel_drag() { dragging_ = nothing; }

// Implement IColViewDragSource.
void ColViewBuildings::disown_dragged_object() {
  CHECK( dragging_.has_value() );
  UnitsState& units_state = GameState::units();
  remove_unit_from_colony( units_state, colony_, dragging_->id );
}

// Implement AwaitView.
wait<> ColViewBuildings::perform_click(
    input::mouse_button_event_t const& event ) {
  if( event.buttons != input::e_mouse_button_event::left_up )
    co_return;
  // TODO
  co_return;
}

} // namespace rn
