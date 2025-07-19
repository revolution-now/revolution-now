/****************************************************************
**command-dump.cpp
*
* Project: Revolution Now
*
* Created by dsicilia on 2022-07-09.
*
* Description: Unit tests for the src/command-dump.* module.
*
*****************************************************************/
#include "test/mocking.hpp"
#include "test/testing.hpp"

// Under test.
#include "src/command-dump.hpp"

// Testing
#include "test/fake/world.hpp"
#include "test/mocks/iagent.hpp"
#include "test/mocks/iengine.hpp"

// Revolution Now
#include "src/commodity.hpp"
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
    add_default_player();
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
** Test Cases
*****************************************************************/
TEST_CASE( "[command-dump] galleon" ) {
  World w;
  MockIAgent& agent = w.agent();
  UnitId id         = w.add_unit_on_map( e_unit_type::galleon,
                                         Coord{ .x = 0, .y = 0 } )
                  .id();
  unique_ptr<CommandHandler> handler =
      handle_command( w.engine(), w.ss(), w.ts(), agent,
                      w.default_player(), id, command::dump{} );
  Unit& unit       = w.units().unit_for( id );
  CargoHold& cargo = unit.cargo();
  Commodity comm;
  int slot = 0;

  // 23 cotton in slot 0.
  comm = { .type = e_commodity::cotton, .quantity = 23 };
  slot = 0;
  add_commodity_to_cargo( w.units(), comm, unit.cargo(), slot,
                          /*try_other_slots=*/false );

  // 100 tools in slot 2.
  comm = { .type = e_commodity::tools, .quantity = 100 };
  slot = 2;
  add_commodity_to_cargo( w.units(), comm, unit.cargo(), slot,
                          /*try_other_slots=*/false );

  // 50 horses in slot 4.
  comm = { .type = e_commodity::horses, .quantity = 50 };
  slot = 4;
  add_commodity_to_cargo( w.units(), comm, unit.cargo(), slot,
                          /*try_other_slots=*/false );

  // 1 musket in slot 5.
  comm = { .type = e_commodity::muskets, .quantity = 1 };
  slot = 5;
  add_commodity_to_cargo( w.units(), comm, unit.cargo(), slot,
                          /*try_other_slots=*/false );

  // Sanity check.
  REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
           Cargo::commodity{
             .obj = Commodity{ .type     = e_commodity::cotton,
                               .quantity = 23 } } );
  REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
  REQUIRE( cargo.cargo_starting_at_slot( 2 ) ==
           Cargo::commodity{
             .obj = Commodity{ .type     = e_commodity::tools,
                               .quantity = 100 } } );
  REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
  REQUIRE( cargo.cargo_starting_at_slot( 4 ) ==
           Cargo::commodity{
             .obj = Commodity{ .type     = e_commodity::horses,
                               .quantity = 50 } } );
  REQUIRE( cargo.cargo_starting_at_slot( 5 ) ==
           Cargo::commodity{
             .obj = Commodity{ .type     = e_commodity::muskets,
                               .quantity = 1 } } );

  // Dump #1, slot 4.
  {
    using enum e_commodity;
    map<int /*slot*/, Commodity> comms;
    comms[0] = { .type = cotton, .quantity = 23 };
    comms[2] = { .type = tools, .quantity = 100 };
    comms[4] = { .type = horses, .quantity = 50 };
    comms[5] = { .type = muskets, .quantity = 1 };
    agent.EXPECT__pick_dump_cargo( comms ).returns( 2 );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    // Not yet removed.
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::tools,
                                 .quantity = 100 } } );

    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::cotton,
                                 .quantity = 23 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::horses,
                                 .quantity = 50 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type = e_commodity::muskets,
                                 .quantity = 1 } } );
  }

  // Dump #2, slot 5.
  {
    using enum e_commodity;
    map<int /*slot*/, Commodity> comms;
    comms[0] = { .type = cotton, .quantity = 23 };
    comms[4] = { .type = horses, .quantity = 50 };
    comms[5] = { .type = muskets, .quantity = 1 };
    agent.EXPECT__pick_dump_cargo( comms ).returns( 5 );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    // Not yet removed.
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type = e_commodity::muskets,
                                 .quantity = 1 } } );

    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::cotton,
                                 .quantity = 23 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::horses,
                                 .quantity = 50 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) == nothing );
  }

  // Dump #3, escape.
  {
    using enum e_commodity;
    map<int /*slot*/, Commodity> comms;
    comms[0] = { .type = cotton, .quantity = 23 };
    comms[4] = { .type = horses, .quantity = 50 };
    agent.EXPECT__pick_dump_cargo( comms ).returns( nothing );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == false );
    // Nothing changed. And note that we don't call `perform`
    // here.
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::cotton,
                                 .quantity = 23 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::horses,
                                 .quantity = 50 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) == nothing );
  }

  // Dump #4, slot 0.
  {
    using enum e_commodity;
    map<int /*slot*/, Commodity> comms;
    comms[0] = { .type = cotton, .quantity = 23 };
    comms[4] = { .type = horses, .quantity = 50 };
    agent.EXPECT__pick_dump_cargo( comms ).returns( 0 );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    // Not yet removed.
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::cotton,
                                 .quantity = 23 } } );

    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::horses,
                                 .quantity = 50 } } );
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) == nothing );
  }

  // Dump #5, slot 4.
  {
    using enum e_commodity;
    map<int /*slot*/, Commodity> comms;
    comms[4] = { .type = horses, .quantity = 50 };
    agent.EXPECT__pick_dump_cargo( comms ).returns( 4 );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    // Not yet removed.
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::horses,
                                 .quantity = 50 } } );

    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) == nothing );
  }

  // Dump #5, no more cargo.
  {
    agent.EXPECT__message_box(
        "This unit is not carrying any cargo that "
        "can be dumped overboard." );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == false );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 2 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 3 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 4 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 5 ) == nothing );
  }
}

