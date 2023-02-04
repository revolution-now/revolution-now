/****************************************************************
**unit-mgr.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-06-17.
*
* Description: Unit tests for the src/unit-mgr.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unit-mgr.hpp"

// Testing.
#include "test/fake/world.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/ref.hpp"
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
  World() : Base() {
    add_player( e_nation::dutch );
    create_default_map();
  }

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
TEST_CASE( "[unit-mgr] current_activity_for_unit" ) {
  World W;

  auto f = [&]( UnitId id ) {
    return current_activity_for_unit( W.units(), W.colonies(),
                                      id );
  };

  SECTION( "expert_farmer carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::expert_farmer )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal carpentry" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_indoors( colony.id, e_indoor_job::hammers,
                            e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::carpentry );
  }

  SECTION( "petty_criminal farmer" ) {
    UnitComposition expected;
    Colony&         colony = W.add_colony( W.kLand );
    UnitId          id =
        W.add_unit_outdoors( colony.id, e_direction::w,
                             e_outdoor_job::food,
                             e_unit_type::petty_criminal )
            .id();
    REQUIRE( f( id ) == e_unit_activity::farming );
  }

  SECTION( "petty_criminal no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::petty_criminal, W.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer no job" ) {
    UnitComposition expected;
    UnitId          id =
        W.add_unit_on_map( e_unit_type::expert_farmer, W.kLand )
            .id();
    REQUIRE( f( id ) == nothing );
  }

  SECTION( "expert_farmer dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "veteran_dragoon" ) {
    UnitComposition expected;
    UNWRAP_CHECK( initial_ut, UnitType::create(
                                  e_unit_type::dragoon,
                                  e_unit_type::expert_farmer ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::fighting );
  }

  SECTION( "pioneer" ) {
    UnitComposition expected;
    UNWRAP_CHECK(
        initial_ut,
        UnitType::create( e_unit_type::pioneer,
                          e_unit_type::petty_criminal ) );
    UnitId id = W.add_unit_on_map( initial_ut, W.kLand ).id();
    REQUIRE( f( id ) == e_unit_activity::pioneering );
  }

  SECTION( "missionary" ) {
    UnitComposition expected;
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id =
        W.add_missionary_in_dwelling(
             UnitType::create( e_unit_type::missionary ),
             dwelling.id )
            .id();
    REQUIRE( f( id ) == e_unit_activity::missioning );
  }
}

TEST_CASE( "[unit-mgr] tribe_for_unit" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::arawak );
  NativeUnit const& unit = W.add_native_unit_on_map(
      e_native_unit_type::mounted_brave, { .x = 0, .y = 0 },
      dwelling.id );
  REQUIRE( tribe_for_unit( W.ss(), unit ) == e_tribe::arawak );
}

TEST_CASE( "[unit-mgr] coord_for_unit_multi_ownership" ) {
  World W;

  SECTION( "colonist in colony" ) {
    W.add_colony_with_new_unit( { .x = 1, .y = 1 } );
    UnitId const id{ 1 };
    REQUIRE(
        !coord_for_unit_indirect( W.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( W.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }

  SECTION( "missionary in dwelling" ) {
    Dwelling const& dwelling =
        W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::sioux );
    UnitId const id =
        W.add_missionary_in_dwelling(
             UnitType::create( e_unit_type::missionary ),
             dwelling.id )
            .id();
    REQUIRE(
        !coord_for_unit_indirect( W.units(), id ).has_value() );
    REQUIRE( coord_for_unit_multi_ownership( W.ss(), id ) ==
             Coord{ .x = 1, .y = 1 } );
  }
}

} // namespace
} // namespace rn
