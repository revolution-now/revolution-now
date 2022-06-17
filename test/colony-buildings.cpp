/****************************************************************
**colony-buildings.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-14.
*
* Description: Unit tests for the src/colony-buildings.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/colony-buildings.hpp"

// Revolution Now
#include "src/colony.hpp"

// refl
#include "refl/to-str.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

#define REQUIRE_IN_OUT( in, ... ) \
  expected = __VA_ARGS__;         \
  REQUIRE( f( IN::in ) == expected )

TEST_CASE( "[colony-buildings] slot_for_building" ) {
  using IN  = e_colony_building;
  using OUT = e_colony_building_slot;

  OUT               expected = {};
  unordered_set<IN> used;

  auto f = [&]( IN o ) {
    used.insert( o );
    return slot_for_building( o );
  };

  REQUIRE_IN_OUT( armory, OUT::muskets );
  REQUIRE_IN_OUT( blacksmiths_house, OUT::tools );
  REQUIRE_IN_OUT( carpenters_shop, OUT::hammers );
  REQUIRE_IN_OUT( fur_traders_house, OUT::coats );
  REQUIRE_IN_OUT( rum_distillers_house, OUT::rum );
  REQUIRE_IN_OUT( tobacconists_house, OUT::cigars );
  REQUIRE_IN_OUT( weavers_house, OUT::cloth );
  REQUIRE_IN_OUT( blacksmiths_shop, OUT::tools );
  REQUIRE_IN_OUT( fur_trading_post, OUT::coats );
  REQUIRE_IN_OUT( lumber_mill, OUT::hammers );
  REQUIRE_IN_OUT( magazine, OUT::muskets );
  REQUIRE_IN_OUT( rum_distillery, OUT::rum );
  REQUIRE_IN_OUT( tobacconists_shop, OUT::cigars );
  REQUIRE_IN_OUT( weavers_shop, OUT::cloth );
  REQUIRE_IN_OUT( arsenal, OUT::muskets );
  REQUIRE_IN_OUT( cigar_factory, OUT::cigars );
  REQUIRE_IN_OUT( fur_factory, OUT::coats );
  REQUIRE_IN_OUT( iron_works, OUT::tools );
  REQUIRE_IN_OUT( rum_factory, OUT::rum );
  REQUIRE_IN_OUT( textile_mill, OUT::cloth );
  REQUIRE_IN_OUT( town_hall, OUT::town_hall );
  REQUIRE_IN_OUT( printing_press, OUT::newspapers );
  REQUIRE_IN_OUT( newspaper, OUT::newspapers );
  REQUIRE_IN_OUT( schoolhouse, OUT::schools );
  REQUIRE_IN_OUT( college, OUT::schools );
  REQUIRE_IN_OUT( university, OUT::schools );
  REQUIRE_IN_OUT( docks, OUT::offshore );
  REQUIRE_IN_OUT( drydock, OUT::offshore );
  REQUIRE_IN_OUT( shipyard, OUT::offshore );
  REQUIRE_IN_OUT( stable, OUT::horses );
  REQUIRE_IN_OUT( stockade, OUT::wall );
  REQUIRE_IN_OUT( fort, OUT::wall );
  REQUIRE_IN_OUT( fortress, OUT::wall );
  REQUIRE_IN_OUT( warehouse, OUT::warehouses );
  REQUIRE_IN_OUT( warehouse_expansion, OUT::warehouses );
  REQUIRE_IN_OUT( church, OUT::crosses );
  REQUIRE_IN_OUT( cathedral, OUT::crosses );
  REQUIRE_IN_OUT( custom_house, OUT::custom_house );

  REQUIRE( used.size() == refl::enum_count<IN> );
}

TEST_CASE( "[colony-buildings] indoor_job_for_slot" ) {
  using IN  = e_colony_building_slot;
  using OUT = maybe<e_indoor_job>;

  OUT               expected = {};
  unordered_set<IN> used;

  auto f = [&]( IN o ) {
    used.insert( o );
    return indoor_job_for_slot( o );
  };

  REQUIRE_IN_OUT( muskets, e_indoor_job::muskets );
  REQUIRE_IN_OUT( tools, e_indoor_job::tools );
  REQUIRE_IN_OUT( rum, e_indoor_job::rum );
  REQUIRE_IN_OUT( cloth, e_indoor_job::cloth );
  REQUIRE_IN_OUT( coats, e_indoor_job::coats );
  REQUIRE_IN_OUT( cigars, e_indoor_job::cigars );
  REQUIRE_IN_OUT( hammers, e_indoor_job::hammers );
  REQUIRE_IN_OUT( town_hall, e_indoor_job::bells );
  REQUIRE_IN_OUT( newspapers, nothing );
  REQUIRE_IN_OUT( schools, e_indoor_job::teacher );
  REQUIRE_IN_OUT( offshore, nothing );
  REQUIRE_IN_OUT( horses, nothing );
  REQUIRE_IN_OUT( wall, nothing );
  REQUIRE_IN_OUT( warehouses, nothing );
  REQUIRE_IN_OUT( crosses, e_indoor_job::crosses );
  REQUIRE_IN_OUT( custom_house, nothing );

  REQUIRE( used.size() == refl::enum_count<IN> );
}

TEST_CASE( "[colony-buildings] slot_for_indoor_job" ) {
  using IN  = e_indoor_job;
  using OUT = e_colony_building_slot;

  OUT               expected = {};
  unordered_set<IN> used;

  auto f = [&]( IN o ) {
    used.insert( o );
    return slot_for_indoor_job( o );
  };

  REQUIRE_IN_OUT( bells, OUT::town_hall );
  REQUIRE_IN_OUT( crosses, OUT::crosses );
  REQUIRE_IN_OUT( hammers, OUT::hammers );
  REQUIRE_IN_OUT( rum, OUT::rum );
  REQUIRE_IN_OUT( cigars, OUT::cigars );
  REQUIRE_IN_OUT( cloth, OUT::cloth );
  REQUIRE_IN_OUT( coats, OUT::coats );
  REQUIRE_IN_OUT( tools, OUT::tools );
  REQUIRE_IN_OUT( muskets, OUT::muskets );
  REQUIRE_IN_OUT( teacher, OUT::schools );

  REQUIRE( used.size() == refl::enum_count<IN> );
}

TEST_CASE( "[colony-buildings] buildings_for_slot" ) {
  using IN    = e_colony_building_slot;
  using OUT   = vector<e_colony_building>;
  using OUT_E = e_colony_building;

  OUT               expected = {};
  unordered_set<IN> used;

  auto f = [&]( IN o ) {
    used.insert( o );
    return buildings_for_slot( o );
  };

  REQUIRE_IN_OUT( muskets, { OUT_E::arsenal, OUT_E::magazine,
                             OUT_E::armory } );
  REQUIRE_IN_OUT( tools,
                  { OUT_E::iron_works, OUT_E::blacksmiths_shop,
                    OUT_E::blacksmiths_house } );
  REQUIRE_IN_OUT( rum,
                  { OUT_E::rum_factory, OUT_E::rum_distillery,
                    OUT_E::rum_distillers_house } );
  REQUIRE_IN_OUT( cloth,
                  { OUT_E::textile_mill, OUT_E::weavers_shop,
                    OUT_E::weavers_house } );
  REQUIRE_IN_OUT( coats,
                  { OUT_E::fur_factory, OUT_E::fur_trading_post,
                    OUT_E::fur_traders_house } );
  REQUIRE_IN_OUT(
      cigars, { OUT_E::cigar_factory, OUT_E::tobacconists_shop,
                OUT_E::tobacconists_house } );
  REQUIRE_IN_OUT(
      hammers, { OUT_E::lumber_mill, OUT_E::carpenters_shop } );
  REQUIRE_IN_OUT( town_hall, { OUT_E::town_hall } );
  REQUIRE_IN_OUT( newspapers,
                  { OUT_E::newspaper, OUT_E::printing_press } );
  REQUIRE_IN_OUT( schools, { OUT_E::university, OUT_E::college,
                             OUT_E::schoolhouse } );
  REQUIRE_IN_OUT( offshore, { OUT_E::shipyard, OUT_E::drydock,
                              OUT_E::docks } );
  REQUIRE_IN_OUT( horses, { OUT_E::stable } );
  REQUIRE_IN_OUT(
      wall, { OUT_E::fortress, OUT_E::fort, OUT_E::stockade } );
  REQUIRE_IN_OUT( warehouses, { OUT_E::warehouse_expansion,
                                OUT_E::warehouse } );
  REQUIRE_IN_OUT( crosses, { OUT_E::cathedral, OUT_E::church } );
  REQUIRE_IN_OUT( custom_house, { OUT_E::custom_house } );

  REQUIRE( used.size() == refl::enum_count<IN> );
}

TEST_CASE( "[colony-buildings] building_for_slot" ) {
  using IN    = e_colony_building_slot;
  using OUT   = maybe<e_colony_building>;
  using OUT_E = e_colony_building;

  OUT               expected = {};
  unordered_set<IN> used;

  Colony colony;

  // Add some buidings but not all. In some cases add multiple
  // per slot, in others just one.
  colony.add_building( OUT_E::magazine );
  colony.add_building( OUT_E::armory );

  colony.add_building( OUT_E::iron_works );
  colony.add_building( OUT_E::blacksmiths_house );

  colony.add_building( OUT_E::rum_factory );
  colony.add_building( OUT_E::rum_distillers_house );

  colony.add_building( OUT_E::fur_trading_post );

  colony.add_building( OUT_E::cigar_factory );
  colony.add_building( OUT_E::tobacconists_shop );
  colony.add_building( OUT_E::tobacconists_house );

  colony.add_building( OUT_E::carpenters_shop );

  colony.add_building( OUT_E::town_hall );

  colony.add_building( OUT_E::college );

  colony.add_building( OUT_E::docks );

  colony.add_building( OUT_E::stable );

  colony.add_building( OUT_E::warehouse_expansion );

  colony.add_building( OUT_E::cathedral );
  colony.add_building( OUT_E::church );

  colony.add_building( OUT_E::custom_house );

  auto f = [&]( IN o ) {
    used.insert( o );
    return building_for_slot( colony, o );
  };

  REQUIRE_IN_OUT( muskets, OUT_E::magazine );
  REQUIRE_IN_OUT( tools, OUT_E::iron_works );
  REQUIRE_IN_OUT( rum, OUT_E::rum_factory );
  REQUIRE_IN_OUT( cloth, nothing );
  REQUIRE_IN_OUT( coats, OUT_E::fur_trading_post );
  REQUIRE_IN_OUT( cigars, OUT_E::cigar_factory );
  REQUIRE_IN_OUT( hammers, OUT_E::carpenters_shop );
  REQUIRE_IN_OUT( town_hall, OUT_E::town_hall );
  REQUIRE_IN_OUT( newspapers, nothing );
  REQUIRE_IN_OUT( schools, OUT_E::college );
  REQUIRE_IN_OUT( offshore, OUT_E::docks );
  REQUIRE_IN_OUT( horses, OUT_E::stable );
  REQUIRE_IN_OUT( wall, nothing );
  REQUIRE_IN_OUT( warehouses, OUT_E::warehouse_expansion );
  REQUIRE_IN_OUT( crosses, OUT_E::cathedral );
  REQUIRE_IN_OUT( custom_house, OUT_E::custom_house );

  REQUIRE( used.size() == refl::enum_count<IN> );
}

TEST_CASE( "[colony-buildings] colony_has_building_level" ) {
  using IN  = e_colony_building;
  using OUT = bool;

  OUT expected = {};

  Colony colony;

  // Has all three.
  colony.add_building( e_colony_building::arsenal );
  colony.add_building( e_colony_building::magazine );
  colony.add_building( e_colony_building::armory );

  // Has first and last.
  colony.add_building( e_colony_building::iron_works );
  colony.add_building( e_colony_building::blacksmiths_house );

  // Has first and second.
  colony.add_building( e_colony_building::rum_factory );
  colony.add_building( e_colony_building::rum_distillery );

  // Has only last.
  colony.add_building( e_colony_building::fur_traders_house );

  // Has only first.
  colony.add_building( e_colony_building::cigar_factory );

  // Has only.
  colony.add_building( e_colony_building::town_hall );

  // Has only middle.
  colony.add_building( e_colony_building::college );

  // Bas second and third.
  colony.add_building( e_colony_building::drydock );
  colony.add_building( e_colony_building::docks );

  auto f = [&]( IN o ) {
    return colony_has_building_level( colony, o );
  };

  // Has all three.
  REQUIRE_IN_OUT( arsenal, true );
  REQUIRE_IN_OUT( magazine, true );
  REQUIRE_IN_OUT( armory, true );

  // Has first and last.
  REQUIRE_IN_OUT( iron_works, true );
  REQUIRE_IN_OUT( blacksmiths_shop, true );
  REQUIRE_IN_OUT( blacksmiths_house, true );

  // Has first and second.
  REQUIRE_IN_OUT( rum_factory, true );
  REQUIRE_IN_OUT( rum_distillery, true );
  REQUIRE_IN_OUT( rum_distillers_house, true );

  // Has only last.
  REQUIRE_IN_OUT( fur_factory, false );
  REQUIRE_IN_OUT( fur_trading_post, false );
  REQUIRE_IN_OUT( fur_traders_house, true );

  // Has only first.
  REQUIRE_IN_OUT( cigar_factory, true );
  REQUIRE_IN_OUT( tobacconists_shop, true );
  REQUIRE_IN_OUT( tobacconists_house, true );

  // Has only.
  REQUIRE_IN_OUT( town_hall, true );

  // Has only middle.
  REQUIRE_IN_OUT( university, false );
  REQUIRE_IN_OUT( college, true );
  REQUIRE_IN_OUT( schoolhouse, true );

  // Has second and third.
  REQUIRE_IN_OUT( shipyard, false );
  REQUIRE_IN_OUT( drydock, true );
  REQUIRE_IN_OUT( docks, true );

  // Has none.
  REQUIRE_IN_OUT( textile_mill, false );
  REQUIRE_IN_OUT( weavers_shop, false );
  REQUIRE_IN_OUT( weavers_house, false );
}

TEST_CASE( "[colony-buildings] colony_warehouse_capacity" ) {
  Colony colony;
  REQUIRE( colony_warehouse_capacity( colony ) == 100 );
  colony.add_building( e_colony_building::warehouse );
  REQUIRE( colony_warehouse_capacity( colony ) == 200 );
  colony.add_building( e_colony_building::warehouse_expansion );
  REQUIRE( colony_warehouse_capacity( colony ) == 300 );
  colony.rm_building( e_colony_building::warehouse );
  REQUIRE( colony_warehouse_capacity( colony ) == 300 );
  colony.rm_building( e_colony_building::warehouse_expansion );
  REQUIRE( colony_warehouse_capacity( colony ) == 100 );
}

} // namespace
} // namespace rn
