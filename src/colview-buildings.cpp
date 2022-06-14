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
#include "production.hpp"
#include "render.hpp"

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
  return Rect::from( coord * box_size.to_scale(), box_size );
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

    if( indoor_job ) {
      unordered_set<UnitId> const& colonists =
          colony_.indoor_jobs()[*indoor_job];
      Coord pos = rect.lower_left() - g_tile_delta.h;
      for( UnitId id : colonists ) {
        render_unit( renderer, pos, id, /*with_icon=*/false );
        pos += g_tile_delta.w / 2_sx;
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

} // namespace rn
