/****************************************************************
**colony-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2020-01-01.
*
* Description: Unit tests for colony-mgr module.
*
*****************************************************************/
#include "testing.hpp"

// Testing
#include "test/fake/world.hpp"

// Revolution Now
#include "colony-mgr.hpp"
#include "cstate.hpp"
#include "game-state.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "ustate.hpp"

// gs
#include "gs/colonies.hpp"
#include "gs/root.hpp"
#include "gs/terrain.hpp"
#include "gs/unit-type.hpp"
#include "gs/units.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rn;

using ::Catch::UnorderedEquals;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() { add_player( e_nation::dutch ); }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    // clang-format off
    vector<MapSquare> tiles{
      _, L, _,
      L, L, L,
      _, L, L,
    };
    // clang-format on
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** FIXME: Legacy World Setup
*****************************************************************/
MapSquare make_land_square() {
  return map_square_for_terrain( e_terrain::grassland );
}

MapSquare make_ocean_square() {
  return map_square_for_terrain( e_terrain::ocean );
}

// FIXME: remove
void generate_unittest_terrain( TerrainState& terrain_state ) {
  MapSquare const L = make_land_square();
  MapSquare const O = make_ocean_square();

  NonRenderingMapUpdater map_updater( terrain_state );
  map_updater.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m = Matrix<MapSquare>( Delta{ .w = 10, .h = 10 } );
    Rect land_rect{ .x = 2, .y = 2, .w = 6, .h = 6 };
    for( auto const& coord : terrain_state.world_map().rect() ) {
      m[coord] = O;
      if( coord.is_inside( land_rect ) ) m[coord] = L;
    }
  } );
}

UnitId create_colonist_on_map( UnitsState&  units_state,
                               Coord        where,
                               IMapUpdater& map_updater ) {
  return create_unit_on_map_non_interactive(
      units_state, map_updater, e_nation::english,
      UnitComposition::create( e_unit_type::free_colonist ),
      where );
}

UnitId create_colonist( UnitsState& units_state ) {
  return create_unit(
      units_state, e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
}

UnitId create_dragoon_on_map( UnitsState&  units_state,
                              Coord        where,
                              IMapUpdater& map_updater ) {
  return create_unit_on_map_non_interactive(
      units_state, map_updater, e_nation::english,
      UnitComposition::create(
          UnitType::create( e_unit_type::dragoon,
                            e_unit_type::petty_criminal )
              .value() ),
      where );
}

UnitId create_hardy_pioneer_on_map( UnitsState&  units_state,
                                    Coord        where,
                                    IMapUpdater& map_updater ) {
  return create_unit_on_map_non_interactive(
      units_state, map_updater, e_nation::english,
      UnitComposition::create( e_unit_type::hardy_pioneer ),
      where );
}

UnitId create_ship( UnitsState& units_state, Coord where,
                    IMapUpdater& map_updater ) {
  return create_unit_on_map_non_interactive(
      units_state, map_updater, e_nation::english,
      UnitComposition::create( e_unit_type::merchantman ),
      where );
}

UnitId create_wagon( UnitsState& units_state, Coord where,
                     IMapUpdater& map_updater ) {
  return create_unit_on_map_non_interactive(
      units_state, map_updater, e_nation::english,
      UnitComposition::create( e_unit_type::wagon_train ),
      where );
}

unordered_map<e_commodity, int> const
    kExpectedInitialColonyCommodityQuantities{
        { e_commodity::horses, 0 },
        { e_commodity::tools, 0 },
        { e_commodity::muskets, 0 },
        // ...
    };

TEST_CASE( "[colony-mgr] found_colony on land successful" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { .x = 2, .y = 2 };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state,
                                  id ) == base::valid );
  ColonyId col_id =
      found_colony( colonies_state, terrain_state, units_state,
                    id, map_updater, "colony" );
  Colony& col = colonies_state.colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == base::lookup(
                      kExpectedInitialColonyCommodityQuantities,
                      type )
                      .value_or( 0 ) );
  }
}

