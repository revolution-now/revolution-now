/****************************************************************
**ai-native-mind.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2023-03-25.
*
* Description: Unit tests for the src/ai-native-mind.* module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/ai-native-mind.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/irand.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/tribe.rds.hpp"

// Must be last.
#include "test/catch-common.hpp"

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::_;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct World : testing::World {
  using Base = testing::World;
  World() : Base() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
      _, L, _, //
      L, L, L, //
      _, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }

  // Temporary: this is for when the brave chooses a random di-
  // rection in which to move.
  void expect_random_move() {
    rand().EXPECT__between_ints( _, _ );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[ai-native-mind] equips brave over dwelling" ) {
  World             W;
  NativeUnitCommand expected;

  e_tribe const tribe_type = e_tribe::aztec;
  AiNativeMind  mind( W.ss(), W.rand(), tribe_type );

  DwellingId const dwelling_id =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe_type ).id;
  Tribe& tribe = W.tribe( tribe_type );

  SECTION( "not over dwelling does not equip" ) {
    NativeUnit& brave = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 2, .y = 2 },
        dwelling_id );
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "over dwelling no arms" ) {
    NativeUnit& brave = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling_id );
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "not over dwelling with arms does not equip" ) {
    NativeUnit& brave = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 2, .y = 2 },
        dwelling_id );
    tribe.muskets        = 1;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 25;
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "over dwelling with arms but delayed" ) {
    NativeUnit& brave = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling_id );
    tribe.muskets        = 1;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 25;
    // Probability for discoverer level to deplete muskets.
    W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
    // Delay equipping.
    W.rand().EXPECT__bernoulli( 0.08 ).returns( true );
    W.expect_random_move();

    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "over dwelling with arms equips" ) {
    NativeUnit& brave = W.add_native_unit_on_map(
        e_native_unit_type::brave, { .x = 1, .y = 1 },
        dwelling_id );
    tribe.muskets        = 1;
    tribe.horse_herds    = 2;
    tribe.horse_breeding = 25;
    // Probability for discoverer level to deplete muskets.
    W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
    // Don't delay equipping.
    W.rand().EXPECT__bernoulli( 0.08 ).returns( false );

    expected = NativeUnitCommand::equip{
      .how = EquippedBrave{
        .type          = e_native_unit_type::mounted_warrior,
        .muskets_delta = -1,
        .horse_breeding_delta = -25 } };
    REQUIRE( mind.command_for( brave.id ) == expected );
  }
}

TEST_CASE(
    "[ai-native-mind] does not de-equip brave over dwelling" ) {
  World             W;
  NativeUnitCommand expected;

  e_tribe const tribe_type = e_tribe::aztec;
  AiNativeMind  mind( W.ss(), W.rand(), tribe_type );

  DwellingId const dwelling_id =
      W.add_dwelling( { .x = 1, .y = 1 }, tribe_type ).id;
  Tribe& tribe = W.tribe( tribe_type );

  SECTION( "mounted_brave,horses=0,muskets=0" ) {
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
        dwelling_id );
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "mounted_brave,horses=25,muskets=0" ) {
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 25;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
        dwelling_id );
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "mounted_brave,horses=0,muskets=1" ) {
    tribe.muskets        = 1;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::mounted_brave, { .x = 1, .y = 1 },
        dwelling_id );
    // Probability for discoverer level to deplete muskets.
    W.rand().EXPECT__bernoulli( 1.0 ).returns( true );
    // Don't delay equipping.
    W.rand().EXPECT__bernoulli( 0.08 ).returns( false );

    expected = NativeUnitCommand::equip{
      .how = EquippedBrave{
        .type          = e_native_unit_type::mounted_warrior,
        .muskets_delta = -1,
        .horse_breeding_delta = 0 } };
    REQUIRE( mind.command_for( brave.id ) == expected );
  }

  SECTION( "armed_brave,horses=0,muskets=0" ) {
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::armed_brave, { .x = 1, .y = 1 },
        dwelling_id );
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "armed_brave,horses=0,muskets=1" ) {
    tribe.muskets        = 1;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::armed_brave, { .x = 1, .y = 1 },
        dwelling_id );
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "armed_brave,horses=25,muskets=0" ) {
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 25;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::armed_brave, { .x = 1, .y = 1 },
        dwelling_id );
    // Don't delay equipping.
    W.rand().EXPECT__bernoulli( 0.08 ).returns( false );

    expected = NativeUnitCommand::equip{
      .how = EquippedBrave{
        .type          = e_native_unit_type::mounted_warrior,
        .muskets_delta = 0,
        .horse_breeding_delta = -25 } };
    REQUIRE( mind.command_for( brave.id ) == expected );
  }

  SECTION( "mounted_warrior,horses=0,muskets=0" ) {
    tribe.muskets        = 0;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 0;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::mounted_warrior, { .x = 1, .y = 1 },
        dwelling_id );
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }

  SECTION( "mounted_warrior,horses=25,muskets=1" ) {
    tribe.muskets        = 1;
    tribe.horse_herds    = 0;
    tribe.horse_breeding = 25;
    NativeUnit& brave    = W.add_native_unit_on_map(
        e_native_unit_type::mounted_warrior, { .x = 1, .y = 1 },
        dwelling_id );
    W.expect_random_move();
    REQUIRE_FALSE( mind.command_for( brave.id )
                       .holds<NativeUnitCommand::equip>() );
  }
}

} // namespace
} // namespace rn
