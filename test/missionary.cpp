/****************************************************************
**missionary.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-05.
*
* Description: Unit tests for the src/missionary.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/missionary.hpp"

// Testing
#include "test/fake/world.hpp"

// ss
#include "src/ss/units.hpp"

// refl
#include "refl/to-str.hpp"

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
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{ L };
    build_map( std::move( tiles ), 1 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[missionary] can_bless_missionaries" ) {
  World   W;
  Colony& colony = W.add_colony_with_new_unit( Coord{} );

  // First with no buildings.
  REQUIRE( can_bless_missionaries( colony ) == false );

  // Next with all buildings except for church/cathedral.
  W.give_all_buildings( colony );
  colony.buildings[e_colony_building::cathedral] = false;
  colony.buildings[e_colony_building::church]    = false;
  REQUIRE( can_bless_missionaries( colony ) == false );

  // Now with church only.
  colony.buildings[e_colony_building::church] = true;
  REQUIRE( can_bless_missionaries( colony ) == true );

  // Now with cathedral.
  colony.buildings[e_colony_building::cathedral] = true;
  REQUIRE( can_bless_missionaries( colony ) == true );

  // Now with cathedral only.
  colony.buildings[e_colony_building::church] = false;
  REQUIRE( can_bless_missionaries( colony ) == true );

  // Round trip.
  colony.buildings[e_colony_building::cathedral] = false;
  REQUIRE( can_bless_missionaries( colony ) == false );
}

TEST_CASE( "[missionary] unit_can_be_blessed" ) {
  auto f = []( e_unit_type type ) {
    return unit_can_be_blessed( UnitType::create( type ) );
  };

  REQUIRE( f( e_unit_type::jesuit_colonist ) == true );
  REQUIRE( f( e_unit_type::jesuit_missionary ) == true );

  REQUIRE( f( e_unit_type::free_colonist ) == true );
  REQUIRE( f( e_unit_type::petty_criminal ) == true );
  REQUIRE( f( e_unit_type::indentured_servant ) == true );

  REQUIRE( f( e_unit_type::expert_farmer ) == true );
  REQUIRE( f( e_unit_type::master_gunsmith ) == true );

  REQUIRE( f( e_unit_type::dragoon ) == true );
  REQUIRE( f( e_unit_type::veteran_dragoon ) == true );

  REQUIRE( f( e_unit_type::regular ) == true );

  REQUIRE( f( e_unit_type::artillery ) == false );
  REQUIRE( f( e_unit_type::caravel ) == false );
  REQUIRE( f( e_unit_type::treasure ) == false );
}

TEST_CASE( "[missionary] bless_as_missionary" ) {
  World    W;
  Colony&  colony = W.add_colony_with_new_unit( Coord{} );
  UnitType input, expected;

  auto f = [&]( UnitType type ) {
    UnitId unit_id = W.add_unit_on_map( type, Coord{} ).id();
    bless_as_missionary( W.default_player(), colony,
                         W.units().unit_for( unit_id ) );
    return W.units().unit_for( unit_id ).type_obj();
  };

  // free_colonist/free_colonist.
  input = UnitType::create( e_unit_type::free_colonist,
                            e_unit_type::free_colonist )
              .value();
  expected = UnitType::create( e_unit_type::missionary,
                               e_unit_type::free_colonist )
                 .value();
  REQUIRE( f( input ) == expected );

  // missionary/petty_criminal.
  input = UnitType::create( e_unit_type::missionary,
                            e_unit_type::petty_criminal )
              .value();
  expected = UnitType::create( e_unit_type::missionary,
                               e_unit_type::petty_criminal )
                 .value();
  REQUIRE( f( input ) == expected );

  // petty_criminal/petty_criminal.
  input = UnitType::create( e_unit_type::petty_criminal,
                            e_unit_type::petty_criminal )
              .value();
  expected = UnitType::create( e_unit_type::missionary,
                               e_unit_type::petty_criminal )
                 .value();
  REQUIRE( f( input ) == expected );

  // expert_farmer/expert_farmer.
  input = UnitType::create( e_unit_type::expert_farmer,
                            e_unit_type::expert_farmer )
              .value();
  expected = UnitType::create( e_unit_type::missionary,
                               e_unit_type::expert_farmer )
                 .value();
  REQUIRE( f( input ) == expected );

  // jesuit_colonist/jesuit_colonist.
  input = UnitType::create( e_unit_type::jesuit_colonist,
                            e_unit_type::jesuit_colonist )
              .value();
  expected = UnitType::create( e_unit_type::jesuit_missionary,
                               e_unit_type::jesuit_colonist )
                 .value();
  REQUIRE( f( input ) == expected );

  // jesuit_missionary/jesuit_colonist.
  input = UnitType::create( e_unit_type::jesuit_missionary,
                            e_unit_type::jesuit_colonist )
              .value();
  expected = UnitType::create( e_unit_type::jesuit_missionary,
                               e_unit_type::jesuit_colonist )
                 .value();
  REQUIRE( f( input ) == expected );

  for( e_commodity c : refl::enum_values<e_commodity> ) {
    INFO( fmt::format( "c: {}", c ) );
    REQUIRE( colony.commodities[c] == 0 );
  }

  // dragoon/jesuit_colonist.
  input = UnitType::create( e_unit_type::dragoon,
                            e_unit_type::jesuit_colonist )
              .value();
  expected = UnitType::create( e_unit_type::jesuit_missionary,
                               e_unit_type::jesuit_colonist )
                 .value();
  REQUIRE( f( input ) == expected );

  for( e_commodity c : refl::enum_values<e_commodity> ) {
    INFO( fmt::format( "c: {}", c ) );
    if( c == e_commodity::horses || c == e_commodity::muskets ) {
      REQUIRE( colony.commodities[c] == 50 );
    } else {
      REQUIRE( colony.commodities[c] == 0 );
    }
  }
}

} // namespace
} // namespace rn