TEST_CASE( "[colony-mgr] found_colony strips unit" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  SECTION( "dragoon" ) {
    Coord  coord = { .x = 2, .y = 2 };
    UnitId id =
        create_dragoon_on_map( units_state, coord, map_updater );
    Unit& founder = units_state.unit_for( id );
    REQUIRE( founder.type() == e_unit_type::dragoon );
    REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                    terrain_state, id )
                 .valid() );
    ColonyId col_id =
        found_colony( colonies_state, terrain_state, units_state,
                      id, map_updater, "colony" );
    REQUIRE( founder.type() == e_unit_type::petty_criminal );
    Colony& col = colonies_state.colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::horses:
          REQUIRE( q ==
                   base::lookup(
                       kExpectedInitialColonyCommodityQuantities,
                       e_commodity::horses )
                           .value_or( 0 ) +
                       50 );
          break;
        case e_commodity::muskets:
          REQUIRE( q ==
                   base::lookup(
                       kExpectedInitialColonyCommodityQuantities,
                       e_commodity::muskets )
                           .value_or( 0 ) +
                       50 );
          break;
        default:
          REQUIRE( q ==
                   base::lookup(
                       kExpectedInitialColonyCommodityQuantities,
                       type )
                       .value_or( 0 ) );
          break;
      }
    }
  }

  SECTION( "hardy_pioneer" ) {
    Coord  coord = { .x = 2, .y = 2 };
    UnitId id = create_hardy_pioneer_on_map( units_state, coord,
                                             map_updater );
    Unit&  founder = units_state.unit_for( id );
    REQUIRE( founder.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                    terrain_state, id )
                 .valid() );
    ColonyId col_id =
        found_colony( colonies_state, terrain_state, units_state,
                      id, map_updater, "colony" );
    REQUIRE( founder.type() == e_unit_type::hardy_colonist );
    Colony& col = colonies_state.colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::tools:
          REQUIRE( q ==
                   base::lookup(
                       kExpectedInitialColonyCommodityQuantities,
                       e_commodity::tools )
                           .value_or( 0 ) +
                       100 );
          break;
        default:
          REQUIRE( q ==
                   base::lookup(
                       kExpectedInitialColonyCommodityQuantities,
                       type )
                       .value_or( 0 ) );
          break;
      }
    }
  }
}

TEST_CASE(
    "[colony-mgr] found_colony on existing colony fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { .x = 2, .y = 2 };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
  ColonyId col_id =
      found_colony( colonies_state, terrain_state, units_state,
                    id, map_updater, "colony 1" );
  Colony& col = colonies_state.colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == base::lookup(
                      kExpectedInitialColonyCommodityQuantities,
                      type )
                      .value_or( 0 ) );
  }

  id = create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id ) ==
           invalid( e_found_colony_err::colony_exists_here ) );
}

TEST_CASE(
    "[colony-mgr] found_colony with existing name fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { .x = 2, .y = 2 };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
  ColonyId col_id =
      found_colony( colonies_state, terrain_state, units_state,
                    id, map_updater, "colony" );
  Colony& col = colonies_state.colony_for( col_id );
  for( auto [type, q] : col.commodities ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == base::lookup(
                      kExpectedInitialColonyCommodityQuantities,
                      type )
                      .value_or( 0 ) );
  }

  coord.x += 2;
  id = create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
}

TEST_CASE(
    "[colony-mgr] found_colony too close to another colony "
    "fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { .x = 2, .y = 2 };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
  found_colony( colonies_state, terrain_state, units_state, id,
                map_updater, "colony" );
  coord.x += 1;
  id = create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id ) ==
           invalid( e_found_colony_err::too_close_to_colony ) );
}

TEST_CASE( "[colony-mgr] found_colony in water fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord   = { .x = 1, .y = 1 };
  auto  ship_id = create_ship( units_state, coord, map_updater );
  auto  unit_id = create_colonist( units_state );
  units_state.change_to_cargo_somewhere( ship_id, unit_id );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, unit_id ) ==
           invalid( e_found_colony_err::no_water_colony ) );
}

TEST_CASE(
    "[colony-mgr] found_colony by unit not on map fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );

  auto id = create_colonist( units_state );
  units_state.change_to_harbor_view(
      id,
      UnitHarborViewState{ .port_status = PortStatus::in_port{},
                           .sailed_from = {} } );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id ) ==
           invalid( e_found_colony_err::colonist_not_on_map ) );
}

