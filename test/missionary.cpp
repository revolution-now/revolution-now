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
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/tribe.rds.hpp"
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
    add_player( e_nation::french );
    set_default_player( e_nation::dutch );
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        L, L, L, L, //
        L, L, L, L, //
        L, L, L, L, //
        L, L, L, L, //
    };
    build_map( std::move( tiles ), 5 );
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

TEST_CASE( "[missionary] is_missionary" ) {
  REQUIRE( is_missionary( e_unit_type::missionary ) );
  REQUIRE( is_missionary( e_unit_type::jesuit_missionary ) );

  REQUIRE_FALSE( is_missionary( e_unit_type::jesuit_colonist ) );
  REQUIRE_FALSE( is_missionary( e_unit_type::free_colonist ) );
  REQUIRE_FALSE( is_missionary( e_unit_type::petty_criminal ) );
}

TEST_CASE(
    "[missionary] "
    "probability_dwelling_produces_convert_on_attack" ) {
  World           W;
  Dwelling const& dwelling =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::cherokee );
  maybe<double> expected;

  auto f = [&] {
    // The dutch is attacking.
    return probability_dwelling_produces_convert_on_attack(
        W.ss(), W.dutch(), dwelling.id );
  };

  SECTION( "no missionary" ) {
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "foreign missionary" ) {
    UnitType const type =
        UnitType::create( e_unit_type::jesuit_missionary );
    W.add_missionary_in_dwelling( type, dwelling.id,
                                  e_nation::french );
    expected = nothing;
    REQUIRE( f() == expected );
  }

  SECTION( "friendly missionary (criminal)" ) {
    UnitType const type =
        UnitType::create( e_unit_type::missionary,
                          e_unit_type::petty_criminal )
            .value();
    W.add_missionary_in_dwelling( type, dwelling.id,
                                  e_nation::dutch );
    expected = .11;
    REQUIRE( f() == expected );
  }

  SECTION( "friendly missionary (servant)" ) {
    UnitType const type =
        UnitType::create( e_unit_type::missionary,
                          e_unit_type::indentured_servant )
            .value();
    W.add_missionary_in_dwelling( type, dwelling.id,
                                  e_nation::dutch );
    expected = .22;
    REQUIRE( f() == expected );
  }

  SECTION( "friendly missionary (free_colonist)" ) {
    UnitType const type =
        UnitType::create( e_unit_type::missionary,
                          e_unit_type::free_colonist )
            .value();
    W.add_missionary_in_dwelling( type, dwelling.id,
                                  e_nation::dutch );
    expected = .33;
    REQUIRE( f() == expected );
  }

  SECTION( "friendly missionary (expert_farmer)" ) {
    UnitType const type =
        UnitType::create( e_unit_type::missionary,
                          e_unit_type::expert_farmer )
            .value();
    W.add_missionary_in_dwelling( type, dwelling.id,
                                  e_nation::dutch );
    expected = .33;
    REQUIRE( f() == expected );
  }

  SECTION( "friendly missionary (jesuit_missionary)" ) {
    UnitType const type =
        UnitType::create( e_unit_type::jesuit_missionary );
    W.add_missionary_in_dwelling( type, dwelling.id,
                                  e_nation::dutch );
    expected = .66;
    REQUIRE( f() == expected );
  }
}

TEST_CASE( "[missionary] should_burn_mission_on_attack" ) {
  World W;
  auto  f = [&]( int alarm ) {
    return should_burn_mission_on_attack( W.rand(), alarm );
  };

  REQUIRE_FALSE( f( 0 ) );
  REQUIRE_FALSE( f( 20 ) );
  REQUIRE_FALSE( f( 40 ) );
  REQUIRE_FALSE( f( 60 ) );
  REQUIRE_FALSE( f( 84 ) );

  EXPECT_CALL( W.rand(), bernoulli( .5 ) ).returns( false );
  REQUIRE_FALSE( f( 85 ) );
  EXPECT_CALL( W.rand(), bernoulli( .5 ) ).returns( true );
  REQUIRE( f( 85 ) );

  EXPECT_CALL( W.rand(), bernoulli( .5 ) ).returns( false );
  REQUIRE_FALSE( f( 90 ) );
  EXPECT_CALL( W.rand(), bernoulli( .5 ) ).returns( true );
  REQUIRE( f( 90 ) );

  EXPECT_CALL( W.rand(), bernoulli( .5 ) ).returns( false );
  REQUIRE_FALSE( f( 99 ) );
  EXPECT_CALL( W.rand(), bernoulli( .5 ) ).returns( true );
  REQUIRE( f( 99 ) );
}

