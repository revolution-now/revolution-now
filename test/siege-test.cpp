/****************************************************************
**siege-test.cpp
*
* Project: Revolution Now
*
* Created by David P. Sicilia on 2025-05-25.
*
* Description: Unit tests for the siege module.
*
*****************************************************************/
#include "test/testing.hpp"

// Under test.
#include "src/siege.hpp"

// Testing.
#include "test/fake/world.hpp"

// Revolution Now
#include "src/unit-ownership.hpp"

// ss
#include "src/ss/dwelling.rds.hpp"
#include "src/ss/native-unit.rds.hpp"
#include "src/ss/players.rds.hpp"
#include "src/ss/ref.hpp"
#include "src/ss/unit-composition.hpp"

// Must be last.
#include "test/catch-common.hpp" // IWYU pragma: keep

namespace rn {
namespace {

using namespace std;

/****************************************************************
** Fake World Setup
*****************************************************************/
struct world : testing::World {
  world() {
    add_player( e_player::english );
    add_player( e_player::french );
    add_player( e_player::spanish );
    add_player( e_player::ref_english );
    set_default_player_type( e_player::english );
    create_default_map();
  }

  void create_default_map() {
    MapSquare const _ = make_ocean();
    MapSquare const L = make_grassland();
    vector<MapSquare> tiles{
      L, L, L, //
      L, _, L, //
      L, L, L, //
      L, L, L, //
      L, L, L, //
    };
    build_map( std::move( tiles ), 3 );
  }
};

/****************************************************************
** Test Cases
*****************************************************************/
TEST_CASE( "[siege] is_colony_under_siege" ) {
  world w;

  Colony const& colony = w.add_colony( { .x = 0, .y = 2 } );

  auto const f = [&] {
    return is_colony_under_siege( w.ss(), colony );
  };

  REQUIRE_FALSE( f() );

  // Foreign ship unit.
  w.add_unit_on_map( e_unit_type::frigate, { .x = 1, .y = 1 },
                     e_player::french );
  REQUIRE_FALSE( f() );

  Dwelling const& dwelling =
      w.add_dwelling( { .x = 0, .y = 0 }, e_tribe::tupi );
  NativeUnitId const native_id =
      w.add_native_unit_on_map( e_native_unit_type::armed_brave,
                                { .x = 0, .y = 1 }, dwelling.id )
          .id;
  REQUIRE_FALSE( f() );

  NativeUnitOwnershipChanger( w.ss(), native_id ).destroy();

  // Friendly unit.
  w.add_unit_on_map( e_unit_type::soldier, { .x = 0, .y = 1 },
                     e_player::english );
  REQUIRE_FALSE( f() );

  // Foreign unit.
  w.add_unit_on_map( e_unit_type::soldier, { .x = 1, .y = 2 },
                     e_player::french );
  REQUIRE_FALSE( f() );

  // Foreign unit.
  w.add_unit_on_map( e_unit_type::dragoon, { .x = 1, .y = 2 },
                     e_player::french );
  REQUIRE( f() );

  // Friendly unit.
  w.add_unit_on_map( e_unit_type::artillery, { .x = 1, .y = 3 },
                     e_player::english );
  REQUIRE_FALSE( f() );

  // Foreign unit.
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 3 }, e_player::ref_english );
  REQUIRE_FALSE( f() );
  w.add_unit_on_map( e_unit_type::veteran_soldier,
                     { .x = 0, .y = 3 }, e_player::ref_english );
  REQUIRE( f() );

  // Foreign unit.
  w.add_unit_on_map( e_unit_type::soldier, { .x = 0, .y = 3 },
                     e_player::ref_english );
  REQUIRE( f() );

  w.english().relationship_with[e_player::ref_english] =
      e_euro_relationship::peace;
  REQUIRE_FALSE( f() );
  w.english().relationship_with[e_player::ref_english] =
      e_euro_relationship::war;
  REQUIRE( f() );

  // Friendly unit.
  w.add_unit_on_map( e_unit_type::veteran_dragoon,
                     { .x = 0, .y = 2 }, e_player::english );
  REQUIRE( f() );

  // Friendly unit.
  w.add_unit_on_map( e_unit_type::scout, { .x = 0, .y = 2 },
                     e_player::english );
  REQUIRE( f() );
  w.add_unit_on_map( e_unit_type::free_colonist,
                     { .x = 0, .y = 2 }, e_player::english );
  REQUIRE( f() );
  w.add_unit_on_map( e_unit_type::damaged_artillery,
                     { .x = 0, .y = 2 }, e_player::english );
  REQUIRE_FALSE( f() );
}

} // namespace
} // namespace rn