TEST_CASE( "[colony-mgr] found_colony by ship fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { .x = 1, .y = 1 };
  auto  id    = create_ship( units_state, coord, map_updater );
  REQUIRE(
      unit_can_found_colony( colonies_state, units_state,
                             terrain_state, id ) ==
      invalid( e_found_colony_err::ship_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] found_colony by non-human fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { .x = 1, .y = 1 };
  auto  id    = create_wagon( units_state, coord, map_updater );
  REQUIRE(
      unit_can_found_colony( colonies_state, units_state,
                             terrain_state, id ) ==
      invalid(
          e_found_colony_err::non_human_cannot_found_colony ) );
}

vector<ColonyId> colonies_all(
    ColoniesState const& colonies_state ) {
  vector<ColonyId> ids;
  for( auto const& [id, colony] : colonies_state.all() )
    ids.push_back( id );
  return ids;
}

vector<ColonyId> colonies_all(
    ColoniesState const& colonies_state, e_nation nation ) {
  vector<ColonyId> ids;
  for( auto const& [id, colony] : colonies_state.all() )
    if( colony.nation == nation ) ids.push_back( id );
  return ids;
}

TEST_CASE( "[colony-mgr] create, query, destroy" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  auto xp = create_empty_colony(
      colonies_state, e_nation::english, Coord{ .x = 1, .y = 2 },
      "my colony" );
  REQUIRE( xp == ColonyId{ 1 } );

  Colony const& colony =
      colonies_state.colony_for( ColonyId{ 1 } );
  REQUIRE( colony.id == ColonyId{ 1 } );
  REQUIRE( colony.nation == e_nation::english );
  REQUIRE( colony.name == "my colony" );
  REQUIRE( colony.location == Coord{ .x = 1, .y = 2 } );

  REQUIRE( colony_exists( colonies_state, ColonyId{ 1 } ) );
  REQUIRE( !colony_exists( colonies_state, ColonyId{ 2 } ) );

  auto xp2 = create_empty_colony(
      colonies_state, e_nation::dutch, Coord{ .x = 1, .y = 3 },
      "my second colony" );
  REQUIRE( xp2 == ColonyId{ 2 } );
  REQUIRE_THAT( colonies_all( colonies_state ),
                UnorderedEquals( vector<ColonyId>{
                    ColonyId{ 1 }, ColonyId{ 2 } } ) );
  REQUIRE_THAT(
      colonies_all( colonies_state, e_nation::dutch ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 2 } } ) );
  REQUIRE_THAT(
      colonies_all( colonies_state, e_nation::english ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 1 } } ) );
  REQUIRE_THAT( colonies_all( colonies_state, e_nation::french ),
                UnorderedEquals( vector<ColonyId>{} ) );

  REQUIRE( colonies_state.maybe_from_name( "my colony" ) ==
           ColonyId{ 1 } );
  colonies_state.destroy_colony( ColonyId{ 1 } );
  REQUIRE_THAT(
      colonies_all( colonies_state ),
      UnorderedEquals( vector<ColonyId>{ ColonyId{ 2 } } ) );

  colonies_state.destroy_colony( ColonyId{ 2 } );
  REQUIRE_THAT( colonies_all( colonies_state ),
                UnorderedEquals( vector<ColonyId>{} ) );
}

TEST_CASE( "[colony-mgr] initial colony buildings." ) {
  World W;
  W.create_default_map();
  Colony& colony =
      W.add_colony_with_new_unit( Coord{ .x = 1, .y = 1 } );
  unordered_set<e_colony_building> buildings;
  for( auto const& [building, has] : colony.buildings )
    if( has ) buildings.insert( building );
  REQUIRE( buildings ==
           unordered_set<e_colony_building>{
               e_colony_building::blacksmiths_house,
               e_colony_building::carpenters_shop,
               e_colony_building::fur_traders_house,
               e_colony_building::rum_distillers_house,
               e_colony_building::tobacconists_house,
               e_colony_building::weavers_house,
               e_colony_building::town_hall,
           } );
}

TEST_CASE( "[colony-mgr] found_colony places initial unit." ) {
  World W;
  W.create_default_map();

  UnitId founder = W.add_unit_on_map( e_unit_type::free_colonist,
                                      Coord{ .x = 1, .y = 1 } );
  // Don't use W.add_colony here because we are testing
  // found_colony specifically.
  ColonyId id =
      found_colony( W.colonies(), W.terrain(), W.units(),
                    founder, W.map_updater(), "my colony" );
  Colony& colony = W.colonies().colony_for( id );

  REQUIRE( colony.outdoor_jobs[e_direction::nw] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::ne] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::w] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::e] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::sw] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::s] == nothing );
  REQUIRE( colony.outdoor_jobs[e_direction::se] == nothing );

  // Colonist should have been placed here.
  REQUIRE( colony.outdoor_jobs[e_direction::n] ==
           ( OutdoorUnit{ .unit_id = founder,
                          .job     = e_outdoor_job::food } ) );
}

TEST_CASE( "[colony-mgr] change_unit_outdoor_job." ) {
  World W;
  W.create_default_map();
  Colony& colony = W.add_colony( Coord{ .x = 1, .y = 1 } );
  // Note that the founding colonist will have been placed on the
  // north tile.
  UnitId farmer = W.add_unit_outdoors( colony.id, e_direction::w,
                                       e_outdoor_job::food );
  UnitId ore_miner = W.add_unit_outdoors(
      colony.id, e_direction::e, e_outdoor_job::ore );

  // Sanity check.
  REQUIRE( colony.outdoor_jobs[e_direction::w] ==
           ( OutdoorUnit{ .unit_id = farmer,
                          .job     = e_outdoor_job::food } ) );
  REQUIRE( colony.outdoor_jobs[e_direction::e] ==
           ( OutdoorUnit{ .unit_id = ore_miner,
                          .job     = e_outdoor_job::ore } ) );

  // Change job.
  change_unit_outdoor_job( colony, farmer,
                           e_outdoor_job::lumber );

  REQUIRE( colony.outdoor_jobs[e_direction::w] ==
           ( OutdoorUnit{ .unit_id = farmer,
                          .job     = e_outdoor_job::lumber } ) );
  REQUIRE( colony.outdoor_jobs[e_direction::e] ==
           ( OutdoorUnit{ .unit_id = ore_miner,
                          .job     = e_outdoor_job::ore } ) );
}

TEST_CASE( "[colony-mgr] found_colony finds job for unit." ) {
  // TODO
}

TEST_CASE( "[colony-mgr] evolve colonies." ) {
  // TODO
}

} // namespace
} // namespace rn