TEST_CASE( "[missionary] player_missionaries_in_tribe" ) {
  World W;
  using V = vector<UnitId>;

  W.add_tribe( e_tribe::apache );
  W.add_tribe( e_tribe::cherokee );

  auto f = [&]( e_nation nation, e_tribe tribe ) {
    return player_missionaries_in_tribe(
        W.ss(), W.player( nation ), tribe );
  };

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  DwellingId dwelling1_id =
      W.add_dwelling( { .x = 0, .y = 0 }, e_tribe::apache ).id;

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  UnitId missionary1_id =
      W.add_missionary_in_dwelling(
           UnitType::create( e_unit_type::missionary ),
           dwelling1_id, e_nation::dutch )
          .id();

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  DwellingId dwelling2_id =
      W.add_dwelling( { .x = 1, .y = 0 }, e_tribe::apache ).id;

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  UnitId missionary2_id =
      W.add_missionary_in_dwelling(
           UnitType::create( e_unit_type::missionary ),
           dwelling2_id, e_nation::dutch )
          .id();

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id, missionary2_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  DwellingId dwelling3_id =
      W.add_dwelling( { .x = 2, .y = 0 }, e_tribe::cherokee ).id;

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id, missionary2_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  UnitId missionary3_id =
      W.add_missionary_in_dwelling(
           UnitType::create( e_unit_type::jesuit_missionary ),
           dwelling3_id, e_nation::dutch )
          .id();

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id, missionary2_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) ==
           V{ missionary3_id } );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  DwellingId dwelling4_id =
      W.add_dwelling( { .x = 3, .y = 0 }, e_tribe::cherokee ).id;

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id, missionary2_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) ==
           V{ missionary3_id } );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );

  UnitId missionary4_id =
      W.add_missionary_in_dwelling(
           UnitType::create( e_unit_type::jesuit_missionary ),
           dwelling4_id, e_nation::french )
          .id();

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary1_id, missionary2_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) ==
           V{ missionary3_id } );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) ==
           V{ missionary4_id } );

  W.units().destroy_unit( missionary1_id );
  W.units().destroy_unit( missionary3_id );

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) ==
           V{ missionary2_id } );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) ==
           V{ missionary4_id } );

  W.units().destroy_unit( missionary2_id );
  W.units().destroy_unit( missionary4_id );

  REQUIRE( f( e_nation::dutch, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::dutch, e_tribe::cherokee ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::apache ) == V{} );
  REQUIRE( f( e_nation::french, e_tribe::cherokee ) == V{} );
}

TEST_CASE( "[missionary] tribe_reaction_to_missionary" ) {
  World              W;
  Tribe&             tribe = W.add_tribe( e_tribe::inca );
  TribeRelationship& relationship =
      tribe.relationship[W.default_nation()].emplace();

  auto f = [&] {
    return tribe_reaction_to_missionary( W.default_player(),
                                         tribe );
  };

  relationship.tribal_alarm = 0;
  REQUIRE( f() == e_missionary_reaction::curiosity );
  relationship.tribal_alarm = 24;
  REQUIRE( f() == e_missionary_reaction::curiosity );

  relationship.tribal_alarm = 25;
  REQUIRE( f() == e_missionary_reaction::cautious );
  relationship.tribal_alarm = 49;
  REQUIRE( f() == e_missionary_reaction::cautious );

  relationship.tribal_alarm = 50;
  REQUIRE( f() == e_missionary_reaction::offended );
  relationship.tribal_alarm = 74;
  REQUIRE( f() == e_missionary_reaction::offended );

  relationship.tribal_alarm = 75;
  REQUIRE( f() == e_missionary_reaction::hostility );
  relationship.tribal_alarm = 99;
  REQUIRE( f() == e_missionary_reaction::hostility );
}

} // namespace
} // namespace rn
