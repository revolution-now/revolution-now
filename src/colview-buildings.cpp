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
#include "colony.hpp"
#include "game-state.hpp" // FIXME
#include "gs-units.hpp"
#include "production.hpp"
#include "render.hpp"

// config
#include "config/colony.rds.hpp"

// refl
#include "refl/query-enum.hpp"
#include "refl/to-str.hpp"

using namespace std;

namespace rn {

namespace {

struct SlotProduction {
  int    quantity = {};
  e_tile tile     = {};
};

maybe<SlotProduction> production_for_slot(
    e_colony_building_slot slot ) {
  ColonyProduction const& pr = colview_production();
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return SlotProduction{
          .quantity =
              pr.ore_products.muskets_produced_theoretical,
          .tile = e_tile::commodity_muskets,
      };
    case e_colony_building_slot::tools:
      return SlotProduction{
          .quantity = pr.ore_products.tools_produced_theoretical,
          .tile     = e_tile::commodity_tools,
      };
    case e_colony_building_slot::rum:
      return SlotProduction{
          .quantity = pr.sugar_rum.product_produced_theoretical,
          .tile     = e_tile::commodity_rum,
      };
    case e_colony_building_slot::cloth:
      return SlotProduction{
          .quantity =
              pr.cotton_cloth.product_produced_theoretical,
          .tile = e_tile::commodity_cloth,
      };
    case e_colony_building_slot::fur:
      return SlotProduction{
          .quantity = pr.fur_coats.product_produced_theoretical,
          .tile     = e_tile::commodity_fur,
      };
    case e_colony_building_slot::cigars:
      return SlotProduction{
          .quantity =
              pr.tobacco_cigars.product_produced_theoretical,
          .tile = e_tile::commodity_cigars,
      };
    case e_colony_building_slot::hammers:
      return SlotProduction{
          .quantity =
              pr.lumber_hammers.product_produced_theoretical,
          .tile = e_tile::product_hammers,
      };
    case e_colony_building_slot::town_hall:
      return SlotProduction{
          .quantity = pr.bells,
          .tile     = e_tile::product_bells,
      };
    case e_colony_building_slot::newspapers: return nothing;
    case e_colony_building_slot::schools: return nothing;
    case e_colony_building_slot::offshore: return nothing;
    case e_colony_building_slot::horses:
      return SlotProduction{
          .quantity = pr.food.horses_produced_theoretical,
          .tile     = e_tile::commodity_horses,
      };
    case e_colony_building_slot::wall: return nothing;
    case e_colony_building_slot::warehouses: return nothing;
    case e_colony_building_slot::crosses:
      return SlotProduction{
          .quantity = pr.crosses,
          .tile     = e_tile::product_crosses,
      };
    case e_colony_building_slot::custom_house: return nothing;
  }
}

maybe<e_indoor_job> indoor_job_for_slot(
    e_colony_building_slot slot ) {
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return e_indoor_job::muskets;
    case e_colony_building_slot::tools:
      return e_indoor_job::tools;
    case e_colony_building_slot::rum: return e_indoor_job::rum;
    case e_colony_building_slot::cloth:
      return e_indoor_job::cloth;
    case e_colony_building_slot::fur: return e_indoor_job::coats;
    case e_colony_building_slot::cigars:
      return e_indoor_job::cigars;
    case e_colony_building_slot::hammers:
      return e_indoor_job::hammers;
    case e_colony_building_slot::town_hall:
      return e_indoor_job::bells;
    case e_colony_building_slot::newspapers: return nothing;
    case e_colony_building_slot::schools:
      return e_indoor_job::teacher;
    case e_colony_building_slot::offshore: return nothing;
    case e_colony_building_slot::horses: return nothing;
    case e_colony_building_slot::wall: return nothing;
    case e_colony_building_slot::warehouses: return nothing;
    case e_colony_building_slot::crosses:
      return e_indoor_job::crosses;
    case e_colony_building_slot::custom_house: return nothing;
  }
}

} // namespace

/****************************************************************
** Buildings
*****************************************************************/
maybe<e_colony_building> ColViewBuildings::building_for_slot(
    e_colony_building_slot slot ) const {
  refl::enum_map<e_colony_building, bool> const& buildings =
      colony_.buildings();
  auto select =
      [&]( initializer_list<e_colony_building> possible )
      -> maybe<e_colony_building> {
    for( e_colony_building building : possible )
      if( buildings[building] ) return building;
    return nothing;
  };
  switch( slot ) {
    case e_colony_building_slot::muskets:
      return select( {
          e_colony_building::arsenal,
          e_colony_building::magazine,
          e_colony_building::armory,
      } );
    case e_colony_building_slot::tools:
      return select( {
          e_colony_building::iron_works,
          e_colony_building::blacksmiths_shop,
          e_colony_building::blacksmiths_house,
      } );
    case e_colony_building_slot::rum:
      return select( {
          e_colony_building::rum_factory,
          e_colony_building::rum_distillery,
          e_colony_building::rum_distillers_house,
      } );
    case e_colony_building_slot::cloth:
      return select( {
          e_colony_building::textile_mill,
          e_colony_building::weavers_shop,
          e_colony_building::weavers_house,
      } );
    case e_colony_building_slot::fur:
      return select( {
          e_colony_building::fur_factory,
          e_colony_building::fur_trading_post,
          e_colony_building::fur_traders_house,
      } );
    case e_colony_building_slot::cigars:
      return select( {
          e_colony_building::cigar_factory,
          e_colony_building::tobacconists_shop,
          e_colony_building::tobacconists_house,
      } );
    case e_colony_building_slot::hammers:
      return select( {
          e_colony_building::lumber_mill,
          e_colony_building::carpenters_shop,
      } );
    case e_colony_building_slot::town_hall:
      return select( {
          e_colony_building::town_hall,
      } );
    case e_colony_building_slot::newspapers:
      return select( {
          e_colony_building::newspaper,
          e_colony_building::printing_press,
      } );
    case e_colony_building_slot::schools:
      return select( {
          e_colony_building::university,
          e_colony_building::college,
          e_colony_building::schoolhouse,
      } );
    case e_colony_building_slot::offshore:
      return select( {
          e_colony_building::shipyard,
          e_colony_building::drydock,
          e_colony_building::docks,
      } );
    case e_colony_building_slot::horses:
      return select( {
          e_colony_building::stable,
      } );
    case e_colony_building_slot::wall:
      return select( {
          e_colony_building::fortress,
          e_colony_building::fort,
          e_colony_building::stockade,
      } );
    case e_colony_building_slot::warehouses:
      return select( {
          e_colony_building::warehouse_expansion,
          e_colony_building::warehouse,
      } );
    case e_colony_building_slot::crosses:
      return select( {
          e_colony_building::cathedral,
          e_colony_building::church,
      } );
    case e_colony_building_slot::custom_house:
      return select( {
          e_colony_building::custom_house,
      } );
  }
}

