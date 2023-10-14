/****************************************************************
**unit-ownership-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2023-09-26.
*
* Description: Unit tests for the unit-ownership module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/unit-ownership.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/ieuro-mind.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/unit-composer.hpp"
#include "src/ss/unit.hpp"
#include "src/ss/units.hpp"
#include "src/ss/woodcut.rds.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

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
    add_player( e_nation::dutch );
    add_player( e_nation::english );
    set_default_player( e_nation::dutch );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const   _ = make_ocean();
    MapSquare const   L = make_grassland();
    vector<MapSquare> tiles{
        _, L, _, L, //
        L, L, L, L, //
        _, L, L, L, //
    };
    build_map( std::move( tiles ), 4 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[unit-ownership] UnitOwnershipChanger" ) {
  World        W;
  UnitId const free_colonist_id =
      W.add_unit_on_map( e_unit_type::free_colonist,
                         { .x = 1, .y = 1 } )
          .id();
  UnitId const missionary_id =
      W.add_unit_on_map( e_unit_type::missionary,
                         { .x = 3, .y = 0 } )
          .id();
  UnitId const caravel_id =
      W.add_unit_on_map( e_unit_type::caravel,
                         { .x = 0, .y = 0 } )
          .id();

  UnitOwnership expected_free_colonist, expected_missionary,
      expected_caravel;

  auto ownership = [&]( UnitId unit_id ) {
    return as_const( W.units() ).ownership_of( unit_id );
  };

  auto o = [&]( UnitId unit_id ) {
    return UnitOwnershipChanger( W.ss(), unit_id );
  };

  expected_free_colonist =
      UnitOwnership::world{ .coord = { .x = 1, .y = 1 } };
  expected_missionary =
      UnitOwnership::world{ .coord = { .x = 3, .y = 0 } };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 0 );

  // Free.
  expected_free_colonist = UnitOwnership::free{};
  expected_missionary =
      UnitOwnership::world{ .coord = { .x = 3, .y = 0 } };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( free_colonist_id ).change_to_free();
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 0 );

  // World.
  expected_free_colonist =
      UnitOwnership::world{ .coord = { .x = 1, .y = 2 } };
  expected_missionary =
      UnitOwnership::world{ .coord = { .x = 3, .y = 0 } };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( free_colonist_id )
      .change_to_map_non_interactive( W.ts(),
                                      { .x = 1, .y = 2 } );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 0 );

  // Colony.
  Colony& colony     = W.add_colony( { .x = 1, .y = 0 } );
  using OutdoorJobsT = decltype( colony.outdoor_jobs );
  expected_free_colonist =
      UnitOwnership::colony{ .id = colony.id };
  expected_missionary =
      UnitOwnership::world{ .coord = { .x = 3, .y = 0 } };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( free_colonist_id )
      .change_to_colony(
          W.ts(), colony,
          ColonyJob::indoor{ .job = e_indoor_job::tools } );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{ free_colonist_id } );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 0 );

  // Cargo.
  expected_free_colonist =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_missionary =
      UnitOwnership::world{ .coord = { .x = 3, .y = 0 } };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( free_colonist_id )
      .change_to_cargo( caravel_id, /*starting_slot=*/0 );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 1 );

  // Dwelling.
  Dwelling& dwelling =
      W.add_dwelling( { .x = 3, .y = 1 }, e_tribe::iroquois );
  expected_free_colonist =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_missionary =
      UnitOwnership::dwelling{ .id = dwelling.id };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( missionary_id ).change_to_dwelling( dwelling.id );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 1 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           missionary_id );

  // Cargo.
  expected_free_colonist =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_missionary =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( missionary_id )
      .change_to_cargo( caravel_id, /*starting_slot=*/0 );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 2 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );

  // Sail to harbor.
  expected_free_colonist =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_missionary =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_caravel = UnitOwnership::harbor{
      .port_status = PortStatus::inbound{ .turns = 0 },
      .sailed_from = Coord{ .x = 0, .y = 0 } };
  o( caravel_id )
      .change_to_harbor( PortStatus::inbound{ .turns = 0 },
                         Coord{} );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 2 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );

  // Sail to new_world.
  expected_free_colonist =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_missionary =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_caravel = UnitOwnership::harbor{
      .port_status = PortStatus::outbound{ .turns = 4 },
      .sailed_from = Coord{ .x = 0, .y = 0 } };
  o( caravel_id )
      .change_to_harbor( PortStatus::outbound{ .turns = 4 },
                         Coord{} );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 2 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );

  // Move to port.
  expected_free_colonist =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_missionary =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_caravel = UnitOwnership::harbor{
      .port_status = PortStatus::in_port{},
      .sailed_from = Coord{ .x = 0, .y = 0 } };
  o( caravel_id )
      .change_to_harbor( PortStatus::in_port{}, Coord{} );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 2 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );

  // Destroy from colony.
  expected_free_colonist =
      UnitOwnership::colony{ .id = colony.id };
  expected_missionary =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_caravel = UnitOwnership::harbor{
      .port_status = PortStatus::in_port{},
      .sailed_from = Coord{ .x = 0, .y = 0 } };
  o( free_colonist_id )
      .change_to_colony(
          W.ts(), colony,
          ColonyJob::indoor{ .job = e_indoor_job::tools } );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{ free_colonist_id } );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );
  o( free_colonist_id ).destroy();
  REQUIRE( W.units().all().size() == 2 );
  REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 1 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );

  // Destroy from cargo.
  expected_missionary =
      UnitOwnership::cargo{ .holder = caravel_id };
  expected_caravel = UnitOwnership::harbor{
      .port_status = PortStatus::in_port{},
      .sailed_from = Coord{ .x = 0, .y = 0 } };
  o( missionary_id )
      .change_to_cargo( caravel_id, /*starting_slot=*/0 );
  REQUIRE( W.units().all().size() == 2 );
  REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( missionary_id ) == expected_missionary );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );
  o( missionary_id ).destroy();
  REQUIRE( W.units().all().size() == 1 );
  REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units()
               .unit_for( caravel_id )
               .cargo()
               .slots_occupied() == 0 );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );

  // Destroy from map.
  expected_caravel =
      UnitOwnership::world{ .coord = { .x = 0, .y = 0 } };
  o( caravel_id )
      .change_to_map_non_interactive( W.ts(),
                                      { .x = 0, .y = 0 } );
  REQUIRE( W.units().all().size() == 1 );
  REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( ownership( caravel_id ) == expected_caravel );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );
  o( caravel_id ).destroy();
  REQUIRE( W.units().all().size() == 0 );
  REQUIRE_FALSE( W.units().exists( free_colonist_id ) );
  REQUIRE_FALSE( W.units().exists( missionary_id ) );
  REQUIRE_FALSE( W.units().exists( caravel_id ) );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::tools] ==
           vector<UnitId>{} );
  REQUIRE( W.units().missionary_from_dwelling( dwelling.id ) ==
           nothing );
}

