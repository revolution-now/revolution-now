/****************************************************************
**continental-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-06-21.
*
* Description: Unit tests for the continental module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/continental.hpp"

// Testing.
#include "test/fake/world.hpp"
#include "test/mocking.hpp"
#include "test/mocks/igui.hpp"
#include "test/util/coro.hpp"

// Revolution Now
#include "src/colony-mgr.hpp"

// ss
#include "src/ss/ref.hpp"
#include "src/ss/unit-composition.hpp"
#include "src/ss/unit.hpp"

// refl
#include "src/refl/to-str.hpp"

// base
#include "src/base/to-str-ext-std.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

using ::mock::matchers::StrContains;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_default_player();
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[continental] compute_continental_promotion" ) {
  world w;
  ContinentalPromotion expected;

  Player& player = w.default_player();
  Colony& colony = w.add_colony( { .x = 0, .y = 0 } );

  auto const f = [&] {
    return compute_continental_promotion( w.ss(), player,
                                          colony.id );
  };

  auto const add_unit = [&]( e_unit_type const type ) -> Unit& {
    return w.add_unit_on_map( type, colony.location );
  };

  using enum e_unit_type;

  // Default.
  expected = {};
  REQUIRE( f() == expected );

  add_unit( pioneer );
  add_unit( soldier );
  add_unit( privateer );

  expected = {};
  REQUIRE( f() == expected );

  UnitId const vet_sol_1 = add_unit( veteran_soldier ).id();

  expected = { .units = { vet_sol_1 } };
  REQUIRE( f() == expected );

  expected = { .units = { vet_sol_1 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::bells );

  expected = { .units = { vet_sol_1 } };
  REQUIRE( f() == expected );

  UnitId const vet_drag_1 = add_unit( veteran_dragoon ).id();

  expected = { .units = { vet_drag_1 } };
  REQUIRE( f() == expected );

  UnitId const vet_sol_2  = add_unit( veteran_soldier ).id();
  UnitId const vet_drag_2 = add_unit( veteran_dragoon ).id();
  UnitId const vet_sol_3  = add_unit( veteran_soldier ).id();
  UnitId const vet_drag_3 = add_unit( veteran_dragoon ).id();
  UnitId const vet_sol_4  = add_unit( veteran_soldier ).id();
  UnitId const vet_drag_4 = add_unit( veteran_dragoon ).id();
  UnitId const vet_sol_5  = add_unit( veteran_soldier ).id();
  UnitId const vet_drag_5 = add_unit( veteran_dragoon ).id();
  BASE_CHECK( vet_sol_2 == UnitId{ 7 } );

  expected = { .units = { vet_drag_1 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::bells );
  w.add_unit_indoors( colony.id, e_indoor_job::bells );

  expected = { .units = { vet_drag_1 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 2;
  expected = { .units = { vet_drag_1 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 3;
  expected = { .units = { vet_drag_1 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 4;
  expected = { .units = { vet_drag_1 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::hammers );
  expected = { .units = { vet_drag_1, vet_drag_2 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::hammers );
  w.add_unit_indoors( colony.id, e_indoor_job::hammers );
  expected = { .units = { vet_drag_1, vet_drag_2 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 5;
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 6;
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::cloth );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 7;
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::cloth );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 8;
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::cloth );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::rum );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4, vet_drag_5 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::rum );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4, vet_drag_5 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 9;
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4, vet_drag_5 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::rum );
  expected = { .units = { vet_sol_1, vet_drag_1, vet_drag_2,
                          vet_drag_3, vet_drag_4, vet_drag_5 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::cigars );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4 } };
  BASE_CHECK( colony_population( colony ) == 13 ); // checkpoint
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::cigars );
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3 } };
  BASE_CHECK( colony_population( colony ) == 14 ); // checkpoint
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 10;
  expected = { .units = { vet_drag_1, vet_drag_2, vet_drag_3,
                          vet_drag_4, vet_drag_5 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 11;
  expected = { .units = { vet_sol_1, vet_drag_1, vet_sol_2,
                          vet_drag_2, vet_drag_3, vet_drag_4,
                          vet_drag_5 } };
  REQUIRE( f() == expected );

  colony.sons_of_liberty.num_rebels_from_bells_only = 14;
  expected = { .units = { vet_sol_1, vet_drag_1, vet_sol_2,
                          vet_drag_2, vet_drag_3, vet_drag_4,
                          vet_drag_5 } };
  REQUIRE( f() == expected );

  w.add_unit_indoors( colony.id, e_indoor_job::cigars );
  w.add_unit_indoors( colony.id, e_indoor_job::muskets );
  w.add_unit_indoors( colony.id, e_indoor_job::muskets );
  w.add_unit_indoors( colony.id, e_indoor_job::muskets );
  w.add_unit_indoors( colony.id, e_indoor_job::tools );
  w.add_unit_indoors( colony.id, e_indoor_job::tools );
  BASE_CHECK( colony_population( colony ) == 20 ); // checkpoint
  colony.sons_of_liberty.num_rebels_from_bells_only = 20;
  expected = { .units = { vet_sol_1, vet_drag_1, vet_sol_2,
                          vet_drag_2, vet_sol_3, vet_drag_3,
                          vet_sol_4, vet_drag_4, vet_sol_5,
                          vet_drag_5 } };
  REQUIRE( f() == expected );
}

TEST_CASE( "[continental] do_continental_promotion" ) {
  world w;
  ContinentalPromotion promotion;

  auto const f = [&] {
    do_continental_promotion( w.ss(), w.ts(), promotion );
  };

  auto const add_unit = [&]( e_unit_type const type ) -> Unit& {
    return w.add_unit_on_map( type, { .x = 0, .y = 0 } );
  };

  using enum e_unit_type;

  Unit const& vet_sol  = add_unit( veteran_soldier );
  Unit const& vet_drag = add_unit( veteran_dragoon );

  REQUIRE( vet_sol.type() == veteran_soldier );
  REQUIRE( vet_drag.type() == veteran_dragoon );

  promotion = { .units = { vet_sol.id(), vet_drag.id() } };
  f();

  REQUIRE( vet_sol.type() == continental_army );
  REQUIRE( vet_drag.type() == continental_cavalry );
}

TEST_CASE( "[continental] continental_promotion_ui_seq" ) {
  world w;
  ContinentalPromotion promotion;

  Colony& colony = w.add_colony( { .x = 0, .y = 0 } );

  auto const f = [&] {
    co_await_test( continental_promotion_ui_seq(
        w.ss(), w.gui(), promotion, colony.id ) );
  };

  // Default.
  f();

  // One unit.
  promotion = { .units = { UnitId{ 1 } } };
  w.gui().EXPECT__message_box(
      StrContains( "[One] veteran unit has been" ) );
  f();

  // Multiple units.
  promotion = { .units = { UnitId{ 1 }, UnitId{ 2 }, UnitId{ 3 },
                           UnitId{ 4 } } };
  w.gui().EXPECT__message_box(
      StrContains( "[Four] veteran units have been" ) );
  f();
}

} // namespace
} // namespace rn
