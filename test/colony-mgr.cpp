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

// Revolution Now
#include "colony-mgr.hpp"
#include "cstate.hpp"
#include "game-state.hpp"
#include "gs-colonies.hpp"
#include "gs-root.hpp"
#include "gs-terrain.hpp"
#include "gs-units.hpp"
#include "lua.hpp"
#include "map-square.hpp"
#include "map-updater.hpp"
#include "ustate.hpp"
#include "utype.hpp"

// refl
#include "refl/to-str.hpp"

// base
#include "base/keyval.hpp"

// luapp
#include "luapp/state.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rn;

using ::Catch::UnorderedEquals;

MapSquare make_land_square() {
  return map_square_for_terrain( e_terrain::grassland );
}

MapSquare make_ocean_square() {
  return map_square_for_terrain( e_terrain::ocean );
}

void generate_unittest_terrain( TerrainState& terrain_state ) {
  MapSquare const L = make_land_square();
  MapSquare const O = make_ocean_square();

  NonRenderingMapUpdater map_updater( terrain_state );
  map_updater.modify_entire_map( [&]( Matrix<MapSquare>& m ) {
    m = Matrix<MapSquare>( 10_w, 10_h );
    Rect land_rect{ 2_x, 2_y, 6_w, 6_h };
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
        { e_commodity::horses, 100 },
        { e_commodity::tools, 60 },
        { e_commodity::muskets, 100 },
    };

TEST_CASE( "[colony-mgr] create colony on land successful" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { 2_x, 2_y };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state,
                                  id ) == base::valid );
  ColonyId col_id = found_colony_unsafe(
      colonies_state, terrain_state, units_state, id,
      map_updater, "colony" );
  Colony& col = colonies_state.colony_for( col_id );
  for( auto [type, q] : col.commodities() ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == base::lookup(
                      kExpectedInitialColonyCommodityQuantities,
                      type )
                      .value_or( 0 ) );
  }
}

TEST_CASE( "[colony-mgr] create colony strips unit" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  SECTION( "dragoon" ) {
    Coord  coord = { 2_x, 2_y };
    UnitId id =
        create_dragoon_on_map( units_state, coord, map_updater );
    Unit& founder = units_state.unit_for( id );
    REQUIRE( founder.type() == e_unit_type::dragoon );
    REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                    terrain_state, id )
                 .valid() );
    ColonyId col_id = found_colony_unsafe(
        colonies_state, terrain_state, units_state, id,
        map_updater, "colony" );
    REQUIRE( founder.type() == e_unit_type::petty_criminal );
    Colony& col = colonies_state.colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities() ) {
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
    Coord  coord = { 2_x, 2_y };
    UnitId id = create_hardy_pioneer_on_map( units_state, coord,
                                             map_updater );
    Unit&  founder = units_state.unit_for( id );
    REQUIRE( founder.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                    terrain_state, id )
                 .valid() );
    ColonyId col_id = found_colony_unsafe(
        colonies_state, terrain_state, units_state, id,
        map_updater, "colony" );
    REQUIRE( founder.type() == e_unit_type::hardy_colonist );
    Colony& col = colonies_state.colony_for( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities() ) {
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
    "[colony-mgr] create colony on existing colony fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { 2_x, 2_y };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
  ColonyId col_id = found_colony_unsafe(
      colonies_state, terrain_state, units_state, id,
      map_updater, "colony 1" );
  Colony& col = colonies_state.colony_for( col_id );
  for( auto [type, q] : col.commodities() ) {
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
    "[colony-mgr] create colony with existing name fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { 2_x, 2_y };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
  ColonyId col_id = found_colony_unsafe(
      colonies_state, terrain_state, units_state, id,
      map_updater, "colony" );
  Colony& col = colonies_state.colony_for( col_id );
  for( auto [type, q] : col.commodities() ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == base::lookup(
                      kExpectedInitialColonyCommodityQuantities,
                      type )
                      .value_or( 0 ) );
  }

  coord += 2_w;
  id = create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
}

