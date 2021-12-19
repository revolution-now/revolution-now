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
#include "lua.hpp"
#include "terrain.hpp"
#include "ustate.hpp"
#include "utype.hpp"

// luapp
#include "luapp/state.hpp"

// Must be last.
#include "catch-common.hpp"

namespace rn {
namespace {

using namespace std;
using namespace rn;

void init_game_world_for_test() {
  testing::default_construct_all_game_state();
  generate_unittest_terrain();
}

UnitId create_colonist_on_map( Coord where ) {
  return create_unit_on_map(
      e_nation::english,
      UnitComposition::create( e_unit_type::free_colonist ),
      where );
}

UnitId create_colonist() {
  return create_unit(
      e_nation::english,
      UnitType::create( e_unit_type::free_colonist ) );
}

UnitId create_dragoon_on_map( Coord where ) {
  return create_unit_on_map(
      e_nation::english,
      UnitComposition::create(
          UnitType::create( e_unit_type::dragoon,
                            e_unit_type::petty_criminal )
              .value() ),
      where );
}

UnitId create_hardy_pioneer_on_map( Coord where ) {
  return create_unit_on_map(
      e_nation::english,
      UnitComposition::create( e_unit_type::hardy_pioneer ),
      where );
}

UnitId create_ship( Coord where ) {
  return create_unit_on_map(
      e_nation::english,
      UnitComposition::create( e_unit_type::merchantman ),
      where );
}

UnitId create_wagon( Coord where ) {
  return create_unit_on_map(
      e_nation::english,
      UnitComposition::create( e_unit_type::wagon_train ),
      where );
}

TEST_CASE( "[colony-mgr] create colony on land successful" ) {
  init_game_world_for_test();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist_on_map( coord );
  REQUIRE( unit_can_found_colony( id ).valid() );
  ColonyId col_id = found_colony_unsafe( id, "colony" );
  Colony&  col    = colony_from_id( col_id );
  for( auto [type, q] : col.commodities() ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }
}

TEST_CASE( "[colony-mgr] create colony strips unit" ) {
  init_game_world_for_test();

  SECTION( "dragoon" ) {
    Coord  coord   = { 2_x, 2_y };
    UnitId id      = create_dragoon_on_map( coord );
    Unit&  founder = unit_from_id( id );
    REQUIRE( founder.type() == e_unit_type::dragoon );
    REQUIRE( unit_can_found_colony( id ).valid() );
    ColonyId col_id = found_colony_unsafe( id, "colony" );
    REQUIRE( founder.type() == e_unit_type::petty_criminal );
    Colony& col = colony_from_id( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities() ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::horses: REQUIRE( q == 50 ); break;
        case e_commodity::muskets: REQUIRE( q == 50 ); break;
        default: REQUIRE( q == 0 ); break;
      }
    }
  }

  SECTION( "hardy_pioneer" ) {
    Coord  coord   = { 2_x, 2_y };
    UnitId id      = create_hardy_pioneer_on_map( coord );
    Unit&  founder = unit_from_id( id );
    REQUIRE( founder.type() == e_unit_type::hardy_pioneer );
    REQUIRE( unit_can_found_colony( id ).valid() );
    ColonyId col_id = found_colony_unsafe( id, "colony" );
    REQUIRE( founder.type() == e_unit_type::hardy_colonist );
    Colony& col = colony_from_id( col_id );
    // Make sure that the founding unit has shed all of its com-
    // modities into the colony commodity store.
    for( auto [type, q] : col.commodities() ) {
      INFO( fmt::format( "type: {}, q: {}", type, q ) );
      switch( type ) {
        case e_commodity::tools: REQUIRE( q == 100 ); break;
        default: REQUIRE( q == 0 ); break;
      }
    }
  }
}

TEST_CASE(
    "[colony-mgr] create colony on existing colony fails" ) {
  init_game_world_for_test();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist_on_map( coord );
  REQUIRE( unit_can_found_colony( id ).valid() );
  ColonyId col_id = found_colony_unsafe( id, "colony 1" );
  Colony&  col    = colony_from_id( col_id );
  for( auto [type, q] : col.commodities() ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }

  id = create_colonist_on_map( coord );
  REQUIRE( unit_can_found_colony( id ) ==
           invalid( e_found_colony_err::colony_exists_here ) );
}

TEST_CASE(
    "[colony-mgr] create colony with existing name fails" ) {
  init_game_world_for_test();

  Coord coord = { 2_x, 2_y };
  auto  id    = create_colonist_on_map( coord );
  REQUIRE( unit_can_found_colony( id ).valid() );
  ColonyId col_id = found_colony_unsafe( id, "colony" );
  Colony&  col    = colony_from_id( col_id );
  for( auto [type, q] : col.commodities() ) {
    INFO( fmt::format( "type: {}, q: {}", type, q ) );
    REQUIRE( q == 0 );
  }

  coord += 1_w;
  id = create_colonist_on_map( coord );
  REQUIRE( unit_can_found_colony( id ).valid() );
}

TEST_CASE( "[colony-mgr] create colony in water fails" ) {
  init_game_world_for_test();

  Coord coord   = { 1_x, 1_y };
  auto  ship_id = create_ship( coord );
  auto  unit_id = create_colonist();
  ustate_change_to_cargo_somewhere( ship_id, unit_id );
  REQUIRE( unit_can_found_colony( unit_id ) ==
           invalid( e_found_colony_err::no_water_colony ) );
}

TEST_CASE(
    "[colony-mgr] found colony by unit not on map fails" ) {
  init_game_world_for_test();

  auto id = create_colonist();
  ustate_change_to_old_world_view(
      id, UnitOldWorldViewState::in_port{} );
  REQUIRE( unit_can_found_colony( id ) ==
           invalid( e_found_colony_err::colonist_not_on_map ) );
}

TEST_CASE( "[colony-mgr] found colony by ship fails" ) {
  init_game_world_for_test();

  Coord coord = { 1_x, 1_y };
  auto  id    = create_ship( coord );
  REQUIRE(
      unit_can_found_colony( id ) ==
      invalid( e_found_colony_err::ship_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] found colony by non-human fails" ) {
  init_game_world_for_test();

  Coord coord = { 1_x, 1_y };
  auto  id    = create_wagon( coord );
  REQUIRE(
      unit_can_found_colony( id ) ==
      invalid(
          e_found_colony_err::non_human_cannot_found_colony ) );
}

TEST_CASE( "[colony-mgr] lua" ) {
  lua::state& st = lua_global_state();
  init_game_world_for_test();
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

} // namespace
} // namespace rn