TEST_CASE( "[unit-ownership] missionary_from_dwelling" ) {
  World         W;
  maybe<UnitId> expected;

  Dwelling const& dwelling1 =
      W.add_dwelling( { .x = 1, .y = 1 }, e_tribe::apache );
  Dwelling const& dwelling2 =
      W.add_dwelling( { .x = 1, .y = 2 }, e_tribe::inca );

  expected = nothing;
  REQUIRE( W.units().missionary_from_dwelling( dwelling1.id ) ==
           expected );
  expected = nothing;
  REQUIRE( W.units().missionary_from_dwelling( dwelling2.id ) ==
           expected );

  Unit const& unit = W.add_missionary_in_dwelling(
      e_unit_type::missionary, dwelling1.id );
  REQUIRE( as_const( W.units() ).ownership_of( unit.id() ) ==
           UnitOwnership::dwelling{ .id = dwelling1.id } );
  REQUIRE( W.units().maybe_dwelling_for_missionary(
               unit.id() ) == dwelling1.id );

  expected = unit.id();
  REQUIRE( W.units().missionary_from_dwelling( dwelling1.id ) ==
           expected );
  expected = nothing;
  REQUIRE( W.units().missionary_from_dwelling( dwelling2.id ) ==
           expected );

  UnitOwnershipChanger( W.ss(), unit.id() ).change_to_free();
  REQUIRE( as_const( W.units() ).ownership_of( unit.id() ) ==
           UnitOwnership::free{} );
  REQUIRE( W.units().maybe_dwelling_for_missionary(
               unit.id() ) == nothing );

  expected = nothing;
  REQUIRE( W.units().missionary_from_dwelling( dwelling1.id ) ==
           expected );
  expected = nothing;
  REQUIRE( W.units().missionary_from_dwelling( dwelling2.id ) ==
           expected );
}

TEST_CASE( "[unit-ownership] change_to_map" ) {
  World       W;
  Unit const& unit =
      W.add_free_unit( e_unit_type::free_colonist );
  UnitOwnership expected;
  Coord         target;

  auto ownership = [&] {
    return as_const( W.units() ).ownership_of( unit.id() );
  };

  auto f = [&] {
    maybe<UnitDeleted> const unit_deleted =
        co_await_test( UnitOwnershipChanger( W.ss(), unit.id() )
                           .change_to_map( W.ts(), target ) );
    BASE_CHECK( !unit_deleted.has_value() );
  };

  // World.
  expected = UnitOwnership::world{ .coord = { .x = 0, .y = 1 } };
  // We'll allow these async events to trigger so that we can
  // verify that we're calling the interactive version of the
  // function that moves a unit onto a map square.
  W.euro_mind().EXPECT__show_woodcut(
      e_woodcut::discovered_new_world );
  // Player asked to name the new world.
  W.gui().EXPECT__string_input( _, _ ).returns<maybe<string>>(
      "my land" );
  target = { .x = 0, .y = 1 };
  f();
  REQUIRE( ownership() == expected );
}