TEST_CASE( "[colony-mgr] too close to another colony fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { 2_x, 2_y };
  auto  id =
      create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id )
               .valid() );
  found_colony_unsafe( colonies_state, terrain_state,
                       units_state, id, map_updater, "colony" );
  coord += 1_w;
  id = create_colonist_on_map( units_state, coord, map_updater );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, id ) ==
           invalid( e_found_colony_err::too_close_to_colony ) );
}

TEST_CASE( "[colony-mgr] create colony in water fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord   = { 1_x, 1_y };
  auto  ship_id = create_ship( units_state, coord, map_updater );
  auto  unit_id = create_colonist( units_state );
  units_state.change_to_cargo_somewhere( ship_id, unit_id );
  REQUIRE( unit_can_found_colony( colonies_state, units_state,
                                  terrain_state, unit_id ) ==
           invalid( e_found_colony_err::no_water_colony ) );
}

TEST_CASE(
    "[colony-mgr] found colony by unit not on map fails" ) {
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

TEST_CASE( "[colony-mgr] found colony by ship fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { 1_x, 1_y };
  auto  id    = create_ship( units_state, coord, map_updater );
  REQUIRE(
      unit_can_found_colony( colonies_state, units_state,
                             terrain_state, id ) ==
      invalid( e_found_colony_err::ship_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] found colony by non-human fails" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  NonRenderingMapUpdater map_updater( terrain_state );

  Coord coord = { 1_x, 1_y };
  auto  id    = create_wagon( units_state, coord, map_updater );
  REQUIRE(
      unit_can_found_colony( colonies_state, units_state,
                             terrain_state, id ) ==
      invalid(
          e_found_colony_err::non_human_cannot_found_colony ) );
}

// FIXME: this uses global state.
TEST_CASE( "[colony-mgr] lua" ) {
  lua::state& st    = lua_global_state();
  GameState::root() = {};
  generate_unittest_terrain( GameState::terrain() );
  auto script = R"(
    local coord = Coord{y=2, x=2}
    local unit_type =
        utype.UnitType.create( e.unit_type.free_colonist )
    local unit_comp = unit_composer
                     .UnitComposition
                     .create_with_type_obj( unit_type )
    unit_ = ustate.create_unit_on_map(
             e.nation.english,
             unit_comp,
             coord )
    col_id = colony_mgr.found_colony(
               unit_:id(), "New York" )
    assert( col_id )
    local colony = cstate.colony_from_id( col_id )
    assert_eq( colony:id(), 1 )
    assert_eq( colony:name(), "New York" )
    assert_eq( colony:nation(), e.nation.english )
    assert_eq( colony:location(), Coord{x=2,y=2} )
    return col_id
  )";
  REQUIRE( st.script.run_safe<ColonyId>( script ) ==
           ColonyId{ 1 } );
  REQUIRE( colony_from_id( ColonyId{ 1 } ).name() ==
           "New York" );
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
    if( colony.nation() == nation ) ids.push_back( id );
  return ids;
}

TEST_CASE( "[colony-mgr] create, query, destroy" ) {
  TerrainState  terrain_state;
  ColoniesState colonies_state;
  UnitsState    units_state;
  generate_unittest_terrain( terrain_state );
  auto xp =
      create_empty_colony( colonies_state, e_nation::english,
                           Coord{ 1_x, 2_y }, "my colony" );
  REQUIRE( xp == ColonyId{ 1 } );

  Colony const& colony =
      colonies_state.colony_for( ColonyId{ 1 } );
  REQUIRE( colony.id() == ColonyId{ 1 } );
  REQUIRE( colony.nation() == e_nation::english );
  REQUIRE( colony.name() == "my colony" );
  REQUIRE( colony.location() == Coord{ 1_x, 2_y } );

  REQUIRE( colony_exists( colonies_state, ColonyId{ 1 } ) );
  REQUIRE( !colony_exists( colonies_state, ColonyId{ 2 } ) );

  auto xp2 = create_empty_colony(
      colonies_state, e_nation::dutch, Coord{ 1_x, 3_y },
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

} // namespace
} // namespace rn