TEST_CASE( "[command-dump] wagon train" ) {
  World w;
  MockIAgent& agent = w.agent();
  UnitId id = w.add_unit_on_map( e_unit_type::wagon_train,
                                 Coord{ .x = 1, .y = 1 } )
                  .id();
  unique_ptr<CommandHandler> handler =
      handle_command( w.engine(), w.ss(), w.ts(), agent,
                      w.default_player(), id, command::dump{} );
  Unit& unit       = w.units().unit_for( id );
  CargoHold& cargo = unit.cargo();
  Commodity comm;
  int slot = 0;

  // 23 cotton in slot 0.
  comm = { .type = e_commodity::cotton, .quantity = 23 };
  slot = 0;
  add_commodity_to_cargo( w.units(), comm, unit.cargo(), slot,
                          /*try_other_slots=*/false );

  // Sanity check.
  REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
           Cargo::commodity{
             .obj = Commodity{ .type     = e_commodity::cotton,
                               .quantity = 23 } } );
  REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );

  // Dump #1, slot 0.
  {
    using enum e_commodity;
    map<int /*slot*/, Commodity> comms;
    comms[0] = { .type = cotton, .quantity = 23 };
    agent.EXPECT__pick_dump_cargo( comms ).returns( 0 );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == true );
    // Not yet removed.
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) ==
             Cargo::commodity{
               .obj = Commodity{ .type     = e_commodity::cotton,
                                 .quantity = 23 } } );

    wait<> w_perform = handler->perform();
    REQUIRE( !w_perform.exception() );
    REQUIRE( w_perform.ready() );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
  }

  // Dump #2, no more cargo.
  {
    agent.EXPECT__message_box(
        "This unit is not carrying any cargo that "
        "can be dumped overboard." );
    wait<bool> w_confirm = handler->confirm();
    REQUIRE( !w_confirm.exception() );
    REQUIRE( w_confirm.ready() );
    REQUIRE( *w_confirm == false );
    REQUIRE( cargo.cargo_starting_at_slot( 0 ) == nothing );
    REQUIRE( cargo.cargo_starting_at_slot( 1 ) == nothing );
  }
}

TEST_CASE( "[command-dump] non-cargo unit" ) {
  World w;
  MockIAgent& agent = w.agent();
  UnitId id = w.add_unit_on_map( e_unit_type::free_colonist,
                                 Coord{ .x = 1, .y = 1 } )
                  .id();
  unique_ptr<CommandHandler> handler =
      handle_command( w.engine(), w.ss(), w.ts(), agent,
                      w.default_player(), id, command::dump{} );

  agent.EXPECT__message_box(
      "Only units with cargo holds can dump cargo "
      "overboard." );
  wait<bool> w_confirm = handler->confirm();
  REQUIRE( !w_confirm.exception() );
  REQUIRE( w_confirm.ready() );
  REQUIRE( *w_confirm == false );
}

} // namespace
} // namespace rn
