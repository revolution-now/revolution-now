/****************************************************************
**ustate.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-17.
*
* Description: Unit tests for the src/ustate.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ustate.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/units.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

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

  inline static Coord const kLand = Coord{ .x = 1, .y = 1 };
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ustate] current_activity_for_unit" ) {
  World W;
  W.create_default_map();

  auto f = [&]( UnitId id ) {
    return current_activity_for_unit( W.units(), W.colonies(),
                                      id );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer );
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal );
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal farmer" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id     = W.add_unit_outdoors(
                     colony.id, e_direction::w, e_outdoor_job::food,
                     e_unit_type::petty_criminal );
    REQUIRE( f( id ) == e_unit_activity::farming );
  }

  SECTION( "petty_criminal no job" ) {
    UnitComposition expected;
    UnitId id = W.add_unit_on_map( e_unit_type::petty_criminal,
                                   W.kLand );
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand );
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand );
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "veteran_dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand );
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "pioneer" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::petty_criminal ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand );
    REQUIRE( f( id ) == e_unit_activity::pioneering );
  }
}

} // namespace
} // namespace rn