Rect ColViewBuildings::rect_for_slot(
    e_colony_building_slot slot ) const {
  // TODO: Temporary.
  Delta const box_size = delta() / Scale{ 4 };
  int const   idx      = static_cast<int>( slot );
  Coord const coord( X{ idx % 4 }, Y{ idx / 4 } );
  Coord const upper_left  = coord * box_size.to_scale();
  Coord       lower_right = upper_left + box_size;
  if( idx / 4 == 3 ) lower_right.y = 0_y + delta().h;
  return Rect::from( upper_left, lower_right );
}

int const kEffectiveUnitWidthPixels = g_tile_delta.w._ / 2;

Rect ColViewBuildings::visible_rect_for_unit_in_slot(
    e_colony_building_slot slot, int unit_idx ) const {
  Rect  rect = rect_for_slot( slot );
  Coord pos  = rect.lower_left() - g_tile_delta.h;
  pos += 3_w;
  pos += W{ kEffectiveUnitWidthPixels } * SX{ unit_idx };
  return Rect::from(
      pos, Delta( g_tile_delta.w / 2_sx, g_tile_delta.h ) );
}

Rect ColViewBuildings::sprite_rect_for_unit_in_slot(
    e_colony_building_slot slot, int unit_idx ) const {
  return visible_rect_for_unit_in_slot( slot, unit_idx ) -
         Delta( W{ ( g_tile_delta.w._ -
                     kEffectiveUnitWidthPixels ) /
                   2 },
                0_h );
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
  SCOPED_RENDERER_MOD_ADD( painter_mods.repos.translation,
                           coord.distance_from_origin() );
  rr::Painter painter = renderer.painter();
  for( e_colony_building_slot slot :
       refl::enum_values<e_colony_building_slot> ) {
    Rect const rect = rect_for_slot( slot );
    painter.draw_empty_rect( rect,
                             rr::Painter::e_border_mode::in_out,
                             gfx::pixel::black() );
    rr::Typer typer = renderer.typer(
        rect.upper_left() + 1_w + 1_h, gfx::pixel::black() );
    maybe<e_colony_building> const building =
        building_for_slot( slot );
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
          colony_.indoor_jobs()[*indoor_job];
      for( int idx = 0; idx < int( colonists.size() ); ++idx ) {
        UnitId unit_id = colonists[idx];
        if( dragging_.has_value() && dragging_->id == unit_id )
          continue;
        Coord pos = visible_rect_for_unit_in_slot( slot, idx )
                        .upper_left();
        // For debugging the bounding rects.
        painter.draw_empty_rect(
            Rect::from( pos,
                        Delta( W{ kEffectiveUnitWidthPixels },
                               g_tile_delta.h ) ),
            rr::Painter::e_border_mode::in_out,
            gfx::pixel{ .r = 0, .g = 0, .b = 0, .a = 30 } );
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

    maybe<SlotProduction> product = production_for_slot( slot );
    if( product.has_value() ) {
      Coord pos =
          rect.upper_left() + 2_h +
          H{ rr::rendered_text_line_size_pixels( "x" ).h };
      render_sprite( painter, pos, product->tile );
      pos += sprite_size( product->tile ).w;
      rr::Typer typer =
          renderer.typer( pos, gfx::pixel::black() );
      typer.write( "x {}", product->quantity );
    }
  }
}

maybe<ColViewObject_t> ColViewBuildings::can_receive(
    ColViewObject_t const& o, e_colview_entity,
    Coord const&           where ) const {
  // Verify that there is a slot under the cursor.
  UNWRAP_RETURN( slot, slot_for_coord( where ) );
  // Check that the colony has a building in this slot.
  if( !building_for_slot( slot ).has_value() ) return nothing;
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
ColViewBuildings::check( ColViewObject_t const& o,
                         e_colview_entity       from,
                         Coord const            where ) const {
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
  if( int( colony_.indoor_jobs()[indoor_job].size() ) >=
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
      colony_.indoor_jobs()[indoor_job];
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

} // namespace rn