TEST_CASE( "[unit-ownership] reinstate_on_map_if_on_map" ) {
  World W;

  Coord const coord       = { .x = 1, .y = 1 };
  Coord const other_coord = { .x = 2, .y = 1 };
  Colony&     colony      = W.add_colony( coord );
  using OutdoorJobsT      = decltype( colony.outdoor_jobs );

  UnitId const worker_id =
      W.add_unit_indoors( colony.id, e_indoor_job::bells ).id();
  UnitId const free_colonist_id =
      W.add_unit_on_map( e_unit_type::free_colonist, coord )
          .id();
  UnitId const other_id =
      W.add_unit_on_map( e_unit_type::free_colonist, other_coord,
                         e_nation::english )
          .id();
  W.units().unit_for( other_id ).sentry();

  UnitOwnership expected_free_colonist, expected_worker,
      expected_other;

  auto ownership = [&]( UnitId unit_id ) {
    return as_const( W.units() ).ownership_of( unit_id );
  };

  auto f = [&]( UnitId unit_id ) {
    UnitOwnershipChanger( W.ss(), unit_id )
        .reinstate_on_map_if_on_map( W.ts() );
  };

  expected_free_colonist =
      UnitOwnership::world{ .coord = coord };
  expected_worker = UnitOwnership::colony{ .id = colony.id };
  expected_other  = UnitOwnership::world{ .coord = other_coord };
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( worker_id ) );
  REQUIRE( W.units().exists( other_id ) );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( worker_id ) == expected_worker );
  REQUIRE( ownership( other_id ) == expected_other );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::bells] ==
           vector<UnitId>{ worker_id } );
  REQUIRE( W.units()
               .unit_for( other_id )
               .orders()
               .holds<unit_orders::sentry>() );

  f( worker_id );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( worker_id ) );
  REQUIRE( W.units().exists( other_id ) );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( worker_id ) == expected_worker );
  REQUIRE( ownership( other_id ) == expected_other );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::bells] ==
           vector<UnitId>{ worker_id } );
  REQUIRE( W.units()
               .unit_for( other_id )
               .orders()
               .holds<unit_orders::sentry>() );

  f( free_colonist_id );
  REQUIRE( W.units().all().size() == 3 );
  REQUIRE( W.units().exists( free_colonist_id ) );
  REQUIRE( W.units().exists( worker_id ) );
  REQUIRE( W.units().exists( other_id ) );
  REQUIRE( ownership( free_colonist_id ) ==
           expected_free_colonist );
  REQUIRE( ownership( worker_id ) == expected_worker );
  REQUIRE( ownership( other_id ) == expected_other );
  REQUIRE( colony.outdoor_jobs == OutdoorJobsT{} );
  REQUIRE( colony.indoor_jobs[e_indoor_job::bells] ==
           vector<UnitId>{ worker_id } );
  // Use the change in orders of the foreign unit next to the
  // free colonist as an indicator that we've actually re-placed
  // the unit, since doing so should unsentry all foreign units
  // in its surroundings.
  REQUIRE( W.units()
               .unit_for( other_id )
               .orders()
               .holds<unit_orders::none>() );
}

TEST_CASE( "[unit-ownership] destroy (native)" ) {
  World            W;
  DwellingId const dwelling_id =
      W.add_dwelling( { .x = 2, .y = 2 }, e_tribe::tupi ).id;
  NativeUnitId const brave_id =
      W.add_native_unit_on_map( e_native_unit_type::brave,
                                { .x = 0, .y = 0 }, dwelling_id )
          .id;
  NativeUnitId const mounted_brave_id =
      W.add_native_unit_on_map(
           e_native_unit_type::mounted_brave, { .x = 1, .y = 0 },
           dwelling_id )
          .id;
  NativeUnitId const armed_brave_id =
      W.add_native_unit_on_map( e_native_unit_type::armed_brave,
                                { .x = 0, .y = 1 }, dwelling_id )
          .id;

  REQUIRE( W.units().exists( brave_id ) );
  REQUIRE( W.units().exists( mounted_brave_id ) );
  REQUIRE( W.units().exists( armed_brave_id ) );

  NativeUnitOwnershipChanger( W.ss(), armed_brave_id ).destroy();

  REQUIRE( W.units().exists( brave_id ) );
  REQUIRE( W.units().exists( mounted_brave_id ) );
  REQUIRE_FALSE( W.units().exists( armed_brave_id ) );

  NativeUnitOwnershipChanger( W.ss(), brave_id ).destroy();

  REQUIRE_FALSE( W.units().exists( brave_id ) );
  REQUIRE( W.units().exists( mounted_brave_id ) );
  REQUIRE_FALSE( W.units().exists( armed_brave_id ) );
}

} // namespace
} // namespace rn
